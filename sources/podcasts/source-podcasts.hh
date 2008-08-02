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

#ifndef MPX_UI_PART_PODCASTS_HH
#define MPX_UI_PART_PODCASTS_HH

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
#include "mpx/mpx-public-mpx.hh"
#include "mpx/i-playbacksource.hh"

namespace MPX
{
    class PodcastsPrivate;

namespace Source
{
    class Podcasts
      : public PlaybackSource
    {
      public:

        Podcasts (const Glib::RefPtr<Gtk::UIManager>&, MPX::Player&);
        virtual ~Podcasts () {}

      private:

        Gtk::Widget                       * m_UI;
        Glib::RefPtr<Gnome::Glade::Xml>     m_ref_xml;
        Glib::RefPtr<Gtk::UIManager>        m_MainUIManager;
        Glib::RefPtr<Gtk::ActionGroup>      m_MainActionGroup;
        PodcastsPrivate                   * m_Private;

        void
        add_podcast ();
  
      protected:

        virtual std::string
        get_guid ();

        virtual std::string
        get_class_guid ();

	    virtual Glib::RefPtr<Gdk::Pixbuf>
		get_icon ();

	    virtual Gtk::Widget*
		get_ui ();

        virtual guint
        add_menu ();

        virtual std::string
        get_uri ();

        virtual bool
        go_next ();

        virtual bool
        go_prev ();

        virtual void
        stop ();

        virtual bool
        play ();

        virtual void
        play_post ();

        virtual void
        restore_context ();

		virtual void
		send_metadata ();
    };
  }
}
#endif // !MPX_UI_PART_PODCASTS
