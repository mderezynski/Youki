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
#include "plugin-manager.hh"
#include "plugin-manager-gui.hh"
#include "mpx/main.hh"

using namespace Glib;
using namespace Gtk;

namespace MPX
{
	class PluginPTV
		: public WidgetLoader<Gtk::TreeView>
	{
			friend class PlaylistPluginManagerGUI;

			class ColumnsT : public Gtk::TreeModelColumnRecord
			{
				public:

					Gtk::TreeModelColumn<Glib::ustring>		NameDesc;
					Gtk::TreeModelColumn<Glib::ustring>		Authors;
					Gtk::TreeModelColumn<Glib::ustring>		Copyright;
					Gtk::TreeModelColumn<Glib::ustring>		Website;
					Gtk::TreeModelColumn<gint64>			Id;
					Gtk::TreeModelColumn<Glib::ustring>		Name;
	
				ColumnsT ()
				{
					add(NameDesc);
					add(Authors);
					add(Copyright);
					add(Website);
					add(Id);
					add(Name);
				};
			};
	
			ColumnsT Columns;
			Glib::RefPtr<Gtk::ListStore> Store;
			PlaylistPluginManager & m_Manager;
			PlaylistPluginManagerGUI & m_ManagerGUI;
			MPX::Library & m_Library;
			Gtk::Button *m_Exec;

			SignalGotIDs m_Signal_Got_IDs;

		public:

			PluginPTV (const Glib::RefPtr<Gnome::Glade::Xml> &xml, PlaylistPluginManager & manager, PlaylistPluginManagerGUI & manager_gui, MPX::Library& lib)
			: WidgetLoader<Gtk::TreeView>(xml, "treeview")
			, m_Manager(manager)
			, m_ManagerGUI(manager_gui)
			, m_Library(lib)
			{
				Store = Gtk::ListStore::create(Columns);

				Gtk::CellRendererText * cell = manage(new Gtk::CellRendererText);
				Gtk::TreeViewColumn * col = manage(new Gtk::TreeViewColumn(_("Name/Description")));
				col->pack_start(*cell);
				col->add_attribute(*cell, "markup", Columns.NameDesc);
				append_column(*col);
		
				append_column(_("Authors"), Columns.Authors);
				append_column(_("Copyright"), Columns.Copyright);
				append_column(_("Website"), Columns.Website);

				PlaylistPluginHoldMap const& map = m_Manager.get_map();	
				for(PlaylistPluginHoldMap::const_iterator i = map.begin(); i != map.end(); ++i)
				{
					TreeIter iter = Store->append();
					(*iter)[Columns.Name] = i->second->get_name();
					(*iter)[Columns.NameDesc] = (boost::format("<b><big>%1%</big></b>\n%2%") % i->second->get_name() % i->second->get_desc()).str();
					(*iter)[Columns.Authors] = i->second->get_authors();
					(*iter)[Columns.Copyright] = i->second->get_copyright();
					(*iter)[Columns.Website] = i->second->get_website();
					(*iter)[Columns.Id] = i->second->get_id();
				}

				xml->get_widget("exec", m_Exec);
				m_Exec->set_sensitive(false);
				m_Exec->signal_clicked().connect( sigc::mem_fun( *this, &PluginPTV::on_exec_clicked ));
				get_selection()->signal_changed().connect( sigc::mem_fun( *this, &PluginPTV::on_selection_changed ));
				set_model (Store);
			}

			~PluginPTV ()
			{
			}

			void
			on_selection_changed ()
			{
				m_Exec->set_sensitive(get_selection()->count_selected_rows());
			}

			void
			on_exec_clicked ()
			{
				g_return_if_fail(get_selection()->count_selected_rows());

				TreeIter iter = get_selection()->get_selected();
				gint64 id = (*iter)[Columns.Id];
				TrackIdV v;
				m_ManagerGUI.hide();
				m_Manager.run(id, m_Library, v); 
				m_Signal_Got_IDs.emit(v);
			}

			virtual void
			on_row_activated (const TreeModel::Path& path, TreeViewColumn* column)
			{
				TreeIter iter = Store->get_iter(path);
				gint64 id = (*iter)[Columns.Id];
				TrackIdV v;
				m_ManagerGUI.hide();
				m_Manager.run(id, m_Library, v); 
				m_Signal_Got_IDs.emit(v);
			}
	};
}

namespace MPX
{
		PlaylistPluginManagerGUI::~PlaylistPluginManagerGUI ()
		{
			delete m_PTV;
		}

		PlaylistPluginManagerGUI::PlaylistPluginManagerGUI (const Glib::RefPtr<Gnome::Glade::Xml> &xml, PlaylistPluginManager &obj_manager, MPX::Library & lib)
	    : WidgetLoader<Gtk::Window>(xml, "window")
		, m_Manager(obj_manager)
		, m_PTV(new PluginPTV(xml, obj_manager, *this, lib))
		{
			Gtk::Button * bClose;
			xml->get_widget("close", bClose);
			bClose->signal_clicked().connect( sigc::mem_fun( *this, &Gtk::Widget::hide ) );
		}

		PlaylistPluginManagerGUI*
		PlaylistPluginManagerGUI::create (PlaylistPluginManager &obj_manager, MPX::Library & lib)
		{
			const std::string path (build_filename(DATA_DIR, build_filename("glade","playlist-plugin-manager.glade")));
			PlaylistPluginManagerGUI * p = new PlaylistPluginManagerGUI(Gnome::Glade::Xml::create (path), obj_manager, lib); 
			return p;
		}

	
		bool
		PlaylistPluginManagerGUI::on_delete_event (GdkEventAny* G_GNUC_UNUSED)
		{
			Gtk::Window::hide ();
			return true;
		}

		SignalGotIDs&
		PlaylistPluginManagerGUI::signal_got_ids ()
		{
			return m_PTV->m_Signal_Got_IDs;
		}
}
