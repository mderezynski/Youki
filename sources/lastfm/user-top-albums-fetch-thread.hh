//
// top-albums-fetch-thread
//
// Authors:
//     Milosz Derezynski <milosz@backtrace.info>
//
// (C) 2008 MPX Project
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
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
#ifndef MPX_TOP_ALBUMS_FETCH_THREAD_HH
#define MPX_TOP_ALBUMS_FETCH_THREAD_HH
#include <glibmm.h>
#include <gdkmm/pixbuf.h>
#include <sigx/sigx.h>

namespace MPX
{
	class UserTopAlbumsFetchThread : public sigx::glib_threadable
	{
		public:
			UserTopAlbumsFetchThread ();
			~UserTopAlbumsFetchThread ();

            typedef sigx::request_f<std::string const&> RequestLoad_t;
            typedef sigx::request_f<>                   RequestStop_t;

            typedef sigc::signal< void, Glib::RefPtr<Gdk::Pixbuf>, std::string const& > SignalAlbum_t;
            typedef sigx::signal_f< SignalAlbum_t >                                     SignalAlbum_x;

            RequestLoad_t           RequestLoad;
            RequestStop_t           RequestStop;
            SignalAlbum_x           SignalAlbum;

        protected:

            virtual void on_startup ();
            virtual void on_cleanup ();
   
            void 
            on_load(std::string const&);
    
            void
            on_stop();
        
            struct ThreadData;
            Glib::Private<ThreadData> m_ThreadData;
	};
}

#endif
