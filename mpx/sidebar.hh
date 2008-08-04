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
#include "plugin.hh"
#include "mpx/mpx-types.hh"
#include "mpx/com/treeview-popup.hh"
#include "mpx/widgets/widgetloader.hh"

namespace MPX
{
  class PlaybackSource;
  class Sidebar
    : public Gnome::Glade::WidgetLoader<MPX::TreeViewPopup>
  {

    public:

        Sidebar (Glib::RefPtr<Gnome::Glade::Xml> const& xml,
                 MPX::PluginManager&);
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
            const Glib::ustring& /*name*/,
            Gtk::Widget* /*ui*/,
            PlaybackSource* /*source*/,
            const Glib::RefPtr<Gdk::Pixbuf>& /*icon*/,
            gint64 /*id*/
        );

        void
        addSubItem(
            const Glib::ustring& /*name*/,
            Gtk::Widget* /*ui*/,
            PlaybackSource* /*source*/,
            const Glib::RefPtr<Gdk::Pixbuf>& /*icon*/,
            gint64 /*parent*/,
            gint64 /*id*/
        );

        void
        addSeparator ();

        typedef sigc::signal<void, ItemKey> SignalIdChanged;

        SignalIdChanged&
        signal_id_changed();

    protected:

        virtual void
        popup_prepare_actions(Gtk::TreePath const&, bool);

    private:

        void
        cell_data_func(
            Gtk::CellRenderer*,
            const Gtk::TreeIter&,
            int
        );

        bool
        slot_select(
            Glib::RefPtr <Gtk::TreeModel> const&,
            Gtk::TreeModel::Path const&,
            bool 
        );

        bool
        slot_separator(
            Glib::RefPtr <Gtk::TreeModel> const&,
            Gtk::TreeIter const&
        );
            
        void
        on_attach_plugin ();

        void
        on_selection_changed ();

    protected:

        virtual bool
        on_drag_motion (Glib::RefPtr<Gdk::DragContext> const& context, int x, int y, guint time);

        virtual void
        on_drag_data_received (Glib::RefPtr<Gdk::DragContext> const& context, int x, int y, const Gtk::SelectionData& data, guint, guint);

    private:

        Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;

        struct SourceColumns : public Gtk::TreeModel::ColumnRecord
        {
            Gtk::TreeModelColumn<Glib::ustring> name;  
            Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
            Gtk::TreeModelColumn<ItemKey> key;
            Gtk::TreeModelColumn<gint64>  page;
            Gtk::TreeModelColumn<bool>    separator;

            SourceColumns()
            {
                add (name);
                add (icon);
                add (key);
                add (page);
                add (separator);
            };
        };

        SourceColumns                Columns;
        Glib::RefPtr<Gtk::TreeStore> Store;

        typedef std::map<ItemKey, Gtk::TreeIter>        IdIterMapT;
        typedef std::map<ItemKey, gint64>               ItemCountMap;
        typedef std::map<std::string, PlaybackSource*>  SourcesByGUID;
        typedef std::map<std::string, PlaybackSource*>  SourcesByClass;
        typedef std::map<ItemKey, PlaybackSource*>      SourcesByKey;

        IdIterMapT               m_IdIterMap;
        ItemCountMap             m_ItemCounts;
        SourcesByGUID            m_SourcesByGUID;
        SourcesByClass           m_SourcesByClass;
        SourcesByKey             m_SourcesByKey;
        boost::optional<ItemKey> m_ActiveId;
        ItemKey                  m_VisibleId;
        Gtk::Notebook          * m_Notebook;
        PluginManager          & m_PluginManager;

        struct Signals_T
        {
            SignalIdChanged     IdChanged;          
        };

        Signals_T Signals;
  };
}
#endif
