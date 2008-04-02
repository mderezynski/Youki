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
#include <libglademm/xml.h>
#include <boost/python.hpp>
#include "plugin.hh"
#include "plugin-manager-gui.hh"

using namespace Glib;

namespace MPX
{
	class PTV
		: public WidgetLoader<Gtk::TreeView>
	{
		public:

			PTV (const Glib::RefPtr<Gnome::Glade::Xml> &xml)
			: WidgetLoader<Gtk::TreeView>(xml, "treeview")
			{
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
		, m_PTV(new PTV(xml))
		{
		}

		PluginManagerGUI*
		PluginManagerGUI::create (PluginManager &obj_manager)
		{
			const std::string path (build_filename(DATA_DIR, build_filename("glade","plugin-manager.glade")));
			PluginManagerGUI * p = new PluginManagerGUI(Gnome::Glade::Xml::create (path), obj_manager); 
			return p;
		}
}
