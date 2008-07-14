//
// component-top-albums
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
#ifndef MPX_COMPONENT_TOP_ALBUMS_HH
#define MPX_COMPONENT_TOP_ALBUMS_HH
#include "config.h"
#include <glibmm.h>
#include <gtkmm.h>
#include <sigx/sigx.h>
#include <set>
#include <string>

#include "mpx/library.hh"
#include "mpx/paccess.hh"

#include "component-base.hh"
#include "component-slot-artist.hh"

#include "top-albums-fetch-thread.hh"

namespace MPX
{
	class ComponentTopAlbums : public ComponentBase
	{
            typedef std::set<std::string> Artists_T;

        public:
            ComponentTopAlbums (PAccess<MPX::Library> &);
            virtual ~ComponentTopAlbums ();

        protected:

            struct Columns_t : public Gtk::TreeModelColumnRecord
            {
                Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > Pixbuf;
                Gtk::TreeModelColumn<Glib::ustring>              Text;

                Columns_t ()
                {
                    add(Pixbuf);
                    add(Text);
                };
            };

            void
            on_entry_activated ();

            void
            on_entry_changed ();

            void
            on_new_album (Glib::RefPtr<Gdk::Pixbuf>, std::string const&);

            void
            on_stopped ();

            void
            setup_view ();

            std::string
            find_nearest_artist (std::string const&);

            Columns_t                         Columns;

            Gtk::Entry                      * m_Entry;
            Gtk::Label                      * m_Label;
            Gtk::Label                      * m_LabelSuggest;
            Gtk::TreeView                   * m_View;

            Glib::RefPtr<Gtk::ListStore>      m_Model;

            TopAlbumsFetchThread            * m_Thread;

            Artists_T                         m_Artists;

            PAccess<MPX::Library>             m_Library;
	};
}

#endif
