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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include "mpx/mpx-library.hh"

#include "source-radio.hh"

#include <gtkmm.h>
#include <gtk/gtk.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <libglademm.h>
#include <map>

#include "mpx/mpx-public-mpx.hh"
#include "mpx/mpx-types.hh"
#include "mpx/mpx-minisoup.hh"

#include "streams-shoutcast.hh"
#include "streams-icecast.hh"

#include "mpx/playlistparser-pls.hh"

using namespace Glib;
using namespace Gtk;

namespace
{
    enum Page
    {
      PAGE_SHOUTCAST,
      PAGE_ICECAST,
    };

    const char RADIO_ACTION_UPDATE_XIPH[] = "radio-action-update-xiph";
    const char RADIO_ACTION_UPDATE_SHOUT[] = "radio-action-update-shout";


    char const * ui_source =
    "<ui>"
    ""
    "<menubar name='MenubarMain'>"
    "   <placeholder name='placeholder-source'>"
    "     <menu action='menu-source-radio'>"
    "         <menuitem action='radio-action-update-xiph'/>"
    "         <menuitem action='radio-action-update-shout'/>"
    "     </menu>"
    "   </placeholder>"
    "</menubar>"
    ""
    "</ui>"
    "";
}

namespace MPX
{
namespace Source
{
    Glib::RefPtr<Gdk::Pixbuf>
    Radio::get_icon ()
    {
        try{
            return IconTheme::get_default()->load_icon("source-radio", 64, ICON_LOOKUP_NO_SVG);
        } catch(...) { return Glib::RefPtr<Gdk::Pixbuf>(0); }
    }

    Gtk::Widget*
    Radio::get_ui ()
    {
        return m_UI;
    }

    guint
    Radio::add_menu ()
    {
        return m_ui_manager->add_ui_from_string(ui_source);
    }

    Radio::Radio (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player & player)
    : PlaybackSource  (ui_manager, _("Radio"), C_NONE, F_ASYNC)
    {
		const std::string path (build_filename(DATA_DIR, build_filename("glade","source-radio.glade")));
		m_ref_xml = Gnome::Glade::Xml::create (path);
		m_UI = m_ref_xml->get_widget("source-radio");

		m_ref_xml->get_widget         ("notebook-radio",              m_notebook_radio);
		m_ref_xml->get_widget         ("notebook-shoutcast-streams",  m_notebook_shoutcast);
		m_ref_xml->get_widget         ("notebook-icecast",            m_notebook_icecast);
		m_ref_xml->get_widget_derived ("shoutcast-genres",            m_shoutcast_base);
		m_ref_xml->get_widget_derived ("shoutcast-streams",           m_shoutcast_list);
		m_ref_xml->get_widget_derived ("icecast-streams",             m_icecast_list);

		m_filter_entry = manage (new Gtk::Entry());
		m_filter_entry->show();

		Label * label = manage (new Label());
		label->set_markup_with_mnemonic (_("_Search:"));
		label->set_mnemonic_widget (*m_filter_entry);
		label->show();

		m_filter_entry->signal_changed().connect (
		  sigc::mem_fun (*this, &Radio::on_filter_changed));
		 
		dynamic_cast<Gtk::HBox*> (m_ref_xml->get_widget ("radio-hbox-filter"))->pack_start (*label, false, false);
		dynamic_cast<Gtk::HBox*> (m_ref_xml->get_widget ("radio-hbox-filter"))->pack_start (*m_filter_entry, true, true);


		try{
			dynamic_cast<Gtk::Image *> (m_ref_xml->get_widget ("throbber-icecast"))->set (build_filename(build_filename (DATA_DIR,"images"),"throbber.gif"));
			dynamic_cast<Gtk::Image *> (m_ref_xml->get_widget ("throbber-shoutcast"))->set (build_filename(build_filename (DATA_DIR,"images"),"throbber.gif"));
		} catch (Gdk::PixbufError & cxe) 
		{
			g_message("PixbufError: %s", cxe.what().c_str());
		}

		m_shoutcast_base->signal_start().connect
		  (sigc::bind (sigc::mem_fun (*m_notebook_shoutcast, &Gtk::Notebook::set_current_page), 1));
		m_shoutcast_base->signal_stop().connect
		  (sigc::bind (sigc::mem_fun (*m_notebook_shoutcast, &Gtk::Notebook::set_current_page), 0));
		m_shoutcast_base->signal_start().connect
		  (sigc::bind (sigc::mem_fun (*m_shoutcast_base, &Gtk::Widget::set_sensitive), false));
		m_shoutcast_base->signal_stop().connect
		  (sigc::bind (sigc::mem_fun (*m_shoutcast_base, &Gtk::Widget::set_sensitive), true));
		m_shoutcast_base->signal_list_updated().connect
		  (sigc::mem_fun (*this, &Radio::on_shoutcast_list_updated));
		m_shoutcast_list->signal_row_activated().connect
		  (sigc::mem_fun (*this, &Radio::on_row_activated));
		m_shoutcast_list->get_selection()->signal_changed().connect
		  (sigc::bind (sigc::mem_fun (*this, &Radio::on_selection_changed), dynamic_cast<RadioDirectory::ViewBase*>(m_shoutcast_list) ) );

		m_icecast_list->signal_start().connect
		  (sigc::bind (sigc::mem_fun (*m_icecast_list, &Gtk::Widget::set_sensitive), false));
		m_icecast_list->signal_stop().connect
		  (sigc::bind (sigc::mem_fun (*m_icecast_list, &Gtk::Widget::set_sensitive), true));
		m_icecast_list->signal_stop().connect
		  (sigc::mem_fun (*this, &Radio::on_icecast_list_updated));
		m_icecast_list->signal_row_activated().connect
		  (sigc::mem_fun (*this, &Radio::on_row_activated));
		m_icecast_list->get_selection()->signal_changed().connect
		  (sigc::bind (sigc::mem_fun (*this, &Radio::on_selection_changed), dynamic_cast<RadioDirectory::ViewBase*>(m_icecast_list) ) );

		m_ui_manager = ui_manager; 

		m_actions = ActionGroup::create ("Actions_Radio");

		m_actions->add (Action::create(
                            "menu-source-radio",
                            _("Radio")
        ));

		m_actions->add (Action::create(
                            RADIO_ACTION_UPDATE_XIPH,
							Gtk::Stock::REFRESH,
							_("Icecast: Update List")),
							sigc::mem_fun(
                                *m_icecast_list,
                                &RadioDirectory::Icecast::refresh
        ));

		m_actions->add (Action::create(
                            RADIO_ACTION_UPDATE_SHOUT,
							Gtk::Stock::REFRESH,
							_("SHOUTcast: Update List")),
							sigc::mem_fun(
                                *m_shoutcast_base,
                                &RadioDirectory::Shoutcast::refresh
        ));


		m_ui_manager->insert_action_group (m_actions);

        m_notebook_radio->signal_switch_page().connect(
            sigc::hide(
                sigc::hide(
                    sigc::mem_fun(
                        *this,
                        &Radio::on_set_notebook_page
        ))));

		on_set_notebook_page ();
    }

    std::string
    Radio::get_guid ()
    {
        return "724e3e0c-3ea9-43b5-a039-aaf8034f131f";
    }

    std::string
    Radio::get_class_guid ()
    {
        return "d0bc877c-376a-46d9-a196-3710147fc44a";
    }

    void
    Radio::on_set_notebook_page ()
    {
		int page = m_notebook_radio->get_current_page(); 

		RadioDirectory::ViewBase * p = (page == PAGE_SHOUTCAST) ? m_shoutcast_list : m_icecast_list;
		p->set_filter(m_filter_entry->get_text());

		if( p->get_selection ()->count_selected_rows() ) 
			  m_Caps = Caps (m_Caps |  C_CAN_PLAY);
		else
			  m_Caps = Caps (m_Caps & ~C_CAN_PLAY);
		Signals.Caps.emit (m_Caps);
    }

    void
    Radio::on_row_activated (Gtk::TreeModel::Path const& path,
                             Gtk::TreeViewColumn       * column)
    {
		m_current_uri = ustring ();
		Signals.PlayRequest.emit ();
    }

    void
    Radio::on_filter_changed ()
    {
		RadioDirectory::ViewBase * p = (m_notebook_radio->get_current_page() == PAGE_SHOUTCAST) ? m_shoutcast_list : m_icecast_list;
		p->set_filter (m_filter_entry->get_text());
    }

    void
    Radio::on_selection_changed (RadioDirectory::ViewBase * view)
    {
		if( view->get_selection ()->count_selected_rows() == 1 )
			  m_Caps = Caps (m_Caps |  C_CAN_PLAY);
		else
			  m_Caps = Caps (m_Caps & ~C_CAN_PLAY);
		Signals.Caps.emit (m_Caps);
    }

    void
    Radio::on_icecast_list_updated ()
    {
		m_icecast_list->set_filter (m_filter_entry->get_text());
		m_notebook_icecast->set_current_page (0);
    }

    void
    Radio::on_shoutcast_list_updated (RadioDirectory::StreamListT const& list)
    {
		m_shoutcast_list->set_stream_list (list);
		m_shoutcast_list->set_filter (m_filter_entry->get_text());
		m_shoutcast_base->queue_draw ();
		m_shoutcast_base->set_sensitive (1);
		m_notebook_shoutcast->set_current_page (0);
        m_filter_entry->grab_focus();
        m_filter_entry->select_region(0,-1);
    }

    std::string
    Radio::get_uri ()
    {
		return m_current_uri;
    }

    bool
    Radio::go_next ()
    {
		return false;
    }

    bool
    Radio::go_prev ()
    {
		return false;
    }

    void
    Radio::stop ()
    {
        if(m_request)
        {
            m_request->cancel();
        }

		if ( !m_shoutcast_list->get_selection()->count_selected_rows() && !m_icecast_list->get_selection()->count_selected_rows())
		{
		  m_Caps = Caps (m_Caps & ~C_CAN_PLAY);
		}

		m_current_uri = ustring();
		m_current_name = ustring();
		m_Caps = Caps (m_Caps & ~C_CAN_PAUSE);
		Signals.Caps.emit (m_Caps);
    }

    void
    Radio::playlist_cb (char const * data, guint size, guint code)
    {
        if (code == 200)
        {
            Track_v v;

            PlaylistParser::PLS * parse = new PlaylistParser::PLS; 
            parse->read(std::string(data, size), v, false);

            if(v.size())
            {
                using boost::get;

                m_current_uri = get<std::string>(v[0][ATTRIBUTE_LOCATION].get()); //FIXME: We would want to get the other URIs listed as well..
                Signals.PlayAsync.emit();
                return;
            }
        }

        m_current_uri = "";
        m_current_name = "";
        Signals.StopRequest.emit();
    }

    void
    Radio::play_async ()
    {
		if( m_current_uri.empty() )
		{
		  int page = m_notebook_radio->get_current_page ();

		  if( page == PAGE_SHOUTCAST )
		  {
				ustring uri;
				m_shoutcast_list->get(m_current_name, uri);
				m_request = Soup::Request::create(uri);
                m_request->request_callback().connect(
                    sigc::mem_fun(
                        *this,
                        &Radio::playlist_cb
                ));
                m_request->run();
		  }
		  else if( page == PAGE_ICECAST )
		  {
				m_icecast_list->get(m_current_name, m_current_uri);
		  }
		}
    }

    bool
    Radio::play ()
    {
		return false;
    }

	void
	Radio::send_metadata ()
	{
		Metadata metadata;
		metadata[ATTRIBUTE_ARTIST] = m_current_name; 
		metadata[ATTRIBUTE_TITLE]  = m_current_uri;
		Signals.Metadata.emit (metadata);
	}

    void
    Radio::play_post ()
    {
		send_metadata ();
		m_Caps = Caps (m_Caps | C_CAN_PAUSE);
		send_caps ();
    }

    void
    Radio::restore_context ()
    {
    }

  } // Source
} // MPX
