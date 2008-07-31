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

#include "config.h"
#include "mpx/mpx-minisoup.hh"

#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <set>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h> // bleh!
#include <cairomm/cairomm.h>
#include <sigx/sigx.h>

#include "mpx/mpx-main.hh"
#include "mpx/mpx-covers-stores.hh"
#include "mpx/mpx-network.hh"
#include "mpx/metadatareader-taglib.hh"

namespace MPX
{
  enum CoverSize
  {
    COVER_SIZE_ALBUM_LIST = 0,
    COVER_SIZE_INFO_AREA  = 1,
    COVER_SIZE_DEFAULT    = 2,

    N_COVER_SIZES
  };

  typedef std::map <std::string, int> RequestKeeperT;
  typedef std::map <std::string, Glib::RefPtr<Gdk::Pixbuf> > MPixbufCache;
  typedef std::map <std::string, Cairo::RefPtr<Cairo::ImageSurface> > MSurfaceCache;
  typedef boost::shared_ptr<CoverStore> StoreP;
  typedef std::vector <StoreP> StoresT;

  struct CoverFetchData
  {
      Soup::RequestRefP request;
      std::string asin;
      std::string mbid;
      std::string uri;
      std::string artist;
      std::string album;
      int n;
      StoresT m_req_stores;

      CoverFetchData(
            const std::string& asin_    = std::string(),
            const std::string& mbid_    = std::string(),
            const std::string& uri_     = std::string(),
            const std::string& artist_  = std::string(),
            const std::string& album_   = std::string(), 
            StoresT            stores   = StoresT()
      )
      : asin(asin_)
      , mbid(mbid_)
      , uri(uri_)
      , artist(artist_)
      , album(album_)
      , n(0)
      , m_req_stores(stores) // we COPY intentionally
      {
      }
  };

  class CoverStore;

  class Covers : public sigx::glib_auto_dispatchable
  {
	public:
	  Glib::Mutex                 RequestKeeperLock;
      Glib::Mutex                 StoresLock;

	  typedef sigc::signal<void, const std::string&> SignalGotCover;

	  struct SignalsT
	  {
          // Fired if a store found cover artwork
		  SignalGotCover GotCover;
	  };

	  SignalsT Signals;

	  SignalGotCover&
	  signal_got_cover()
      {
        return Signals.GotCover ;
      }

      typedef bool (Covers::*FetchFunc) (const std::string&, Glib::RefPtr<Gdk::Pixbuf>&);

	  Covers ();

	  bool
	  fetch(
        const std::string&                      /*mbid*/,
        Glib::RefPtr<Gdk::Pixbuf>&              /*cover*/
      );

	  bool
	  fetch(
        const std::string&                      /*mbid*/,
        Cairo::RefPtr<Cairo::ImageSurface>&     /*cover*/,
        CoverSize                               /*size*/
      );

	  void
	  cache(
        const std::string& /*mbid*/,
        const std::string& /*uri*/      = std::string(),
        const std::string& /*asin*/     = std::string(),
        const std::string& /*artist*/   = std::string(),
        const std::string& /*album*/    = std::string(),
        bool               /*acquire*/  = false
      );

      void
      purge();

      void
      cache_artwork(
        const std::string& /* mbid */,
        Glib::RefPtr<Gdk::Pixbuf> /*cover*/
      );

	  std::string
	  get_thumb_path (std::string /*mbid*/);

    private:

      void
      store_not_found_cb (CoverFetchData*);

      void
      source_pref_changed_callback(const std::string& domain, const std::string& key, const Mcs::KeyVariant& value );

      void
      rebuild_stores ();
 
      bool
      cache_inline (const std::string& mbid, const std::string& uri);

	  RequestKeeperT              RequestKeeper;
	  MPixbufCache                m_pixbuf_cache;
	  MSurfaceCache               m_surface_cache[N_COVER_SIZES];
      StoresT                     m_all_stores, m_current_stores;
      int                         m_rebuild;
      int                         m_rebuilt;
  };
}

#endif
