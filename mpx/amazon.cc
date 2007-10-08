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

#include "amazon.hh"
#include "minisoup.hh"
#include "network.hh"
#include "uri.hh"
#include "util-file.hh"
#include "util-graphics.hh"
using namespace Glib;

namespace
{
  const char* amazon_hosts[] =
  {
    "images.amazon.com",  
    "images-eu.amazon.com",
    "ec1.images-amazon.com",
  };

  static boost::format amazon_f ("http://%s/images/P/%s.01.LZZZZZZZ.jpg");
  static boost::format amazon_f_small ("http://%s/images/P/%s.01.MZZZZZZZ.jpg");

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
  namespace Amazon
  {
    Covers::Covers (NM & nm)
    : m_NM (nm)
    {}

    bool
    Covers::try_fetching (std::string const& site, RefPtr<Gdk::Pixbuf>& cover)
    {
      Soup::RequestSyncRefP request = Soup::RequestSync::create (site);
      guint code = request->run (); 
  
      try{
          if (code == 200)
          {
            RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create ();
            loader->write (reinterpret_cast<const guint8*>(request->get_data_raw()), request->get_data_size());
            loader->close ();

            if (loader->get_pixbuf()->get_width() == 1 && loader->get_pixbuf()->get_height() == 1)
            {
              /*DO NOTHING*/
            }
            else
            { 
              cover = loader->get_pixbuf ();
            }
          }
      }
      catch (Gdk::PixbufError & cxe)
      {
        cover = RefPtr<Gdk::Pixbuf> (0);
      }
      return cover;
    }
    
    RefPtr<Gdk::Pixbuf>
    Covers::site_fetch_and_save_cover (ustring const& asin, std::string const& thumb_path)
    {
      RefPtr<Gdk::Pixbuf> cover;
      for (unsigned int n = 0; n < G_N_ELEMENTS(amazon_hosts); ++n)
      {
          try {
              if (!try_fetching((amazon_f % amazon_hosts[n] % asin).str(), cover))
                try_fetching((amazon_f_small % amazon_hosts[n] % asin).str(), cover);
              
              if (cover)
                {
                  cover->save (thumb_path, "png");
                  m_pixbuf_cache.insert (std::make_pair (asin, cover));
                  return cover;
                }
            }
          catch (Gdk::PixbufError & cxe)
            {
            }
      }
      throw NoCoverError("No cover on Amazon found!");
    }
    
    Cairo::RefPtr<Cairo::ImageSurface>
    Covers::site_fetch_and_save_cover_cairo (ustring const& asin, std::string const& thumb_path, CoverSize size)
    {
      RefPtr<Gdk::Pixbuf> cover;
      for (unsigned int n = 0; n < G_N_ELEMENTS(amazon_hosts); ++n)
      {
          try {
              if (!try_fetching((amazon_f % amazon_hosts[n] % asin).str(), cover))
                try_fetching((amazon_f_small % amazon_hosts[n] % asin).str(), cover);
              
              if (cover)
                {
                  cover->save (thumb_path, "png");
                  int px = pixel_size (size);
                  Cairo::RefPtr<Cairo::ImageSurface> surface =
                    Util::cairo_image_surface_from_pixbuf (cover->scale_simple (px, px, Gdk::INTERP_BILINEAR));
                  m_surface_cache[size].insert (std::make_pair (asin, surface));
                  return surface;
                }
            }
          catch (Gdk::PixbufError & cxe)
            {}
      }
      throw NoCoverError("No cover on Amazon found!");
    }

    std::string
    Covers::get_thumb_path (ustring const& asin)
    {
      std::string basename = (boost::format ("%s.png") % asin).str ();
      Glib::ScopedPtr<char> path (g_build_filename(g_get_user_cache_dir(), "mpx", "covers", basename.c_str(), NULL));
      return std::string(path.get());
    }

    void 
    Covers::cache (ustring const& asin)
    {
      std::string thumb_path = get_thumb_path (asin);
      if (file_test (thumb_path, FILE_TEST_EXISTS))
        return;

      site_fetch_and_save_cover (asin, thumb_path);
    }

    void 
    Covers::fetch (ustring const& asin, RefPtr<Gdk::Pixbuf>& cover, bool only_cached)
    {
      MPixbufCache::const_iterator i = m_pixbuf_cache.find (asin);
      if (i != m_pixbuf_cache.end())
      {
        cover = i->second; 
        return;
      }

      std::string thumb_path = get_thumb_path (asin);
      if (file_test (thumb_path, FILE_TEST_EXISTS))
      {
        cover = Gdk::Pixbuf::create_from_file (thumb_path); 
        m_pixbuf_cache.insert (std::make_pair (asin, cover));
        return;
      }

      if (only_cached || m_NM.Check_Status())
        throw NoCoverError("No cached cover present");

      cover = site_fetch_and_save_cover (asin, thumb_path);
    }

    Cairo::RefPtr<Cairo::ImageSurface>
    Covers::fetch (ustring const& asin, CoverSize size, bool only_cached)
    {
      MSurfaceCache::const_iterator i = m_surface_cache[size].find (asin);
      if (i != m_surface_cache[size].end())
      {
        return i->second; 
      }

      std::string thumb_path = get_thumb_path (asin);
      if (file_test (thumb_path, FILE_TEST_EXISTS))
      {
        int px = pixel_size (size);
        Cairo::RefPtr<Cairo::ImageSurface> surface =
          Util::cairo_image_surface_from_pixbuf (Gdk::Pixbuf::create_from_file (thumb_path)->scale_simple (px, px, Gdk::INTERP_BILINEAR));
        m_surface_cache[size].insert (std::make_pair (asin, surface));
        return surface;
      }

      if (only_cached || m_NM.Check_Status())
        throw NoCoverError("No cached cover present");

      return site_fetch_and_save_cover_cairo (asin, thumb_path, size);
    }
  }
}
