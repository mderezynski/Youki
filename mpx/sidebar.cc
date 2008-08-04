//  MPX
//  Copyright (C) 2007 MPX Project 
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
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <gtk/gtktreeview.h>
#include <cairomm/cairomm.h>
#include <boost/format.hpp>
#include "mpx/i-playbacksource.hh"
#include "mpx/util-ui.hh"
#include "mpx/widgets/cell-renderer-expander.h"
#include "plugin.hh"
#include "sidebar.hh"
using namespace Glib;
using namespace Gtk;

namespace
{
    const char ACTION_ATTACH_PLUGIN [] = "action-attach-plugin";
    
    const char ui_sidebar_popup [] =
    "<ui>"
    ""
    "<menubar name='popup-sidebar'>"
    ""
    "   <menu action='dummy' name='menu-sidebar'>"
    "       <menuitem action='action-attach-plugin' name='action-attach-plugin'/>"
    "   </menu>"
    ""
    "</menubar>"
    ""
    "</ui>";
}

namespace MPX
{
    Sidebar::Sidebar (RefPtr<Gnome::Glade::Xml> const& xml,
                      PluginManager                  & obj_plugin_manager)
                      
    : Gnome::Glade::WidgetLoader<MPX::TreeViewPopup>(xml, "sidebar") 
    , m_ref_xml(xml)
    , m_PluginManager(obj_plugin_manager)
    {
        m_default_path = "/popup-sidebar/menu-sidebar";

        m_ref_xml->get_widget("notebook-sources", m_Notebook);

        TreeViewColumn * column = manage (new TreeViewColumn);

        CellRendererPixbuf * cell1 = manage (new CellRendererPixbuf);
        CellRendererText * cell2 = manage (new CellRendererText);
        CellRendererText * cell3 = manage (new CellRendererText);

        cell1->property_xalign() = 0.5;
        cell1->property_ypad() = 1;
        cell1->property_width() = 24;

        cell2->property_xalign() = 0.0;
        cell2->property_ypad() = 1;

        cell3->property_xalign() = 1.0;
        cell3->property_ypad() = 1;

        column->pack_start( *cell1, false );
        column->set_cell_data_func( *cell1, sigc::bind( sigc::mem_fun( *this, &Sidebar::cell_data_func ), 0)); 

        column->pack_start( *cell2, true );
        column->set_cell_data_func( *cell2, sigc::bind( sigc::mem_fun( *this, &Sidebar::cell_data_func ), 1));
        append_column( *column );

        column = manage (new TreeViewColumn);
        column->pack_start( *cell3, true );
        column->set_cell_data_func( *cell3, sigc::bind( sigc::mem_fun( *this, &Sidebar::cell_data_func ), 2));
        append_column( *column );

        set_row_separator_func( sigc::mem_fun( *this, &Sidebar::slot_separator ));

        get_selection()->set_select_function( sigc::mem_fun( *this, &Sidebar::slot_select ));
        get_selection()->set_mode( SELECTION_BROWSE );
        get_selection()->signal_changed().connect( sigc::mem_fun( *this, &Sidebar::on_selection_changed )); 

        Store = TreeStore::create( Columns );
        set_model( Store );

        std::vector<TargetEntry> Entries;
        Entries.push_back(TargetEntry("mpx-album"));
        Entries.push_back(TargetEntry("mpx-track"));
        drag_dest_set(Entries, DEST_DEFAULT_ALL);
        drag_dest_add_uri_targets();
    }

    Sidebar::~Sidebar()
    {
    }

    ItemKey const&
    Sidebar::getVisibleId ()
    {
        return m_VisibleId;
    }

    ItemKey const&
    Sidebar::getActiveId ()
    {
        if(m_ActiveId)
        {
            return m_ActiveId.get();
        }
        else
        {
            throw std::runtime_error(_("No active Id"));
        }
    }

    void 
    Sidebar::setActiveId(ItemKey const& key)
    {
        m_ActiveId = key;
        queue_draw ();
    }

    void
    Sidebar::setItemCount(ItemKey const& key, gint64 count)
    {
        ItemCountMap::iterator i = m_ItemCounts.find(key);
        if(i != m_ItemCounts.end())
        {
            m_ItemCounts.erase(i);
        }

        m_ItemCounts[key] = count;
        queue_draw ();
    }

    void
    Sidebar::clearActiveId ()
    {
        m_ActiveId.reset();
        queue_draw ();
    }

    void
    Sidebar::addSeparator()
    {
        TreeIter iter = Store->append();
        (*iter)[Columns.separator] = true;
    }

    void
    Sidebar::addItem(
            const Glib::ustring             & name,
            Gtk::Widget                     * ui,
            PlaybackSource                  * source,
            const Glib::RefPtr<Gdk::Pixbuf> & icon,
            gint64 id
    )
    {
        TreeIter iter = Store->append();

        (*iter)[Columns.name] = name;
        (*iter)[Columns.icon] = icon;
        (*iter)[Columns.key]  = ItemKey(boost::optional<gint64>(), id);
        (*iter)[Columns.page] = m_Notebook->append_page(*ui);
        (*iter)[Columns.separator] = false;

        m_IdIterMap.insert(
            std::make_pair(
                ItemKey(
                    boost::optional<gint64>(),
                    id
                ),
                iter
        ));

        m_SourcesByKey.insert(
            std::make_pair(
                ItemKey(
                    boost::optional<gint64>(),
                    id
                ),
                source
        ));
    }

    void
    Sidebar::addSubItem(
            const Glib::ustring             & name,
            Gtk::Widget                     * ui,
            PlaybackSource                  * source,
            const Glib::RefPtr<Gdk::Pixbuf> & icon,
            gint64 parent,
            gint64 id
    )
    {
        if(!m_IdIterMap.count(ItemKey(boost::optional<gint64>(),parent)))
        {
            // throw std::runtime_error ("Parent id doesn't exist");
            return;
        }

        TreeIter parent_iter = m_IdIterMap.find(ItemKey(boost::optional<gint64>(),parent))->second;
        TreeIter iter = Store->append(parent_iter->children());

        (*iter)[Columns.name] = name;
        (*iter)[Columns.icon] = icon;
        (*iter)[Columns.key]  = ItemKey(parent,id); 
        (*iter)[Columns.page] = m_Notebook->append_page(*ui);
        (*iter)[Columns.separator] = false;

        m_IdIterMap.insert(std::make_pair(ItemKey(parent,id), iter));
        m_SourcesByKey.insert(std::make_pair(ItemKey(boost::optional<gint64>(),id), source));
    }


    void
    Sidebar::popup_prepare_actions(Gtk::TreePath const&, bool)
    {
    }

    void
    Sidebar::on_attach_plugin ()
    {
    }

    bool
    Sidebar::on_drag_motion (RefPtr<Gdk::DragContext> const& context, int x, int y, guint time)
    {
        TreeModel::Path path;
        TreeViewDropPosition pos;

        if( get_dest_row_at_pos( x, y, path, pos ))
        {
            get_selection()->select(path);
        }

        return true;
    }

    void
    Sidebar::on_drag_data_received (RefPtr<Gdk::DragContext> const& context, int x, int y, const SelectionData& data, guint, guint)
    {
        TreeModel::Path path;
        TreeViewDropPosition pos;

        if( get_dest_row_at_pos( x, y, path, pos ))
        {
            TreeIter iter = Store->get_iter(path); 
            SourcesByKey::iterator i = m_SourcesByKey.find((*iter)[Columns.key]);
            PlaybackSource* & source = i->second;
            source->drag_data(context, data, 0, 0);
        }
    }

    void
    Sidebar::on_selection_changed ()
    {
        if(get_selection()->count_selected_rows())
        {
            TreeIter iter = get_selection()->get_selected();

            ItemKey key = (*iter)[Columns.key];
            gint64 page = (*iter)[Columns.page];

            m_Notebook->set_current_page( page );
            m_VisibleId = key;

            Signals.IdChanged.emit(key);
        }
    }

    void
    Sidebar::cell_data_func( CellRenderer * basecell, const TreeModel::iterator& iter, int cellid )
    {
        if(cellid == 2)
        {
            Gtk::CellRendererText & cell = *(dynamic_cast<Gtk::CellRendererText*>(basecell));
            ItemKey key = ((*iter)[Columns.key]);
            if(m_ItemCounts.count(key) && m_ItemCounts[key] != 0)
            {
                cell.property_markup() = (boost::format("<small>(%lld)</small>") % m_ItemCounts[key]).str();
            }
            else
            {
                cell.property_markup() = ""; 
            } 
        }
        else
        if(cellid == 1)
        {
            Gtk::CellRendererText & cell = *(dynamic_cast<Gtk::CellRendererText*>(basecell));
            cell.property_markup() = (*iter)[Columns.name];
            if(m_ActiveId)
            {
                if(m_ActiveId.get() == ItemKey((*iter)[Columns.key]))
                {
                    cell.property_weight() = Pango::WEIGHT_BOLD;
                    return;
                }
            }

            cell.property_weight() = Pango::WEIGHT_NORMAL;
        }
        else if(cellid == 0)
        {
            Gtk::CellRendererPixbuf & cell = *(dynamic_cast<Gtk::CellRendererPixbuf*>(basecell));
            cell.property_pixbuf() = (*iter)[Columns.icon];
        }
    }

    bool
    Sidebar::slot_select(
        const Glib::RefPtr <Gtk::TreeModel>&    model,
        const Gtk::TreeModel::Path&             path,
        bool                                    was_selected
    )
    {
        return true; 
    }

    bool
    Sidebar::slot_separator(
        const Glib::RefPtr <Gtk::TreeModel>&    model,
        const Gtk::TreeIter&                    iter
    )
    {
        return bool((*iter)[Columns.separator]);
    }

    Sidebar::SignalIdChanged&
    Sidebar::signal_id_changed()
    {
        return Signals.IdChanged;
    }
}
