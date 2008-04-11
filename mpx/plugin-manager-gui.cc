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
#include "config.h"
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <libglademm/xml.h>
#include <boost/python.hpp>
#include <boost/format.hpp>
#include "plugin.hh"
#include "plugin-manager-gui.hh"
#include "mpx/main.hh"

using namespace Glib;
using namespace Gtk;

namespace MPX
{
	class PTV
		: public WidgetLoader<Gtk::TreeView>
	{
			class ColumnsT : public Gtk::TreeModelColumnRecord
			{
				public:

					Gtk::TreeModelColumn<bool>				Active;
					Gtk::TreeModelColumn<Glib::ustring>		NameDesc;
					Gtk::TreeModelColumn<Glib::ustring>		Authors;
					Gtk::TreeModelColumn<Glib::ustring>		Copyright;
					Gtk::TreeModelColumn<Glib::ustring>		Website;
					Gtk::TreeModelColumn<gint64>			Id;
					Gtk::TreeModelColumn<Glib::ustring>		Name;
	
				ColumnsT ()
				{
					add(Active);
					add(NameDesc);
					add(Authors);
					add(Copyright);
					add(Website);
					add(Id);
					add(Name);
				};
			};
	
			Glib::RefPtr<Gtk::ListStore> Store;
			ColumnsT Columns;

			PluginManager & m_Manager;

		public:

			PTV (const Glib::RefPtr<Gnome::Glade::Xml> &xml, PluginManager & manager)
			: WidgetLoader<Gtk::TreeView>(xml, "treeview")
			, m_Manager(manager)
			{
				Store = Gtk::ListStore::create(Columns);

				append_column_editable(_("Active"), Columns.Active);

				Gtk::CellRendererText * cell = manage(new Gtk::CellRendererText);
				Gtk::TreeViewColumn * col = manage(new Gtk::TreeViewColumn(_("Name/Description")));
				col->pack_start(*cell);
				col->add_attribute(*cell, "markup", Columns.NameDesc);
				append_column(*col);
		
				append_column(_("Authors"), Columns.Authors);
				append_column(_("Copyright"), Columns.Copyright);
				append_column(_("Website"), Columns.Website);

				std::vector<Gtk::CellRenderer*> renderers = get_column(0)->get_cell_renderers();
				renderers[0]->property_sensitive () = true;
				Gtk::CellRendererToggle * cell_toggle =
					dynamic_cast<Gtk::CellRendererToggle*>(renderers[0]);

				cell_toggle->signal_toggled().connect( sigc::mem_fun( *this, &PTV::on_cell_toggled ) );

				PluginHoldMap const& map = m_Manager.get_map();	

				for(PluginHoldMap::const_iterator i = map.begin(); i != map.end(); ++i)
				{
					TreeIter iter = Store->append();
					(*iter)[Columns.Name] = i->second->get_name();
					(*iter)[Columns.NameDesc] = (boost::format("<b><big>%1%</big></b>\n%2%") % i->second->get_name() % i->second->get_desc()).str();
					(*iter)[Columns.Authors] = i->second->get_authors();
					(*iter)[Columns.Copyright] = i->second->get_copyright();
					(*iter)[Columns.Website] = i->second->get_website();
					(*iter)[Columns.Active] = i->second->get_active();
					(*iter)[Columns.Id] = i->second->get_id();
				}

				set_model (Store);
			}

			void
			on_cell_toggled (const Glib::ustring& p_string)
			{
				bool result = false;
				TreePath path (p_string);
				TreeIter iter = Store->get_iter(path);
				gint64 id = (*iter)[Columns.Id];
				bool active = (*iter)[Columns.Active];

				if(active)
					m_Manager.deactivate(id);
				else
					result = m_Manager.activate(id);

				(*iter)[Columns.Active] = result;
			}

			~PTV ()
			{
			}
	};
}

namespace MPX
{
		PluginManagerGUI::~PluginManagerGUI ()
		{
			delete m_PTV;
		}

		PluginManagerGUI::PluginManagerGUI (const Glib::RefPtr<Gnome::Glade::Xml> &xml, PluginManager &obj_manager)
	    : WidgetLoader<Gtk::Window>(xml, "window")
		, m_Manager(obj_manager)
		, m_PTV(new PTV(xml, obj_manager))
		{
			m_PTV->get_model()->signal_row_changed().connect( sigc::mem_fun( *this, &PluginManagerGUI::on_row_changed ) );

			xml->get_widget("label", label);

			if(obj_manager.get_traceback_count())
			{
				set_error_text();
				buTraceback->set_sensitive();
			}


			xml->get_widget("traceback", buTraceback);
			buTraceback->signal_clicked().connect( sigc::mem_fun( *this, &PluginManagerGUI::show_dialog ) );

			Gtk::Button * buClose;
			xml->get_widget("close", buClose);
			buClose->signal_clicked().connect( sigc::mem_fun( *this, &Gtk::Widget::hide ) );
		}

		PluginManagerGUI*
		PluginManagerGUI::create (PluginManager &obj_manager)
		{
			const std::string path (build_filename(DATA_DIR, build_filename("glade","plugin-manager.glade")));
			PluginManagerGUI * p = new PluginManagerGUI(Gnome::Glade::Xml::create (path), obj_manager); 
			return p;
		}

	
		bool
		PluginManagerGUI::on_delete_event (GdkEventAny* G_GNUC_UNUSED)
		{
			Gtk::Window::hide ();
			return true;
		}

		void
		PluginManagerGUI::on_row_changed(const Gtk::TreeModel::Path& /*path*/, const Gtk::TreeModel::iterator& /*iter*/)
		{
			if(m_Manager.get_traceback_count())
			{
				set_error_text();
				buTraceback->set_sensitive();
			}
			else
			{
				label->set_markup(" ");
				buTraceback->set_sensitive(false);
			}
		}

		void
		PluginManagerGUI::show_dialog()
		{
			Gtk::MessageDialog dialog ("Failed to activate: " + m_Manager.get_last_traceback().get_name(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
			dialog.set_title ("Plugin traceback - MPX");
			dialog.set_secondary_text (m_Manager.pull_last_traceback().get_traceback());
			dialog.run();

			if(m_Manager.get_traceback_count())
				set_error_text();
			else
			{
				label->set_markup(" ");
				buTraceback->set_sensitive(false);
			}
		}
		
		void
		PluginManagerGUI::set_error_text()
		{
			Glib::ustring text = "<b>Failed to activate: " + m_Manager.get_last_traceback().get_name() + "</b> ";

			unsigned int n = m_Manager.get_traceback_count();
			if(n > 1)
				text += Glib::ustring::compose("(%1 more errors)", n-1);

			label->set_markup(text);
		}

}
