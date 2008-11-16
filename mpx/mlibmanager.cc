//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <gtkmm.h>
#include <gtk/gtk.h>
#include <glibmm/i18n.h>
#include <boost/format.hpp>

#include "mpx/mpx-main.hh"
#include "mpx/util-file.hh"
#include "mpx/util-string.hh"
#include "libhal++/hal++.hh"
#include "mpx/mpx-sql.hh"

#include "mlibmanager.hh"

using namespace Glib;
using namespace Gtk;

namespace MPX
{
    MLibManager*
    MLibManager::create (MPX::HAL & obj_hal, MPX::Library & obj_library)
    {
      const std::string path (build_filename (DATA_DIR, "glade" G_DIR_SEPARATOR_S "mlibmanager.glade"));
      MLibManager *p = new MLibManager(Gnome::Glade::Xml::create (path), obj_hal, obj_library);
      return p;
    }

    MLibManager::~MLibManager ()
    {
    }

    MLibManager::MLibManager (RefPtr<Gnome::Glade::Xml> const& xml,
                              MPX::HAL & obj_hal, MPX::Library & obj_library)
    : Gnome::Glade::WidgetLoader<Gtk::Window>(xml, "window")
    , sigx::glib_auto_dispatchable()
    , m_HAL(obj_hal)
    , m_Library(obj_library)
    , m_ref_xml(xml)
    {
        Gtk::CellRendererText * cell = 0; 
        Gtk::TreeViewColumn * col = 0; 

        m_HAL.signal_volume_added().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::on_volume_added
        ));

        m_Library.scanner().signal_scan_start().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::scan_start
        ));

        m_Library.scanner().signal_scan_end().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::scan_end
        ));

        m_ref_xml->get_widget("volumes-view", m_VolumesView);
        m_VolumesView->get_selection()->set_mode( Gtk::SELECTION_BROWSE );

        cell = manage (new Gtk::CellRendererText());
        col = manage (new Gtk::TreeViewColumn(_("Device Name")));
        col->pack_start(*cell);
        col->add_attribute(*cell, "text", VolumeColumns.Name);
        m_VolumesView->append_column(*col);


        cell = manage (new Gtk::CellRendererText());
        col = manage (new Gtk::TreeViewColumn(_("Mount Point")));
        col->pack_start(*cell);
        col->add_attribute(*cell, "text", VolumeColumns.Mountpoint);
        m_VolumesView->append_column(*col);


        VolumeStore = Gtk::ListStore::create(VolumeColumns);
        m_VolumesView->set_model(VolumeStore);
        m_VolumesView->get_selection()->signal_changed().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::on_volumes_changed
        ));
        m_VolumesView->signal_row_activated().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::on_volumes_row_activated
        ));

        populate_volumes();


        /* FSTREE VIEW */

        m_ref_xml->get_widget("fstree-view", m_FSTree);

        cell = manage (new Gtk::CellRendererText());
        Gtk::CellRendererToggle * cell2 = manage (new Gtk::CellRendererToggle());

        cell2->signal_toggled().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::on_path_toggled
        ));

        col = manage (new Gtk::TreeViewColumn());

        col->pack_start(*cell2, false);
        col->pack_start(*cell, false);

        col->set_cell_data_func(
            *cell2,
            sigc::mem_fun(
                *this,
                &MLibManager::cell_data_func_active
        ));

        col->set_cell_data_func(
            *cell,
            sigc::mem_fun(
                *this,
                &MLibManager::cell_data_func_text
        ));

        m_FSTree->append_column(*col);

        FSTreeStore = Gtk::TreeStore::create(FSTreeColumns);
        FSTreeStore->set_default_sort_func(
            sigc::mem_fun(
                *this,
                &MLibManager::fstree_sort
        ));
        FSTreeStore->set_sort_column(
            -1,
            Gtk::SORT_ASCENDING
        ); 

        m_FSTree->signal_row_expanded().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::on_fstree_row_expanded
        )); 

        m_FSTree->get_selection()->set_mode(
            Gtk::SELECTION_NONE
        ); 

        m_FSTree->set_model(FSTreeStore);

        /* BUTTONS */

        m_ref_xml->get_widget("b-close", m_Close);
        m_Close->signal_clicked().connect(
            sigc::mem_fun(
            *this,
            &MLibManager::hide
        ));

        m_ref_xml->get_widget("b-rescan", m_Rescan);
        m_Rescan->signal_clicked().connect(
            sigc::mem_fun(
            *this,
            &MLibManager::on_rescan_volume
        ));
        m_Rescan->set_sensitive( false );

        m_ref_xml->get_widget("b-deep-rescan", m_DeepRescan);
        m_DeepRescan->signal_clicked().connect(
            sigc::mem_fun(
            *this,
            &MLibManager::on_rescan_volume_deep
        ));
        m_DeepRescan->set_sensitive( false );

        m_ref_xml->get_widget("b-vacuum", m_Vacuum);
        m_Vacuum->signal_clicked().connect(
            sigc::mem_fun(
            *this,
            &MLibManager::on_vacuum_volume
        ));
        m_Vacuum->set_sensitive( false );
    }

    /* ------------------------------------------------------------------------------------------------*/

    void
    MLibManager::scan_end( ScanSummary const& )
    {
        Gtk::Widget::set_sensitive(true);
    }
    
    void
    MLibManager::scan_start ()
    {
        Gtk::Widget::set_sensitive(false);
    }

    void
    MLibManager::hide ()
    {
        Gtk::Widget::hide();
    }

    void
    MLibManager::present ()
    {
        Gtk::Window::present ();
    }

    void
    MLibManager::on_volume_removed (const HAL::Volume& volume)
    {
    } 

    void
    MLibManager::on_volume_added (const HAL::Volume& volume)
    {
        static boost::format volume_label_f1 ("%1%: %2%");

        try{
            Hal::RefPtr<Hal::Context> Ctx = m_HAL.get_context();
            Hal::RefPtr<Hal::Volume> Vol = Hal::Volume::create_from_udi(Ctx, volume.volume_udi); 
            std::string vol_label = Vol->get_partition_label();
            std::string vol_mount = Vol->get_mount_point();
            
            Hal::RefPtr<Hal::Drive> Drive = Hal::Drive::create_from_udi(Ctx, Vol->get_storage_device_udi()); 
            std::string drive_model  = Drive->get_model();
            std::string drive_vendor = Drive->get_vendor();
            
            TreeIter iter = VolumeStore->append(); 

            if(!vol_label.empty())
                (*iter)[VolumeColumns.Name] = (volume_label_f1 % drive_model % vol_label).str();
            else
                (*iter)[VolumeColumns.Name] = drive_model; 
            
            (*iter)[VolumeColumns.Mountpoint] = vol_mount; 
            (*iter)[VolumeColumns.Volume] = Vol;
        } catch(...) {
            MessageDialog dialog (_("An error occured trying to display the volume"));
            dialog.run();
        }
    } 

    void
    MLibManager::populate_volumes ()
    {
        static boost::format volume_label_f1 ("%1%: %2%");

        try { 
                Hal::RefPtr<Hal::Context> Ctx = m_HAL.get_context();
                HAL::VolumesV volumes;
                m_HAL.volumes(volumes);

                for(HAL::VolumesV::const_iterator i = volumes.begin(); i != volumes.end(); ++i)
                {
                    Hal::RefPtr<Hal::Volume> Vol = *i;
                    std::string vol_label = Vol->get_partition_label();
                    std::string vol_mount = Vol->get_mount_point();
                    
                    Hal::RefPtr<Hal::Drive> Drive = Hal::Drive::create_from_udi(Ctx, Vol->get_storage_device_udi()); 
                    std::string drive_model  = Drive->get_model();
                    std::string drive_vendor = Drive->get_vendor();
                    
                    TreeIter iter = VolumeStore->append(); 

                    if(!vol_label.empty())
                        (*iter)[VolumeColumns.Name] = (volume_label_f1 % drive_model % vol_label).str();
                    else
                        (*iter)[VolumeColumns.Name] = drive_model; 
                    
                    (*iter)[VolumeColumns.Mountpoint] = vol_mount; 
                    (*iter)[VolumeColumns.Volume] = Vol;
                }
        } catch(...) {
            MessageDialog dialog (_("An error occured trying to populate volumes"));
            dialog.run();
        }
    }

    int
    MLibManager::fstree_sort (const TreeIter & iter_a, const TreeIter & iter_b)
    {
        Glib::ustring a = (*iter_a)[FSTreeColumns.SegName];
        Glib::ustring b = (*iter_b)[FSTreeColumns.SegName];
        return a.compare(b);
    }

    void
    MLibManager::prescan_path (std::string const& scan_path, TreeIter & iter)
    {
        try{
                Glib::Dir dir (scan_path);
                std::vector<std::string> strv (dir.begin(), dir.end());
                dir.close ();

                for(std::vector<std::string>::const_iterator i = strv.begin(); i != strv.end(); ++i)
                {
                    std::string path = build_filename(scan_path, *i);
                    if((i->operator[](0) != '.')
                      && file_test(path, FILE_TEST_IS_DIR))
                    {
                        if(!m_HAL.path_is_mount_path(path))
                        {
                            FSTreeStore->append(iter->children());
                            return;
                        }
                    }
                }
        } catch( Glib::FileError ) {
        }
    } 

    void
    MLibManager::append_path (std::string const& root_path, TreeIter & root_iter)
    {
        try{
                Glib::Dir dir (root_path);
                std::vector<std::string> strv (dir.begin(), dir.end());
                dir.close ();

                for(std::vector<std::string>::const_iterator i = strv.begin(); i != strv.end(); ++i)
                {
                    std::string path = build_filename(root_path, *i);
                    if(i->operator[](0) != '.')
                        try{
                            if(file_test(path, FILE_TEST_IS_DIR))
                            {
                                if(!m_HAL.path_is_mount_path(path))
                                {
                                    TreeIter iter = FSTreeStore->append(root_iter->children());
                                    (*iter)[FSTreeColumns.SegName] = *i; 
                                    (*iter)[FSTreeColumns.FullPath] = path;
                                    (*iter)[FSTreeColumns.WasExpanded] = false;
                                    prescan_path (path, iter);
                                }
                            }
                        } catch (Glib::Error)
                        {
                        }
                }
        } catch( Glib::FileError ) {
        }

        (*root_iter)[FSTreeColumns.WasExpanded] = true;
    }

    void
    MLibManager::build_fstree (std::string const& root_path)
    {
        FSTreeStore->clear ();
        TreeIter root_iter = FSTreeStore->append();
        FSTreeStore->append(root_iter->children());
        (*root_iter)[FSTreeColumns.SegName] = root_path;
        (*root_iter)[FSTreeColumns.FullPath] = root_path;
    }

    bool
    MLibManager::has_active_parent(Gtk::TreeIter & iter)
    {
        TreePath path = FSTreeStore->get_path(iter);

        while(path.up())
        {
            iter = FSTreeStore->get_iter(path);
            std::string full_path = ((*iter)[FSTreeColumns.FullPath]);
            if(m_ManagedPaths.count(full_path))
                return true;
        }
        return false;
    }

    void
    MLibManager::recreate_path_frags ()
    {
        m_ManagedPathFrags = PathFragsV();
        for(StrSetT::const_iterator i = m_ManagedPaths.begin(); i != m_ManagedPaths.end(); ++i)
        {
            char ** path_frags = g_strsplit((*i).c_str(), "/", -1);
            char ** path_frags_p = path_frags;
            m_ManagedPathFrags.resize(m_ManagedPathFrags.size()+1);
            PathFrags & frags = m_ManagedPathFrags.back();
            while (*path_frags)
            {
                frags.push_back(*path_frags);
                ++path_frags;
            }
            g_strfreev(path_frags_p);
        }
    }

    void
    MLibManager::on_volumes_row_activated (const Gtk::TreePath&, Gtk::TreeViewColumn*)
    {
        on_rescan_volume ();
    }

    void
    MLibManager::on_volumes_changed ()
    {
        bool has_selection = m_VolumesView->get_selection()->count_selected_rows();

        m_Rescan->set_sensitive( has_selection );
        m_DeepRescan->set_sensitive( has_selection );
        m_Vacuum->set_sensitive( has_selection );

        if( has_selection )
        {
            TreeIter iter = m_VolumesView->get_selection()->get_selected();

            Hal::RefPtr<Hal::Volume> Vol = (*iter)[VolumeColumns.Volume];

            m_VolumeUDI     = Vol->get_udi();
            m_DeviceUDI     = Vol->get_storage_device_udi();
            m_MountPoint    = Vol->get_mount_point();
            m_ManagedPaths  = StrSetT();

            SQL::RowV v;
            m_Library.getSQL(
                    v,
                    (boost::format ("SELECT DISTINCT insert_path FROM track WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s'")
                        % m_DeviceUDI
                        % m_VolumeUDI
                    ).str()
            );

            for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
            {
                m_ManagedPaths.insert(build_filename(m_MountPoint, boost::get<std::string>((*i)["insert_path"])));
            }

            recreate_path_frags ();
            build_fstree(Vol->get_mount_point());
            TreePath path (1);
            m_FSTree->expand_row(path, false);
        }
    }

    void
    MLibManager::on_fstree_row_expanded (const Gtk::TreeIter & iter, const Gtk::TreePath & path)
    {
        if( (*iter)[FSTreeColumns.WasExpanded] )
            return;

        GtkTreeIter children;

        bool has_children = (
            gtk_tree_model_iter_children(
                GTK_TREE_MODEL(FSTreeStore->gobj()),
                &children,
                const_cast<GtkTreeIter*>(iter->gobj())
        ));

        std::string full_path = (*iter)[FSTreeColumns.FullPath];
        TreeIter iter_copy = iter;

        append_path(full_path, iter_copy);

        if(has_children)
        {
            gtk_tree_store_remove(GTK_TREE_STORE(FSTreeStore->gobj()), &children);
        }
        else
            g_warning("%s:%d : No placeholder row present, state seems corrupted.", __FILE__, __LINE__);
    }

    void
    MLibManager::on_path_toggled (const Glib::ustring & path_str)
    {
        TreeIter iter = FSTreeStore->get_iter(path_str);
        TreeIter iter_copy = iter;
        std::string full_path = (*iter)[FSTreeColumns.FullPath];

        if(has_active_parent(iter))
            return;
        
        if(m_ManagedPaths.count(full_path))
        {
            MessageDialog dialog(
                (boost::format ("<b>Are you sure you want remove</b> Music from this path from the Library:\n\n'<b>%s</b>'")
                    % Markup::escape_text(filename_to_utf8(full_path)).c_str()
                ).str(),
                true,
                MESSAGE_QUESTION,
                BUTTONS_YES_NO,
                true
            );

            if( dialog.run() == GTK_RESPONSE_YES )
            {
                std::string insert_path = full_path.substr(m_MountPoint.length());
                m_Library.execSQL(mprintf("DELETE FROM track WHERE hal_device_udi = '%q' AND hal_volume_udi = '%q' AND insert_path = '%q'", m_DeviceUDI.c_str(), m_VolumeUDI.c_str(), insert_path.c_str()));
                m_Library.vacuum();
                m_ManagedPaths.erase(full_path);
                recreate_path_frags ();
                TreePath path = FSTreeStore->get_path(iter_copy); 
                FSTreeStore->row_changed(path, iter_copy);
            }
        }
        else
        {
            MessageDialog dialog(
                (boost::format ("Are you sure you want <b>add</b> Music <b>from this path</b> to the Library:\n\n'<b>%s</b>'")
                    % Markup::escape_text(filename_to_utf8(full_path)).c_str()
                ).str(),
                true,
                MESSAGE_QUESTION,
                BUTTONS_YES_NO,
                true
            );

            if( dialog.run() == GTK_RESPONSE_YES )
            {
                    m_ManagedPaths.insert(full_path);
                    recreate_path_frags ();
                    TreePath path = FSTreeStore->get_path(iter_copy); 
                    FSTreeStore->row_changed(path, iter_copy);
                    StrV v;
                    v.push_back(filename_to_uri(full_path));
                    m_Library.initScan(v, true);
            }
        }
    }

    void
    MLibManager::on_rescan_volume ()
    {
        StrV v;
        for(StrSetT::const_iterator i = m_ManagedPaths.begin(); i != m_ManagedPaths.end(); ++i)
        {
            //v.push_back(filename_to_uri(build_filename(m_MountPoint, *i)));
            v.push_back(filename_to_uri(*i));
        }
        m_Library.initScan(v);
    }

    void
    MLibManager::on_rescan_volume_deep ()
    {
        StrV v;
        for(StrSetT::const_iterator i = m_ManagedPaths.begin(); i != m_ManagedPaths.end(); ++i)
        {
            //v.push_back(filename_to_uri(build_filename(m_MountPoint, *i)));
            v.push_back(filename_to_uri(*i));
        }
        m_Library.initScan(v, true);
    }

    void
    MLibManager::on_vacuum_volume ()
    {
        m_Library.vacuum_volume(m_DeviceUDI, m_VolumeUDI);
    }

    void
    MLibManager::cell_data_func_active (CellRenderer * basecell, TreeIter const& iter)
    {
        CellRendererToggle & cell = *(dynamic_cast<CellRendererToggle*>(basecell));

        TreeIter iter_copy = iter;

        std::string full_path = (*iter)[FSTreeColumns.FullPath];
        if(m_ManagedPaths.count(full_path))
            cell.property_active() = true; 
        else
            cell.property_active() = false; 
    }

    void
    MLibManager::cell_data_func_text (CellRenderer * basecell, TreeIter const& iter)
    {
        CellRendererText & cell = *(dynamic_cast<CellRendererText*>(basecell));

        TreePath path = FSTreeStore->get_path(iter); 
        if(path.size() < 1)
            return;

        Glib::ustring segName = (*iter)[FSTreeColumns.SegName];

        for(PathFragsV::const_iterator i = m_ManagedPathFrags.begin(); i != m_ManagedPathFrags.end(); ++i)
        {
            PathFrags const& frags = *i;

            if(!(frags.size() < path.size()))
            {
                std::string const& frag = frags[path.size()-1];
                if( frag == segName ) 
                {
                    cell.property_markup() = (boost::format("<span foreground='#0000ff'><b>%s</b></span>") % Markup::escape_text(segName)).str();
                    return;
                }
            }
        }

        TreeIter iter_copy = iter;
        if(has_active_parent(iter_copy))
            cell.property_markup() = (boost::format("<span foreground='#a0a0a0'>%s</span>") % Markup::escape_text(segName)).str();
        else
            cell.property_text() = segName; 
    }

    bool
    MLibManager::slot_select (const Glib::RefPtr<Gtk::TreeModel>&model, const Gtk::TreePath &path, bool was_selected)
    {
        TreeIter iter = FSTreeStore->get_iter(path);
        return !(has_active_parent(iter));
    }
}
