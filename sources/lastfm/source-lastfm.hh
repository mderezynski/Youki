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
//  The MPXx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.

#ifndef MPX_UI_PART_LASTFM_HH
#define MPX_UI_PART_LASTFM_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <string>
#include <sigc++/connection.h>
#include <boost/optional.hpp>

#include <glibmm/ustring.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>
#include <gtkmm/uimanager.h>
#include <libglademm/xml.h>

#include "mpx/mpx-public.hh"
#include "mpx/i-playbacksource.hh"

#include "lastfm.hh"

using namespace Glib;

namespace MPX
{
namespace Source
{
    class LastFM
      : public PlaybackSource
    {
      public:

        LastFM (const Glib::RefPtr<Gtk::UIManager>&, MPX::Player&);
        virtual ~LastFM () {}
  
      private:

		Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;
		Glib::RefPtr<Gtk::ActionGroup>  m_actions;
		Glib::RefPtr<Gtk::UIManager>    m_ui_manager;
		Glib::RefPtr<Gtk::UIManager>    m_ui_manager_main;
		Gtk::Widget					  * m_UI;
        Gtk::Entry                    * m_URL_Entry;
        Gtk::ComboBox                 * m_CBox_Sel;
    
        MPX::LastFMRadio                m_LastFMRadio;
        boost::optional<XSPF::Playlist> m_Playlist;
        XSPF::ItemIter                  m_PlaylistIter;
        Glib::Mutex                     m_PlaylistLock;

        MPX::Player                   & m_Player;

        void
        on_url_entry_activated ();

        void
        on_playlist (XSPF::Playlist const&);

        void
        on_no_playlist ();

        void
        on_tuned ();

        void
        on_tune_error (Glib::ustring const&);

      protected:

	    virtual Glib::RefPtr<Gdk::Pixbuf>
		get_icon ();

	    virtual Gtk::Widget*
		get_ui ();

        virtual std::string
        get_uri ();

        virtual bool
        go_next ();

        virtual void
        go_next_async ();

        virtual bool
        go_prev ();

        virtual void
        go_prev_async ();

        virtual void
        stop ();

        virtual bool
        play ();

        virtual void
        play_async ();

        virtual void
        play_post ();

        virtual void
        next_post ();

        virtual void
        restore_context ();

		virtual void
		send_metadata ();
    };
  }
}
#endif // !MPX_UI_PART_LASTFM
