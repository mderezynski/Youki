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
#include "mpx/algorithm/ld.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-minisoup.hh"
#include "mpx/mpx-network.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-string.hh"
#include "mpx/xml/xml.hh"

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
        pixel_size = 256;
        break;

      default:
        pixel_size = 256;
    }
    return pixel_size;
  }
}

namespace MPX
{
    Covers::Covers (MPX::Service::Manager& services)
    : sigx::glib_auto_dispatchable()
    , Service::Base("mpx-service-covers")
    , m_rebuild(0)
    , m_rebuilt(0)
    {
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), PACKAGE, "covers", NULL));
        g_mkdir(path.get(), 0700);

        m_all_stores.push_back(StoreP(new LocalCovers(*this)));
        m_all_stores.push_back(StoreP(new MusicBrainzCovers(*this)));
        m_all_stores.push_back(StoreP(new AmazonCovers(*this)));
        m_all_stores.push_back(StoreP(new AmapiCovers(*this)));
        m_all_stores.push_back(StoreP(new InlineCovers(*this)));

        for( size_t i = 0; i < m_all_stores.size(); i++ )
        {
            m_all_stores[i]->not_found_callback().connect(sigc::mem_fun(*this, &Covers::store_not_found_cb));
        }

        rebuild_stores();
        mcs->subscribe ("CoversSubscription0", "Preferences-CoverArtSources",  "SourceActive4", sigc::mem_fun( *this, &Covers::source_pref_changed_callback ));
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
        m_current_stores = StoresT (m_all_stores.size(), StoreP());

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
                m_current_stores[0] = m_all_stores[at0];
            }

        if (use1)
            {
                m_current_stores[1] = m_all_stores[at1];
            }

        if (use2)
            {
                m_current_stores[2] = m_all_stores[at2];
            }

        if (use3)
            {
                m_current_stores[3] = m_all_stores[at3];
            }

        if (use4)
            {
                m_current_stores[4] = m_all_stores[at4];
            }
    }
 
    void
    Covers::store_not_found_cb (CoverFetchData* data)
    {
        g_message(G_STRFUNC);

        if( g_atomic_int_get(&m_rebuilt) )
        {
            g_message(G_STRLOC ": Erasing MBID [%s] from RequestKeeper", data->mbid.c_str());
            RequestKeeperT::iterator i = RequestKeeper.find(data->mbid);
            RequestKeeper.erase(i);
            delete data;
            return;
        }

        int i = RequestKeeper[data->mbid];
        i++;

        if(i < data->m_req_stores.size())
        {
            while( !data->m_req_stores[i] )
            {
                i++;
            }
        

            if(i < data->m_req_stores.size())
            { 
                g_message(G_STRLOC ": Trying next store, m_req_stores size is %d", int(data->m_req_stores.size()));
                RequestKeeperT::iterator iter = RequestKeeper.find(data->mbid);
                RequestKeeper.erase(iter);
                RequestKeeper[data->mbid] = i;

                StoreP store = data->m_req_stores[i]; // to avoid race conditions
                store->load_artwork(data);
                return;
            }
        }

        g_message(G_STRLOC ": Erasing MBID [%s] from RequestKeeper", data->mbid.c_str());
        RequestKeeperT::iterator iter = RequestKeeper.find(data->mbid);
        RequestKeeper.erase(iter);
        delete data;
    }

    void
    Covers::cache_artwork(
        const std::string& mbid,
        RefPtr<Gdk::Pixbuf> cover
    )
    {
        m_pixbuf_cache.insert(std::make_pair(mbid, cover));
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

    void 
    Covers::cache(
        const std::string& mbid,
        const std::string& asin,
        const std::string& uri,
        const std::string& artist,
        const std::string& album,
        bool               acquire
    )
    {
        Glib::Mutex::Lock L (RequestKeeperLock);

        if( RequestKeeper.count(mbid) )
        {
            return;
        }

        if( g_atomic_int_get(&m_rebuild) )
        {
            rebuild_stores();
            g_atomic_int_set(&m_rebuild, 0);
            g_atomic_int_set(&m_rebuilt, 1);
        }

        std::string thumb_path = get_thumb_path (mbid);
        if( file_test( thumb_path, FILE_TEST_EXISTS ))
        {
            Signals.GotCover.emit(mbid);
            return; 
        }

        if( acquire )
        {
            if(m_current_stores.size())
            {
                RequestKeeper[mbid] = 0;
                CoverFetchData * data = new CoverFetchData(asin, mbid, uri, artist, album, m_current_stores);
                data->m_req_stores[0]->load_artwork(data);
            }
        }
    }

    bool
    Covers::fetch(
        const std::string&      mbid,
        RefPtr<Gdk::Pixbuf>&    cover
    )
    {
        MPixbufCache::const_iterator i = m_pixbuf_cache.find(mbid);
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
            } catch( Gdk::PixbufError )
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
        MSurfaceCache::const_iterator i = m_surface_cache[size].find (mbid);
        if (i != m_surface_cache[size].end())
        {
            surface = (*i).second; 
            return true;
        }

        std::string thumb_path = get_thumb_path (mbid);
        if (file_test (thumb_path, FILE_TEST_EXISTS))
        {
            int px = pixel_size (size);
            
            try{
                surface = Util::cairo_image_surface_from_pixbuf(
                              Gdk::Pixbuf::create_from_file(thumb_path)->scale_simple(
                                                                            px,
                                                                            px,
                                                                            Gdk::INTERP_BILINEAR
                ));
                m_surface_cache[size].insert( std::make_pair( mbid, surface ));
                return true;
            } catch( Gdk::PixbufError )
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
