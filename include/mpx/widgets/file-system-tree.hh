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

#ifndef MPX_MLIBMANAGER_HH 
#define MPX_MLIBMANAGER_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif // HAVE_CONFIG_H
#include <giomm.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/window.h>
#include <libglademm/xml.h>
#include <sigx/sigx.h>
#include "mpx/widgets/widgetloader.hh"

using namespace Gnome::Glade;

namespace MPX
{
    class FileSystemTree
      : public WidgetLoader<Gtk::TreeView>
      , public sigx::glib_auto_dispatchable
    {
        public:

            FileSystemTree (Glib::RefPtr<Gnome::Glade::Xml> const& xml, std::string const&); 
            virtual ~FileSystemTree () {}

            void
            build_file_system_tree (std::string const& root_path);

        protected:

            virtual void
            on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time);

            virtual void
            on_row_expanded (const Gtk::TreeIter & iter, const Gtk::TreePath & path);

        private:

            int
            file_system_tree_sort (const Gtk::TreeIter & iter_a, const Gtk::TreeIter & iter_b);

            void
            prescan_path (std::string const& path, Gtk::TreeIter & iter);

            void
            append_path (std::string const& root_path, Gtk::TreeIter & root_iter);
            
            void
            cell_data_func_text (Gtk::CellRenderer * basecell, Gtk::TreeIter const& iter);


            struct FileSystemTreeColumnsT : public Gtk::TreeModel::ColumnRecord
            {
                Gtk::TreeModelColumn<Glib::ustring>   SegName;
                Gtk::TreeModelColumn<std::string>     FullPath;
                Gtk::TreeModelColumn<bool>            WasExpanded;
                Gtk::TreeModelColumn<bool>            IsDir;

                FileSystemTreeColumnsT ()
                {
                    add (SegName);
                    add (FullPath);
                    add (WasExpanded);
                    add (IsDir);
                };
            };

            FileSystemTreeColumnsT FileSystemTreeColumns;
            Glib::RefPtr<Gtk::TreeStore> FileSystemTreeStore;
    };
}
#endif
