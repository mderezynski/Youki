//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//  --
//
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <gtkmm.h>
#include <glibmm.h>
#include <giomm.h>
#include <glibmm/i18n.h>
#include <glib/gstdio.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/stat.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <taglib-gio.h>
#include <fileref.h>
#include <tfile.h>
#include <tag.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <id3v2framefactory.h>
#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>
#include <attachedpictureframe.h>

#include "mpx/mpx-audio.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-covers-stores.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-minisoup.hh"
#include "mpx/mpx-network.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-string.hh"
#include "mpx/xml/xml.hh"
#include "mpx/algorithm/ld.hh"

using namespace Glib;
using namespace TagLib;

namespace
{
  int
  pixel_size(
        MPX::CoverSize size
  )
  {
    int pixel_size = 0;
    switch (size)
    {
      case MPX::COVER_SIZE_ALBUM:
        pixel_size = 90;
        break;

      case MPX::COVER_SIZE_DEFAULT:
        pixel_size = 384;
        break;

      default:
        pixel_size = 256;
    }
    return pixel_size;
  }

  using namespace MPX;

  void
  create_or_lock( CoverFetchContext* data, MutexMap & mmap )
  {
    if( mmap.find( data->qualifier.mbid ) == mmap.end() )
    {
        MutexPtr p (new Glib::RecMutex );
        p->lock();
        mmap.insert(std::make_pair( data->qualifier.mbid, p ));
    }
    else
    {
        mmap[data->qualifier.mbid]->lock();
    }
  }

  void
  create_or_unlock( CoverFetchContext* data, MutexMap & mmap )
  {
    if( mmap.find( data->qualifier.mbid ) == mmap.end() )
    {
        MutexPtr p (new Glib::RecMutex );
        mmap.insert(std::make_pair( data->qualifier.mbid, p ));
    }
    else
    {
        mmap[data->qualifier.mbid]->unlock();
    }
  }
}

namespace MPX
{
    Covers::Covers ()
    : Service::Base("mpx-service-covers")
    , m_rebuild(0)
    , m_rebuilt(0)
    {
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), PACKAGE, "covers", NULL));
        g_mkdir(path.get(), 0700);

        m_stores_all.push_back(StorePtr(new LocalCovers(*this)));
        m_stores_all.push_back(StorePtr(new MusicBrainzCovers(*this)));
        m_stores_all.push_back(StorePtr(new AmazonCovers(*this)));
        m_stores_all.push_back(StorePtr(new AmapiCovers(*this)));
        m_stores_all.push_back(StorePtr(new InlineCovers(*this)));

        rebuild_stores();

        mcs->subscribe(
                "Preferences-CoverArtSources",
                "SourceActive4",
                sigc::mem_fun(
                    *this,
                    &Covers::source_pref_changed_callback
        ));
    }

    void
    Covers::precache(
        MPX::Library* const library
    )
    {
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), PACKAGE, "covers", NULL));

        SQL::RowV v;

        library->getSQL( v, "SELECT DISTINCT mb_album_id FROM album" );

        for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
        {
            try{
                const std::string& mbid = boost::get<std::string>((*i)["mb_album_id"]);
                const std::string& cpth = build_filename( path.get(), mbid) + ".png";
                if( Glib::file_test( cpth, Glib::FILE_TEST_EXISTS )) 
                {
                    Glib::RefPtr<Gdk::Pixbuf> cover = Gdk::Pixbuf::create_from_file( cpth ); 
                    m_pixbuf_cache.insert(std::make_pair( mbid, cover ));
                }
            }
            catch( Gdk::PixbufError )
            {
            }
            catch( Glib::FileError )
            {  
            }
        }
    }

    void
    Covers::source_pref_changed_callback(const std::string& domain, const std::string& key, const Mcs::KeyVariant& value )
    {
        g_atomic_int_set(&m_rebuilt, 0);
        g_atomic_int_set(&m_rebuild, 1);
    }

    void
    Covers::rebuild_stores()
    {
        m_stores_cur = StoresVec (m_stores_all.size(), StorePtr());

        int at0 = mcs->key_get<int>("Preferences-CoverArtSources", "Source0");
        int at1 = mcs->key_get<int>("Preferences-CoverArtSources", "Source1");
        int at2 = mcs->key_get<int>("Preferences-CoverArtSources", "Source2");
        int at3 = mcs->key_get<int>("Preferences-CoverArtSources", "Source3");
        int at4 = mcs->key_get<int>("Preferences-CoverArtSources", "Source4");

        bool use0 = mcs->key_get<bool>("Preferences-CoverArtSources", "SourceActive0");
        bool use1 = mcs->key_get<bool>("Preferences-CoverArtSources", "SourceActive1");
        bool use2 = mcs->key_get<bool>("Preferences-CoverArtSources", "SourceActive2");
        bool use3 = mcs->key_get<bool>("Preferences-CoverArtSources", "SourceActive3");
        bool use4 = mcs->key_get<bool>("Preferences-CoverArtSources", "SourceActive4");

        if (use0)
            {
                m_stores_cur[0] = m_stores_all[at0];
            }

        if (use1)
            {
                m_stores_cur[1] = m_stores_all[at1];
            }

        if (use2)
            {
                m_stores_cur[2] = m_stores_all[at2];
            }

        if (use3)
            {
                m_stores_cur[3] = m_stores_all[at3];
            }

        if (use4)
            {
                m_stores_cur[4] = m_stores_all[at4];
            }
    }

    void
    Covers::cache_artwork(
        const std::string& mbid,
        RefPtr<Gdk::Pixbuf> cover
    )
    {
        cover->save( get_thumb_path( mbid ), "png" );
        m_pixbuf_cache[mbid] = cover;
    }

    std::string
    Covers::get_thumb_path(
        std::string mbid
    )
    {
        using boost::algorithm::replace_all;
        replace_all(mbid, "/","_");
        std::string basename = (boost::format ("%s.png") % mbid).str ();
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), PACKAGE, "covers", basename.c_str(), NULL));
        return std::string(path.get());
    }

/*
    void
    Covers::store_has_found_cb( CoverFetchContext* data )
    {
    }
 
*/

    void 
    Covers::cache(
          const RequestQualifier& qual
        , bool                    acquire
    )
    {
        if( g_atomic_int_get(&m_rebuild) )
        {
            rebuild_stores();
            g_atomic_int_set(&m_rebuild, 0);
            g_atomic_int_set(&m_rebuilt, 1);
        }

        std::string thumb_path = get_thumb_path( qual.mbid ) ;

        if( file_test( thumb_path, FILE_TEST_EXISTS ))
        {
            Signals.GotCover.emit( qual.mbid );
            return; 
        }

        if( acquire && m_stores_cur.size() )
        {
            int i = 0; 

            CoverFetchContext * data = new CoverFetchContext( qual, m_stores_cur );

            if( i < data->stores.size() )
            {
                while( !data->stores[i] )
                {
                    i++;
                }
            
                if( i < data->stores.size() )
                { 
                    create_or_lock( data, m_mutexes ) ;

                    StorePtr store = data->stores[i] ; 

                    store->load_artwork( data ) ;

                    if( store->get_state() == FETCH_STATE_COVER_SAVED )
                    {
                        Signals.GotCover.emit( data->qualifier.mbid );
                        create_or_unlock( data, m_mutexes );
                        delete data;
                        return;
                    }
                }
            }

            delete data;
        }
    }

    bool
    Covers::fetch(
        const std::string&      mbid,
        RefPtr<Gdk::Pixbuf>&    cover
    )
    {
        PixbufCache::const_iterator i = m_pixbuf_cache.find(mbid);
        if (i != m_pixbuf_cache.end())
        {
            cover = (*i).second; 
            return true;
        }

        std::string thumb_path = get_thumb_path (mbid);
        if (file_test( thumb_path, FILE_TEST_EXISTS ))
        {
            try{
              cover = Gdk::Pixbuf::create_from_file( thumb_path ); 
              m_pixbuf_cache.insert (std::make_pair(mbid, cover));
              return true;
            }
            catch( Gdk::PixbufError )
            {
            }
            catch( Glib::FileError )
            {
            }
        }

        return false;
    }

    bool
    Covers::fetch(
        const std::string&                  mbid,
        Cairo::RefPtr<Cairo::ImageSurface>& surface,
        CoverSize                           size
    )
    {
        Glib::RefPtr<Gdk::Pixbuf> pb;

        if(fetch( mbid, pb ))
        {
            int px = pixel_size (size);
            
            try{
                surface = Util::cairo_image_surface_from_pixbuf(
                              pb->scale_simple(
                                    px,
                                    px,
                                    Gdk::INTERP_BILINEAR
                ));
                return true;
            }
            catch( Gdk::PixbufError )
            {
            }
            catch( Glib::FileError )
            {
            }
        }

        return false;
    }

    void
    Covers::purge()
    {
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), PACKAGE, "covers", NULL));

        Glib::Dir dir (path.get());
        StrV v (dir.begin(), dir.end());
        dir.close ();

        for(StrV::const_iterator i = v.begin(); i != v.end(); ++i)
        {
            std::string fullpath = build_filename(path.get(), *i);
            g_unlink(fullpath.c_str());
        }
    }
}
