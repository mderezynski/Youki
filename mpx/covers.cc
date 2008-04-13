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
#include <glibmm/i18n.h>
#include <glib/gstdio.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/stat.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "mpx/covers.hh"
#include "mpx/minisoup.hh"
#include "mpx/network.hh"
#include "mpx/uri.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/xml.hh"

using namespace Glib;

namespace
{
  static boost::format amazon_f[] =
  {
    boost::format("http://images.amazon.com/images/P/%s.01.LZZZZZZZ.jpg"),
    boost::format("http://images-eu.amazon.com/images/P/%s.01.LZZZZZZZ.jpg"),
    boost::format("http://ec1.images-amazon.com/images/P/%s.01.LZZZZZZZ.jpg"),
    boost::format("http://images.amazon.com/images/P/%s.01.MZZZZZZZ.jpg"),
    boost::format("http://images-eu.amazon.com/images/P/%s.01.MZZZZZZZ.jpg"),
    boost::format("http://ec1.images-amazon.com/images/P/%s.01.MZZZZZZZ.jpg"),
  };

  static boost::format mbxml_f ("http://www.uk.musicbrainz.org/ws/1/release/%s?type=xml&inc=url-rels");

  int
  pixel_size (MPX::CoverSize size)
  {
    int pixel_size = 0;
    switch (size)
    {
      case MPX::COVER_SIZE_ALBUM_LIST:
        pixel_size = 72;
        break;

      case MPX::COVER_SIZE_INFO_AREA:
        pixel_size = 48;
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
    Covers::Covers (NM & nm)
    : m_NM (nm)
    {
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), "mpx", "covers", NULL));
        g_mkdir(path.get(), 0700);
    }

    void
    Covers::site_fetch_and_save_cover_amazn (CoverFetchData * amzn_data)
    {
        amzn_data->request = Soup::Request::create ((amazon_f[amzn_data->n] % amzn_data->asin).str());
        amzn_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &Covers::reply_cb_amazn ), amzn_data ));
        amzn_data->request->run();
    }

    void
    Covers::site_fetch_and_save_cover_mbxml (CoverFetchData * amzn_data)
    {
        amzn_data->request = Soup::Request::create ((mbxml_f % amzn_data->mbid).str());
        amzn_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &Covers::reply_cb_mbxml ), amzn_data ));
        amzn_data->request->run();
    }

    void
    Covers::reply_cb_mbxml (char const* data, guint size, guint code, CoverFetchData* amzn_data)
    {
        if (code == 200)
        {
			std::string image_url;
			try{
				image_url = xpath_get_text (data, size,
					"/metadata/release/relation-list//relation[@type='CoverArtLink']/@target", "mb=http://musicbrainz.org/ns/mmd-1.0#"); 
			} catch (std::runtime_error & cxe)
			{
				delete amzn_data;
				return;
			}
			g_message("Image URL: %s", image_url.c_str());
	        amzn_data->request = Soup::Request::create (image_url);
		    amzn_data->request->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &Covers::reply_cb_amazn ), amzn_data ));
			amzn_data->request->run();
        }
        else
        {
			delete amzn_data;
        }
    }

    void
    Covers::reply_cb_amazn (char const* data, guint size, guint code, CoverFetchData* amzn_data)
    {
        if (code == 200)
        {
            RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create ();
            loader->write (reinterpret_cast<const guint8*>(amzn_data->request->get_data_raw()), amzn_data->request->get_data_size());
            loader->close ();

            RefPtr<Gdk::Pixbuf> cover = loader->get_pixbuf();

            if (cover->get_width() == 1 && cover->get_height() == 1)
            {
              /*DO NOTHING*/
            }
            else
            { 
                Glib::Mutex::Lock L (RequestKeeperLock);
                cover->save (get_thumb_path(amzn_data->mbid), "png");
                m_pixbuf_cache.insert (std::make_pair (amzn_data->mbid, cover));
                Signals.GotCover.emit(amzn_data->mbid);
                RequestKeeper.erase(amzn_data->mbid);
            }
            delete amzn_data;
            return;
        }
        else
        {
            ++(amzn_data->n);
            if(amzn_data->n > 5)
            {
                delete amzn_data;
                return; // no more hosts to try
            }
            site_fetch_and_save_cover_amazn(amzn_data);
        }
    }
    
    std::string
    Covers::get_thumb_path (std::string const& mbid)
    {
        std::string basename = (boost::format ("%s.png") % mbid).str ();
        Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), "mpx", "covers", basename.c_str(), NULL));
        return std::string(path.get());
    }

    void 
    Covers::cache (std::string const& mbid, std::string const& asin, bool acquire)
    {
        Glib::Mutex::Lock L (RequestKeeperLock);
        if(RequestKeeper.count(mbid))
        {
            return;
        }

        std::string thumb_path = get_thumb_path (mbid);
        if (file_test (thumb_path, FILE_TEST_EXISTS))
        {
            Signals.GotCover.emit(mbid);
            return; 
        }

        if(acquire)
        {
            CoverFetchData * data = new CoverFetchData(mbid, asin);
            RequestKeeper.insert(mbid);

            if(asin.empty())
                site_fetch_and_save_cover_mbxml(data);
            else
                site_fetch_and_save_cover_amazn(data);
        }
    }

    bool
    Covers::fetch (std::string const& mbid, RefPtr<Gdk::Pixbuf>& cover)
    {
        MPixbufCache::const_iterator i = m_pixbuf_cache.find(mbid);
        if (i != m_pixbuf_cache.end())
        {
          cover = i->second; 
          return true;
        }

        std::string thumb_path = get_thumb_path (mbid);
        if (file_test (thumb_path, FILE_TEST_EXISTS))
        {
          cover = Gdk::Pixbuf::create_from_file (thumb_path); 
          m_pixbuf_cache.insert (std::make_pair(mbid, cover));
          return true;
        }

        return false;
    }

    bool
    Covers::fetch (std::string const& mbid, Cairo::RefPtr<Cairo::ImageSurface>& surface, CoverSize size)
    {
        MSurfaceCache::const_iterator i = m_surface_cache[size].find (mbid);
        if (i != m_surface_cache[size].end())
        {
          surface = i->second; 
          return true;
        }

        std::string thumb_path = get_thumb_path (mbid);
        if (file_test (thumb_path, FILE_TEST_EXISTS))
        {
          int px = pixel_size (size);
          surface = Util::cairo_image_surface_from_pixbuf
			(Gdk::Pixbuf::create_from_file (thumb_path)->scale_simple (px, px, Gdk::INTERP_BILINEAR));
          m_surface_cache[size].insert (std::make_pair (mbid, surface));
          return true;
        }

        return false;
    }
}
