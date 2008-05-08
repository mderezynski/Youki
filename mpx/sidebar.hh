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
#include <boost/optional.hpp>
#include "mpx/types.hh"

namespace MPX
{
  class Sidebar
    : public Gtk::TreeView
  {

    public:

        typedef sigc::signal<void, ItemKey>                SignalIdChanged;

        Sidebar (BaseObjectType                       * obj,
                 Glib::RefPtr<Gnome::Glade::Xml> const& xml);
        virtual ~Sidebar ();

        ItemKey const&
        getActiveId ();

        ItemKey const&
        getVisibleId ();

        void
        setActiveId (ItemKey const&);

        void
        clearActiveId ();

        void
        setItemCount (ItemKey const&, gint64 count);

        void
        addItem(
            const Glib::ustring& name,
            Gtk::Widget* ui,
            const Glib::RefPtr<Gdk::Pixbuf> & icon,
            gint64 id
        );

        void
        addSubItem(
            const Glib::ustring& name,
            Gtk::Widget* ui,
            const Glib::RefPtr<Gdk::Pixbuf> & icon,
            gint64 parent,
            gint64 id
        );

        SignalIdChanged&
        signal_id_changed();

    private:

        void
        cell_data_func(
            Gtk::CellRenderer*,
            const Gtk::TreeIter&,
            int
        );


        bool
        slot_select(
            Glib::RefPtr <Gtk::TreeModel>   const& model,
            Gtk::TreeModel::Path            const& path,
            bool                                   was_selected
        );

        void
        on_selection_changed ();

        bool
        on_drag_motion (Glib::RefPtr<Gdk::DragContext> const& context, int x, int y, guint time);

    private:

        struct SourceColumns : public Gtk::TreeModel::ColumnRecord
        {
            Gtk::TreeModelColumn<Glib::ustring> name;  
            Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
            Gtk::TreeModelColumn<ItemKey> key;
            Gtk::TreeModelColumn<gint64>  page;

            SourceColumns()
            {
                add (name);
                add (icon);
                add (key);
                add (page);
            };
        };

        SourceColumns                Columns;
        Glib::RefPtr<Gtk::TreeStore> Store;

        typedef std::map<ItemKey, Gtk::TreeIter> IdIterMapT;
        typedef std::set<gint64>                 ActiveIdT;
        typedef std::map<ItemKey, gint64>        ItemCountMap;

        IdIterMapT      m_IdIterMap;
        ActiveIdT       m_ActiveIds;
        ItemCountMap    m_ItemCounts;

        boost::optional<ItemKey> m_active_id;
        ItemKey                  m_visible_id;
        Gtk::Notebook          * m_Notebook;

        Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;

        SignalIdChanged signal_id_changed_;
  };
}
#endif
