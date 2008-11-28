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
#ifndef MPX_PLUGIN_MANGER_GUI_HH
#define MPX_PLUGIN_MANGER_GUI_HH

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plugin.hh"
#include "mpx/widgets/widgetloader.hh"
#include "mpx/mpx-services.hh"

#include <libglademm/xml.h>
#include <boost/python.hpp>

namespace MPX
{
    class PTV;
    class PluginManagerGUI
      : public Gnome::Glade::WidgetLoader<Gtk::Window>
      , public Service::Base
    {
		public:

			virtual ~PluginManagerGUI ();

		protected:

			PluginManagerGUI (const Glib::RefPtr<Gnome::Glade::Xml>&, PluginManager &);
			virtual bool on_delete_event (GdkEventAny* G_GNUC_UNUSED);

		public:

			static PluginManagerGUI*
			create (PluginManager &);

			void
			on_selection_changed();

			void
			on_row_changed(const Gtk::TreeModel::Path&, const Gtk::TreeModel::iterator&);

			void
			show_dialog();

			void
			show_options();

			void
			set_error_text();

		private:

            void
            check_traceback();

			PluginManager & m_Manager;
			PTV * m_PTV;
			Gtk::Notebook * notebook;
			Gtk::Alignment * options;
			Gtk::Button * buTraceback;
			Gtk::Label * error, * overview;
    };
}
#endif
