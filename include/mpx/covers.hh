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

#ifndef MPX_COVERS_HH
#define MPX_COVERS_HH

#include <string>
#include <map>
#include <set>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h> // bleh!
#include <cairomm/cairomm.h>
#include <sigx/sigx.h>

#include "mpx/network.hh"
#include "mpx/metadatareader-taglib.hh"
#include "mpx/minisoup.hh"

namespace MPX
{
  enum CoverSize
  {
    COVER_SIZE_ALBUM_LIST = 0,
    COVER_SIZE_INFO_AREA  = 1,
    COVER_SIZE_DEFAULT    = 2,

    N_COVER_SIZES
  };

  class Covers : public sigx::glib_auto_dispatchable
  {
	public:

	  typedef sigc::signal<void, const std::string&> SignalGotCover;

	  struct SignalsT
	  {
		  SignalGotCover GotCover;
	  };

	  SignalsT Signals;

	  SignalGotCover&
	  signal_got_cover()
      {
        return Signals.GotCover ;
      }

      typedef bool (Covers::*FetchFunc) (std::string const&, Glib::RefPtr<Gdk::Pixbuf>&);

	  Covers ();

	  bool
	  fetch (std::string const& mbid, Glib::RefPtr<Gdk::Pixbuf>&);

	  bool
	  fetch (std::string const& mbid, Cairo::RefPtr<Cairo::ImageSurface>&, CoverSize size);

	  void
	  cache (std::string const& mbid, std::string const& uri, std::string const& asin = std::string(), bool acquire = true);

	private:

	  struct CoverFetchData
	  {
		  Soup::RequestRefP request;
		  std::string mbid;
		  std::string asin;
          std::string uri;
		  int n;

		  CoverFetchData (const std::string& mbid_, const std::string& asin_, const std::string& uri_)
		  : mbid(mbid_)
		  , asin(asin_)
          , uri(uri_)
		  , n (0)
		  {
		  }
	  };

	  typedef std::set <std::string> RequestKeeperT;
	  typedef std::map <std::string, Glib::RefPtr<Gdk::Pixbuf> > MPixbufCache;
	  typedef std::map <std::string, Cairo::RefPtr<Cairo::ImageSurface> > MSurfaceCache;

	  std::string
	  get_thumb_path (std::string const& /*mbid*/);

	  void 
	  site_fetch_and_save_cover_mbxml (CoverFetchData*);
	  void 
	  site_fetch_and_save_cover_amazn (CoverFetchData*);

	  void
	  reply_cb_amazn (char const*, guint, guint, CoverFetchData*);
	  void
	  reply_cb_mbxml (char const*, guint, guint, CoverFetchData*);

      void
      cache_inline (std::string const& mbid, std::string const& uri);

	  RequestKeeperT              RequestKeeper;
	  Glib::Mutex                 RequestKeeperLock;

	  MPixbufCache                m_pixbuf_cache;
	  MSurfaceCache               m_surface_cache[N_COVER_SIZES];
  };
}

#endif
