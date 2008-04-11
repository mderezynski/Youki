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
#include <gtkmm.h>
#include <gtk/gtktreeview.h>
#include <cairomm/cairomm.h>
#include "mpx-sources.hh"
using namespace Glib;
using namespace Gtk;

namespace MPX
{
    Sources::Sources (RefPtr<Gnome::Glade::Xml> const&  xml)
    : m_ref_xml (xml)
    {
		m_IconView = EXO_ICON_VIEW(exo_icon_view_new ());
        exo_icon_view_set_selection_mode( m_IconView, GTK_SELECTION_BROWSE );
        m_Store = ListStore::create( m_SourceColumns );
        exo_icon_view_set_model( m_IconView, GTK_TREE_MODEL(m_Store->gobj()) );
		GtkCellRenderer * cellPixbuf = gtk_cell_renderer_pixbuf_new();
		GtkCellRenderer * cellText = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(m_IconView), cellPixbuf, FALSE);
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(m_IconView), cellText, FALSE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(m_IconView), cellPixbuf, "pixbuf", 0);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(m_IconView), cellText, "text", 1);
		g_object_set(G_OBJECT(cellText), "xalign", double(0.5), NULL);
		GtkContainer * container = GTK_CONTAINER((xml->get_widget("scrolledwindow-sources")->gobj()));
		gtk_container_add(GTK_CONTAINER(container), GTK_WIDGET(m_IconView));
		gtk_widget_show_all(GTK_WIDGET(m_IconView));
		g_signal_connect(G_OBJECT(m_IconView), "selection-changed", G_CALLBACK(on_selection_changed), this);
    }

    Sources::~Sources()
    {
    }

    void
    Sources::addSource (const Glib::ustring& name, const Glib::RefPtr<Gdk::Pixbuf>& icon)
    {
        TreeIter iter = m_Store->append();
        (*iter)[m_SourceColumns.name] = name;
        (*iter)[m_SourceColumns.icon] = icon;
    }

	void
	Sources::on_selection_changed (ExoIconView *view, gpointer data)
	{
		Sources & sources = *(reinterpret_cast<Sources*>(data));
		GList * list = exo_icon_view_get_selected_items(sources.m_IconView);
		if(list)
		{
			TreePath path (static_cast<GtkTreePath*>(list->data));
			sources.m_CurrentSource = path[0];
			sources.signal_source_changed_.emit( sources.m_CurrentSource );
		}
	}
}
