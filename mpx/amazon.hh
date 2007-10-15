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

#ifndef MPX_AMAZON_HH
#define MPX_AMAZON_HH

#include <string>
#include <map>
#include <set>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h> // bleh!
#include <cairomm/cairomm.h>
#include "network.hh"
#include "minisoup.hh"

namespace MPX
{
  enum CoverSize
  {
    COVER_SIZE_ALBUM_LIST = 0,
    COVER_SIZE_INFO_AREA  = 1,
    COVER_SIZE_DEFAULT    = 2,

    N_COVER_SIZES
  };

  namespace Amazon
  {
#include "exception.hh"

    EXCEPTION(NoCoverError)

    class Covers
    {
      public:

        typedef sigc::signal<void, Glib::ustring> SignalGotCover;

        struct SignalsT
        {
            SignalGotCover GotCover;
        };

        SignalsT Signals;

        SignalGotCover&
        signal_got_cover(){ return Signals.GotCover ; }

        Covers (NM&);

        bool
        fetch (Glib::ustring const& asin, Glib::RefPtr<Gdk::Pixbuf>&);

        bool
        fetch (Glib::ustring const& asin, Cairo::RefPtr<Cairo::ImageSurface>&, CoverSize size);

        void
        cache (Glib::ustring const& asin); // this operation is asynchronous

      private:

        NM &m_NM;

        typedef std::map <Glib::ustring, Glib::RefPtr<Gdk::Pixbuf> > MPixbufCache;
        MPixbufCache m_pixbuf_cache;

        typedef std::map <Glib::ustring, Cairo::RefPtr<Cairo::ImageSurface> > MSurfaceCache;
        MSurfaceCache m_surface_cache[N_COVER_SIZES];

        std::string
        get_thumb_path (Glib::ustring const& asin);

        struct AmazonFetchData
        {
            Soup::RequestRefP request;
            Glib::ustring asin;
            int n;

            AmazonFetchData (const Glib::ustring& asin_arg)
            : asin(asin_arg)
            , n (0)
            {}
        };
    
        std::set<Glib::ustring> RequestKeeper;
        Glib::Mutex RequestKeeperLock;

        void 
        site_fetch_and_save_cover (AmazonFetchData*);

        void
        reply_cb (char const* data, guint size, guint status_code, AmazonFetchData*);
    };
  }
}

#endif
