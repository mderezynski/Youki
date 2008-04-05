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

#include <gtkmm.h>
#include <gtk/gtk.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <libglademm.h>
#include <map>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

#include "mpx/mpx-public.hh"
#include "mpx/library.hh"
#include "mpx/types.hh"
#include "mpx/minisoup.hh"
#include "streams-shoutcast.hh"
#include "streams-icecast.hh"
#include "source-radio.hh"

using namespace Glib;
using namespace Gtk;

#define RADIO_ACTION_SHOW_ICECAST   "radio-action-show-icecast"
#define RADIO_ACTION_SHOW_SHOUTCAST "radio-action-show-shoutcast"
#define RADIO_ACTION_UPDATE_LIST    "radio-action-update-list"

namespace
{
  enum Page
  {
    PAGE_SHOUTCAST,
    PAGE_ICECAST,
  };
}

namespace
{
  using namespace MPX;
  using namespace std;

  typedef map < string, string > StringMap;

  void
  parse_to_map (StringMap& map, string const& buffer)
  {
    using boost::algorithm::split;
    using boost::algorithm::split_regex;
    using boost::algorithm::is_any_of;

    //vector<string> lines;
    //split_regex (lines, buffer, boost::regex ("\\\r?\\\n"));
	char **lines = g_strsplit(buffer.c_str(), "\n", -1);

    //for (unsigned int n = 0; n < lines.size(); ++n)
    for (int n = 0; lines[n] != NULL; ++n)
    {
      char **line = g_strsplit (lines[n], "=", 2);
      if (line[0] && line[1] && strlen(line[0]) && strlen(line[1]))
      {
        ustring key (line[0]);
        map[std::string (key.lowercase())] = line[1];
      }
      g_strfreev (line);
    }
    g_strfreev (lines);
  }

  void 
  parse_pls (std::string const& buffer, VUri & list)
  {
	StringMap map;
	parse_to_map (map, buffer);

	if (map.empty()) 
		return; 

	if (map.find("numberofentries") == map.end())
		return;

	unsigned int n = strtol (map.find("numberofentries")->second.c_str(), NULL, 10);
	static boost::format File ("file%d");
	for (unsigned int a = 1; a <= n ; ++a)
	{
	  StringMap::iterator const& i = map.find ((File % a).str());
	  if (i != map.end())
	  {
		list.push_back (i->second);
	  }
	}

	if (list.empty()) 
		return; 

	if (list.size() != n)
		return;
  }
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

    Radio::Radio (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player & player)
    : PlaybackSource  (ui_manager, _("Radio"))
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

		m_ui_manager = Gtk::UIManager::create();
		m_actions = ActionGroup::create ("Actions_Radio");
		m_actions->add (Action::create ("MenuUiPartRadio", _("Radio")));

		m_actions->add  (Action::create (RADIO_ACTION_UPDATE_LIST,
										  Gtk::Stock::REFRESH,
										  _("Update Icecast List")),
										  AccelKey("<control>r"),
										  (sigc::mem_fun (*this, &Radio::on_update_list)));

		Gtk::RadioButtonGroup gr1;
		m_actions->add  (RadioAction::create (gr1, RADIO_ACTION_SHOW_SHOUTCAST,
											  "Shoutcast"),
											  (sigc::mem_fun (*this, &Radio::on_set_notebook_page)));

		m_actions->add  (RadioAction::create (gr1, RADIO_ACTION_SHOW_ICECAST,
											  _("Icecast")),
											  (sigc::mem_fun (*this, &Radio::on_set_notebook_page)));

		RefPtr<Gtk::RadioAction>::cast_static (m_actions->get_action (RADIO_ACTION_SHOW_SHOUTCAST))->property_value() = 0;
		RefPtr<Gtk::RadioAction>::cast_static (m_actions->get_action (RADIO_ACTION_SHOW_ICECAST))->property_value() = 1;

		m_actions->get_action (RADIO_ACTION_SHOW_SHOUTCAST)->connect_proxy
			  (*(dynamic_cast<ToggleToolButton *>(m_ref_xml->get_widget ("radio-tooltb-shoutcast"))));

		m_actions->get_action (RADIO_ACTION_SHOW_ICECAST)->connect_proxy
			  (*(dynamic_cast<ToggleToolButton *>(m_ref_xml->get_widget ("radio-tooltb-icecast"))));

		m_ui_manager->insert_action_group (m_actions);

		on_set_notebook_page ();
    }

    void
    Radio::on_set_notebook_page ()
    {
		int page = RefPtr<Gtk::RadioAction>::cast_static (m_actions->get_action (RADIO_ACTION_SHOW_SHOUTCAST))->get_current_value();
		m_notebook_radio->set_current_page (page);

		RadioDirectory::ViewBase * p = (page == PAGE_SHOUTCAST) ? m_shoutcast_list : m_icecast_list;
		p->set_filter (m_filter_entry->get_text());

		if( p->get_selection ()->count_selected_rows() ) 
			  m_Caps = Caps (m_Caps |  PlaybackSource::C_CAN_PLAY);
		else
			  m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_PLAY);
		Signals.Caps.emit (m_Caps);
    }

    void
    Radio::on_update_list ()
    {
		if( m_notebook_radio->get_current_page() == 1)
		{
		  m_notebook_icecast->set_current_page(1);
		  m_icecast_list->refresh();
		}
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
			  m_Caps = Caps (m_Caps |  PlaybackSource::C_CAN_PLAY);
		else
			  m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_PLAY);
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
		if ( !m_shoutcast_list->get_selection()->count_selected_rows() && !m_icecast_list->get_selection()->count_selected_rows())
		{
		  m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_PLAY);
		}

		m_current_uri = ustring();
		m_current_name = ustring();
		m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_PAUSE);
		Signals.Caps.emit (m_Caps);
    }

    bool
    Radio::play ()
    {
		if( m_current_uri.empty() )
		{
		  int page = m_notebook_radio->get_current_page ();
		  if( page == PAGE_SHOUTCAST )
		  {
				ustring uri;
				m_shoutcast_list->get (m_current_name, uri);
				Soup::RequestSyncRefP req = Soup::RequestSync::create(uri);
				if( req->run () == 200)
				{
					std::string data;
					req->get_data (data);
					VUri list;
					parse_pls(data, list);

					if(list.size())
						m_current_uri = list[0]; //FIXME: We would want to get the other URIs listed as well..
					else
						return false;
				}
				else
					return false;
		  }

		  if( page == PAGE_ICECAST )
		  {
				m_icecast_list->get (m_current_name, m_current_uri);
		  }
		}

		return true;
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
		m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_PAUSE);
		send_caps ();
    }

    void
    Radio::restore_context ()
    {
    }

  } // Source
} // MPX
