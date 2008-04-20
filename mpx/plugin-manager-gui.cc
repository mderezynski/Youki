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
					Gtk::TreeModelColumn<bool>				HasGUI;
					Gtk::TreeModelColumn<std::string>		Desc;
					Gtk::TreeModelColumn<std::string>		Authors;
					Gtk::TreeModelColumn<std::string>		Copyright;
					Gtk::TreeModelColumn<std::string>		Website;
					Gtk::TreeModelColumn<gint64>			Id;
					Gtk::TreeModelColumn<std::string>		Name;
	
				ColumnsT ()
				{
					add(Active);
					add(HasGUI);
					add(Desc);
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
				append_column(_("Name"), Columns.Name);

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
					(*iter)[Columns.Desc] = i->second->get_desc();
					(*iter)[Columns.Authors] = i->second->get_authors();
					(*iter)[Columns.Copyright] = i->second->get_copyright();
					(*iter)[Columns.Website] = i->second->get_website();
					(*iter)[Columns.Active] = i->second->get_active();
					(*iter)[Columns.HasGUI] = i->second->get_has_gui();
					(*iter)[Columns.Id] = i->second->get_id();

				}

				set_model (Store);
				signal_row_activated().connect( sigc::mem_fun( *this, &PTV::on_row_activated ) );
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

			Gtk::Widget *
			get_gui (const TreeModel::TreeModel::iterator& iter)
			{
				gint64 id = (*iter)[Columns.Id];

				if((*iter)[Columns.HasGUI])
					return m_Manager.get_gui(id);
				else
					return NULL;
			}

			bool
			is_active(const TreeModel::iterator& iter) const
			{
				return (*iter)[Columns.Active];
			}

			bool
			get_has_gui(const TreeModel::iterator& iter) const
			{
				return (*iter)[Columns.HasGUI];
			}

			const std::string
			get_desc(const TreeModel::iterator& iter) const
			{
				return (*iter)[Columns.Desc];
			}

			const std::string
			get_authors(const TreeModel::iterator& iter) const
			{
				return (*iter)[Columns.Authors];
			}

			const std::string
			get_copyright(const TreeModel::iterator& iter) const
			{
				return (*iter)[Columns.Copyright];
			}

			const std::string
			get_website(const TreeModel::iterator& iter) const
			{
				return (*iter)[Columns.Website];
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
			m_PTV->get_selection()->signal_changed().connect( sigc::mem_fun( *this, &PluginManagerGUI::on_selection_changed ) );
			m_PTV->get_model()->signal_row_changed().connect( sigc::mem_fun( *this, &PluginManagerGUI::on_row_changed ) );

			xml->get_widget("notebook", notebook);
			notebook->get_nth_page(1)->hide();

			xml->get_widget("overview", overview);
			xml->get_widget("options", options);
			xml->get_widget("error", error);
			xml->get_widget("traceback", buTraceback);
			buTraceback->signal_clicked().connect( sigc::mem_fun( *this, &PluginManagerGUI::show_dialog ) );

			if(obj_manager.get_traceback_count())
			{
				set_error_text();
				buTraceback->set_sensitive();
			}

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
		PluginManagerGUI::on_selection_changed()
		{
			Glib::RefPtr<TreeSelection> sel = m_PTV->get_selection();
			TreeModel::iterator iter = sel->get_selected();
			overview->set_markup( (boost::format(_("%1%\n\n<b>Authors:</b> %2%\n<b>Copyright:</b> %3%\n<b>Website:</b> %4%"))
										% Glib::Markup::escape_text(m_PTV->get_desc(iter)).c_str()
										% Glib::Markup::escape_text(m_PTV->get_authors(iter)).c_str()
										% Glib::Markup::escape_text(m_PTV->get_copyright(iter)).c_str()
										% Glib::Markup::escape_text(m_PTV->get_website(iter)).c_str() ).str() );

			if(m_PTV->get_has_gui(iter))
			{
				notebook->get_nth_page(1)->show();
				std::list<Gtk::Widget*> list = options->get_children();
				if(list.size())
				{
					Gtk::Widget * old_widget = *(list.begin());
					options->remove(*old_widget);
				}
				Gtk::Widget * widget = m_PTV->get_gui(iter);
				if(widget != NULL)
				{
					options->pack_start(*widget, Gtk::PACK_SHRINK);
					widget->show();
				}
			}
			else
				notebook->get_nth_page(1)->hide();
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
				error->set_markup(" ");
				buTraceback->set_sensitive(false);
			}
		}

		void
		PluginManagerGUI::show_dialog()
		{
			Gtk::MessageDialog dialog ((boost::format(_("Failed to %1%: %2%")) % m_Manager.get_last_traceback().get_method() % m_Manager.get_last_traceback().get_name()).str(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
			dialog.set_title (_("Plugin traceback - MPX"));
			dialog.set_secondary_text (m_Manager.pull_last_traceback().get_traceback());
			dialog.run();

			if(m_Manager.get_traceback_count())
				set_error_text();
			else
			{
				error->set_markup(" ");
				buTraceback->set_sensitive(false);
			}
		}

		void
		PluginManagerGUI::set_error_text()
		{
			std::string text = (boost::format(_("<b>Failed to %1%: %2%</b>")) % m_Manager.get_last_traceback().get_method() % m_Manager.get_last_traceback().get_name()).str();

			unsigned int n = m_Manager.get_traceback_count();
			if(n > 1)
				text += (boost::format(_(" (%d more errors)")) % (n-1)).str();

			error->set_markup(text);
		}

}
