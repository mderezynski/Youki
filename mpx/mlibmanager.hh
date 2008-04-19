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

#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/window.h>
#include <libglademm/xml.h>

#include "mpx/widgetloader.h"
#include "mpx/hal.hh"
#include "mpx/library.hh"

#include "libhal++/hal++.hh"

using namespace Gnome::Glade;

namespace MPX
{
    class MLibManager
      : public WidgetLoader<Gtk::Window>
    {
        public:

            static MLibManager* create (MPX::HAL & obj_hal, MPX::Library & obj_library);
            virtual ~MLibManager ();

            void
            present ();

            void
            hide ();

        private:

            MLibManager (Glib::RefPtr<Gnome::Glade::Xml> const& xml, MPX::HAL & obj_hal, MPX::Library & obj_library);

            void
            scan_end (gint64,gint64,gint64,gint64,gint64);

            void
            scan_start ();

            void
            populate_volumes ();

            void
            build_fstree (std::string const& root_path);
        
            void
            append_path (std::string const& root_path, Gtk::TreeIter & root_iter);

            void
            prescan_path (std::string const& path, Gtk::TreeIter & iter);

            int
            fstree_sort (const Gtk::TreeIter & iter_a, const Gtk::TreeIter & iter_b);

            bool
            has_active_parent (Gtk::TreeIter &);
            

            void
            on_volumes_cbox_changed ();

            void
            on_fstree_row_expanded (const Gtk::TreeIter & iter, const Gtk::TreePath & path);

            void
            on_path_toggled (const Glib::ustring &);
    


            Gtk::ComboBox * m_VolumesCBox;

            struct VolumeColumnsT : public Gtk::TreeModel::ColumnRecord
            {
                Gtk::TreeModelColumn<Glib::ustring>             Name;
                Gtk::TreeModelColumn<Hal::RefPtr<Hal::Volume> > Volume;

                VolumeColumnsT ()
                {
                    add (Name);
                    add (Volume);
                };
            };

            VolumeColumnsT VolumeColumns;
            Glib::RefPtr<Gtk::ListStore> VolumeStore;
    


            Gtk::TreeView * m_FSTree;

            struct FSTreeColumnsT : public Gtk::TreeModel::ColumnRecord
            {
                Gtk::TreeModelColumn<Glib::ustring>             SegName;
                Gtk::TreeModelColumn<std::string>               FullPath;
                Gtk::TreeModelColumn<bool>                      Active;
                Gtk::TreeModelColumn<bool>                      WasExpanded;

                FSTreeColumnsT ()
                {
                    add (SegName);
                    add (FullPath);
                    add (Active);
                    add (WasExpanded);
                };
            };

            FSTreeColumnsT FSTreeColumns;
            Glib::RefPtr<Gtk::TreeStore> FSTreeStore;


            typedef std::set<std::string> StrSetT;

            StrSetT m_ManagedPaths;
            std::string m_VolumeUDI;
            std::string m_DeviceUDI;
            std::string m_MountPath; 


            Gtk::Button * m_Apply;
            Gtk::Button * m_Close;


            MPX::HAL & m_HAL;
            MPX::Library & m_Library;
            Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;
    };
}
#endif
