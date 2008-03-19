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

namespace MPX
{
  class Sources
    : public Gtk::IconView
  {
    private:

        Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;

		typedef sigc::signal<void, int> SignalSourceChanged;

        struct SourceColumns : public Gtk::TreeModel::ColumnRecord
        {
            Gtk::TreeModelColumn<Glib::ustring> name;  
            Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;

            SourceColumns()
            {
                add (name);
                add (icon);
            };
        };

        SourceColumns m_SourceColumns;
        Glib::RefPtr<Gtk::ListStore> m_Store;

#if 0
        Gtk::TreeIter m_RootDevices;
        Gtk::TreeIter m_RootSources;
#endif

#if 0
        bool
        slot_select (Glib::RefPtr <Gtk::TreeModel> const& model, Gtk::TreeModel::Path const& path, bool was_selected);

        void
		cell_data_func( Gtk::CellRenderer*, const Gtk::TreeIter&, int );

        static void
        rb_sourcelist_expander_cell_data_func (GtkTreeViewColumn *column,
                               GtkCellRenderer   *cell,
                               GtkTreeModel      *model,
                               GtkTreeIter       *iter,
                               gpointer           data) ;
#endif

		void
		on_selection_changed ();

		SignalSourceChanged signal_source_changed_;
		int m_CurrentSource;
     
    public:

        Sources (BaseObjectType                       * obj,
                 Glib::RefPtr<Gnome::Glade::Xml> const& xml);
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
