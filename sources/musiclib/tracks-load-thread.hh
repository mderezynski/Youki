//
// tracks-load-thread 
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
#ifndef MPX_TOP_USER_ALBUMS_FETCH_THREAD_HH
#define MPX_TOP_USER_ALBUMS_FETCH_THREAD_HH
#include <glibmm.h>
#include <gtkmm/liststore.h>
#include <gdkmm/pixbuf.h>
#include <sigx/sigx.h>
#include "mpx/paccess.hh"
#include "mpx/library.hh"

namespace MPX
{
	class TracksLoadThread : public sigx::glib_threadable
	{
		public:

			TracksLoadThread (Glib::RefPtr<Gtk::TreeStore> const&, Glib::Mutex &, PAccess<MPX::Library> &);
			~TracksLoadThread ();

            typedef sigx::request_f<>                       Request_t;
            typedef sigc::signal< void, double >            SignalPercentage_t;
            typedef sigc::signal< void, Gtk::TreeIter&, MPX::SQL::Row&> SignalRow_t;

            typedef sigx::signal_f< SignalPercentage_t >    SignalPercentage_x;
            typedef sigx::signal_f< SignalRow_t >           SignalRow_x;

            Request_t               Request;
            SignalPercentage_x      SignalPercentage;
            SignalRow_x             SignalRow;

        protected:

            virtual void on_startup ();
            virtual void on_cleanup ();
   
            void 
            on_request();
    
            bool
            idle_loader ();

            struct ThreadData;
            Glib::Private<ThreadData> m_ThreadData;


            Glib::RefPtr<Gtk::TreeStore>        Store;
            Glib::Mutex                     &   Lock;
            PAccess<MPX::Library>           &   Lib;
	};
}

#endif
