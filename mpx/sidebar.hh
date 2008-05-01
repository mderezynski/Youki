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
#ifndef MPX_SIDEBAR_HH
#define MPX_SIDEBAR_HH
#include "config.h"
#include <map>
#include <set>
#include <gtkmm.h>
#include <libglademm/xml.h>

namespace MPX
{
  class Sidebar
    : public Gtk::TreeView
  {
    private:

        Glib::RefPtr<Gnome::Glade::Xml>   m_ref_xml;
        struct SourceColumns : public Gtk::TreeModel::ColumnRecord
        {
            Gtk::TreeModelColumn<Glib::ustring> name;  
            Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
            Gtk::TreeModelColumn<gint64> id;

            SourceColumns()
            {
                add (name);
                add (icon);
                add (id);
            };
        };

        SourceColumns m_SourceColumns;
        Glib::RefPtr<Gtk::TreeStore> m_Store;

        typedef std::map<gint64, Gtk::TreeIter> IdIterMapT;
        typedef std::set<gint64>              ActiveIdT;
        IdIterMapT m_IdIterMap;
        ActiveIdT m_ActiveIds;


        void cell_data_func( Gtk::CellRenderer*, const Gtk::TreeIter&, int );

        bool
        slot_select (Glib::RefPtr <Gtk::TreeModel> const& model,
                     Gtk::TreeModel::Path const& path,
                     bool was_selected);

        static void
        rb_sourcelist_expander_cell_data_func (GtkTreeViewColumn *column,
                               GtkCellRenderer   *cell,
                               GtkTreeModel      *model,
                               GtkTreeIter       *iter,
                               gpointer           data) ;

        void on_selection_changed ();
        gint64 m_active_id;

    public:

        typedef sigc::signal<void, gint64> SignalIdChanged;

    private:

        SignalIdChanged signal_id_changed_;

    public:

        Sidebar (BaseObjectType                       * obj,
                 Glib::RefPtr<Gnome::Glade::Xml> const& xml);
        virtual ~Sidebar ();

        SignalIdChanged&
        signal_id_changed()
        {
            return signal_id_changed_;
        }

        gint64
        getActiveId ();

        void
        addItem (const Glib::ustring& name, const Glib::RefPtr<Gdk::Pixbuf>& icon, gint64 id);
  };
}
#endif
