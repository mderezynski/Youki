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
#ifndef MPX_SOURCES_HH
#define MPX_SOURCES_HH
#include "config.h"

#include <gtkmm.h>
#include <libglademm/xml.h>
#include <exo/exo.h>

namespace MPX
{
  class Sources
  {
    private:

		typedef sigc::signal<void, int> SignalSourceChanged;

        struct SourceColumns : public Gtk::TreeModel::ColumnRecord
        {
            Gtk::TreeModelColumn<Glib::ustring> name;  
            Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;

            SourceColumns()
            {
                add (icon);
                add (name);
            };
        };

        Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;
        SourceColumns m_SourceColumns;
        Glib::RefPtr<Gtk::ListStore> m_Store;
		ExoIconView *m_IconView;

		static void
		on_selection_changed (ExoIconView*,gpointer);

		SignalSourceChanged signal_source_changed_;
		int m_CurrentSource;
     
    public:

        Sources (Glib::RefPtr<Gnome::Glade::Xml> const& xml);
        virtual ~Sources ();

        void addSource (const Glib::ustring& name, const Glib::RefPtr<Gdk::Pixbuf>& icon);

		int getSource ()
		{
			return m_CurrentSource;
		}

		SignalSourceChanged& sourceChanged ()
		{
			return signal_source_changed_;
		}
  };
}
#endif
