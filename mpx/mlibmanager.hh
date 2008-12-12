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

// FIXME: Must be here for some damn reason
#include <giomm.h>

#include "mpx/widgets/widgetloader.hh"
#include "mpx/mpx-hal.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-services.hh"

#include "libhal++/hal++.hh"

#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/window.h>
#include <libglademm/xml.h>
#include <sigx/sigx.h>

namespace MPX
{
    enum PathTestResult
    {
        IS_PRESENT,
        RELOCATED,
        DELETED,
        IGNORED
    };

    class MLibManager
      : public Gnome::Glade::WidgetLoader<Gtk::Window>
      , public sigx::glib_auto_dispatchable
      , public Service::Base
    {
        public:

            static MLibManager* create (MPX::HAL & obj_hal, MPX::Library & obj_library);
            virtual ~MLibManager ();

            void
            present ();

            void
            hide ();

            bool
            is_present();

            void
            rescan_all_volumes (bool /*deep*/ = false);

            void
            push_message (const std::string&);

        private:

            MLibManager (Glib::RefPtr<Gnome::Glade::Xml> const& xml, MPX::HAL & obj_hal, MPX::Library & obj_library);

            void
            scan_end(
            );

            void
            scan_summary(
                const ScanSummary&
            );

            void
            scan_start(
            );

            void
            scan_run(
                gint64,
                bool
            );

            void
            populate_volumes ();

            void
            build_fstree(
                const std::string&
            );

            int
            fstree_sort(
                const Gtk::TreeIter&,
                const Gtk::TreeIter&
            );

            void
            append_path(
                const std::string&,
                Gtk::TreeIter&
            );

            void
            prescan_path(
                const std::string&,
                Gtk::TreeIter& iter
            );

            bool
            has_active_parent(
                Gtk::TreeIter&
            );

            PathTestResult
            path_test(
                const std::string&
            );

            void
            on_volumes_changed ();

            void
            on_fstree_row_expanded(
                const Gtk::TreeIter&,
                const Gtk::TreePath&
            );

            void
            on_path_toggled(
                const Glib::ustring&
            );

            void
            on_rescan_volume(bool /*deep*/);

            void
            on_vacuum_volume ();

            void
            on_volume_added(
                const HAL::Volume&
            );

            void
            on_volume_removed(
                const HAL::Volume&
            );

            // auto rescanning
            bool
            on_rescan_timeout();

            void
            on_rescan_in_intervals_changed (MCS_CB_DEFAULT_SIGNATURE);

            // ui stuff
            Gtk::TreeView * m_VolumesView;

            struct VolumeColumnsT : public Gtk::TreeModel::ColumnRecord
            {
                Gtk::TreeModelColumn<Glib::ustring>                 Name;
                Gtk::TreeModelColumn<Glib::ustring>                 Mountpoint;
                Gtk::TreeModelColumn<Hal::RefPtr<Hal::Volume> >     Volume;
                Gtk::TreeModelColumn<Glib::RefPtr<Gio::Volume> >    GioVolume;

                VolumeColumnsT ()
                {
                    add (Name);
                    add (Mountpoint);
                    add (Volume);
                    add (GioVolume);
                };
            };

            VolumeColumnsT VolumeColumns;
            Glib::RefPtr<Gtk::ListStore> VolumeStore;



            Gtk::TreeView * m_FSTree;

            struct FSTreeColumnsT : public Gtk::TreeModel::ColumnRecord
            {
                Gtk::TreeModelColumn<Glib::ustring>   SegName;
                Gtk::TreeModelColumn<std::string>     FullPath;
                Gtk::TreeModelColumn<bool>            WasExpanded;

                FSTreeColumnsT ()
                {
                    add (SegName);
                    add (FullPath);
                    add (WasExpanded);
                };
            };

            FSTreeColumnsT FSTreeColumns;
            Glib::RefPtr<Gtk::TreeStore> FSTreeStore;


            void
            cell_data_func_active(
                Gtk::CellRenderer*,
                const Gtk::TreeIter&
            );

            void
            cell_data_func_text(
                Gtk::CellRenderer*,
                const Gtk::TreeIter&
            );

            bool
            slot_select(
                const Glib::RefPtr<Gtk::TreeModel>&,
                const Gtk::TreePath&,
                bool was_selected
            );

            void
            recreate_path_frags ();

            typedef std::set<std::string>       StrSetT;
            typedef std::vector<std::string>    PathFrags;
            typedef std::vector<PathFrags>      PathFragsV;

            StrSetT     m_ManagedPaths;
            PathFragsV  m_ManagedPathFrags;

            std::string m_VolumeUDI;  // holds current VUDI
            std::string m_DeviceUDI;  //     - " -     DUDI
            std::string m_MountPoint; //     - " -     mount point

            Gtk::Button     * m_Close;
            Gtk::Button     * m_Rescan;
            Gtk::Button     * m_DeepRescan;
            Gtk::Button     * m_Vacuum;
            Gtk::Statusbar  * m_Statusbar;
            Gtk::Widget     * m_VboxInner;
            Glib::Timer       m_rescan_timer;
    
            MPX::HAL        & m_HAL;
            MPX::Library    & m_Library;

            Glib::RefPtr<Gtk::TextBuffer>   m_TextBufferDetails;

            Glib::RefPtr<Gtk::UIManager>    m_ui_manager;
            Glib::RefPtr<Gtk::ActionGroup>  m_actions;

            void
            on_mlib_remove_dupes();

            void
            on_mlib_rescan_library();

            void
            on_mlib_vacuum_library();

            void
            on_volume_rescan_volume();

            void
            on_volume_vacuum_volume();
    };
}
#endif
