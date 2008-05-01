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
#include "mpx/widgets/gossip-cell-renderer-expander.h"
#include "sidebar.hh"
using namespace Glib;
using namespace Gtk;

namespace MPX
{
    Sidebar::Sidebar (BaseObjectType                 *  obj,
                      RefPtr<Gnome::Glade::Xml> const&  xml)
    : TreeView  (obj)
    , m_ref_xml (xml)
    {
        TreeViewColumn * column = manage (new TreeViewColumn);

        CellRendererPixbuf * cell1 = manage (new CellRendererPixbuf);
        CellRendererText * cell2 = manage (new CellRendererText);

        cell1->property_xalign() = 1.0;
        cell1->property_width() = 36;
        cell1->property_ypad() = 1;

        cell2->property_xalign() = 0.0;
        cell2->property_ypad() = 1;

        column->pack_start( *cell1, false );
        column->set_cell_data_func( *cell1, sigc::bind( sigc::mem_fun( *this, &Sidebar::cell_data_func ), 0)); 

        column->pack_start( *cell2, true );
        column->set_cell_data_func( *cell2, sigc::bind( sigc::mem_fun( *this, &Sidebar::cell_data_func ), 1));

        set_show_expanders( false );

        GtkCellRenderer * renderer = gossip_cell_renderer_expander_new ();
        gtk_tree_view_column_pack_end (column->gobj(), renderer, FALSE);
        gtk_tree_view_column_set_cell_data_func (column->gobj(),
                             renderer,
                             GtkTreeCellDataFunc(rb_sourcelist_expander_cell_data_func),
                             this,
                             NULL);

        append_column( *column );

        get_selection()->set_select_function( sigc::mem_fun( *this, &Sidebar::slot_select ));
        get_selection()->set_mode( SELECTION_BROWSE );
        get_selection()->signal_changed().connect( sigc::mem_fun( *this, &Sidebar::on_selection_changed )); 

        m_Store = TreeStore::create( m_SourceColumns );
        set_model( m_Store );
    }

    Sidebar::~Sidebar()
    {
    }

    void
    Sidebar::on_selection_changed ()
    {
        TreeIter iter = get_selection()->get_selected();
        gint64 id = (*iter)[m_SourceColumns.id];
        m_visible_id = id;
        signal_id_changed_.emit(id);
    }

    gint64
    Sidebar::getVisibleId ()
    {
        return m_visible_id;
    }

    gint64
    Sidebar::getActiveId ()
    {
        if(m_active_id)
        {
            return m_active_id.get();
        }
        else
        {
            throw std::runtime_error(_("No active Id"));
        }
    }

    void 
    Sidebar::setActiveId (gint64 id)
    {
        m_active_id = id;
        queue_draw ();
    }

    void
    Sidebar::clearActiveId ()
    {
        m_active_id.reset();
        queue_draw ();
    }

    void
    Sidebar::cell_data_func( CellRenderer * basecell, const TreeModel::iterator& iter, int cellid )
    {
        if(cellid == 1)
        {
            Gtk::CellRendererText & cell = *(dynamic_cast<Gtk::CellRendererText*>(basecell));
            cell.property_markup() = (*iter)[m_SourceColumns.name];
            if(m_active_id)
            {
                if(m_active_id.get() == (*iter)[m_SourceColumns.id])
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
            cell.property_pixbuf() = (*iter)[m_SourceColumns.icon];
        }
    }

    void
    Sidebar::addItem (const Glib::ustring& name, const Glib::RefPtr<Gdk::Pixbuf>& icon, gint64 id)
    {
        TreeIter iter = m_Store->append();
        (*iter)[m_SourceColumns.name] = name;
        (*iter)[m_SourceColumns.icon] = icon;
        (*iter)[m_SourceColumns.id] = id;

        m_IdIterMap.insert(std::make_pair(id, iter));
    }

    bool
    Sidebar::slot_select (Glib::RefPtr <Gtk::TreeModel> const& model,
                Gtk::TreeModel::Path const& path, bool was_selected)
    {
        return true; 
    }

    void
    Sidebar::rb_sourcelist_expander_cell_data_func (GtkTreeViewColumn *column,
                           GtkCellRenderer   *cell,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           data) 
    {
        if (gtk_tree_model_iter_has_child (model, iter))
        {
            GtkTreePath *path;
            gboolean     row_expanded;

            path = gtk_tree_model_get_path (model, iter);
            row_expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (column->tree_view), path);
            gtk_tree_path_free (path);

            g_object_set (cell,
                      "visible", TRUE,
                      "expander-style", row_expanded ? GTK_EXPANDER_EXPANDED : GTK_EXPANDER_COLLAPSED,
                      NULL);
        } else {
            g_object_set (cell, "visible", FALSE, NULL);
        }

        Gtk::CellRenderer * r = Glib::wrap(cell,false);
        Sidebar * w = reinterpret_cast<Sidebar*>(data);
        r->property_cell_background_gdk() = w->get_style()->get_bg(STATE_INSENSITIVE);
    }
}
