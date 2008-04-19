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

#include "mpx/main.hh"
#include "mpx/util-file.hh"
#include "libhal++/hal++.hh"

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
    : WidgetLoader<Gtk::Window>(xml, "window")
    , m_HAL(obj_hal)
    , m_Library(obj_library)
    , m_ref_xml(xml)
    {

        m_Library.signal_scan_start().connect( sigc::mem_fun( *this, &MLibManager::scan_start ));
        m_Library.signal_scan_end().connect( sigc::mem_fun( *this, &MLibManager::scan_end ));

        m_ref_xml->get_widget("volumes-cbox", m_VolumesCBox);
        Gtk::CellRendererText * cell = manage (new Gtk::CellRendererText());
        m_VolumesCBox->pack_start(*cell);
        m_VolumesCBox->add_attribute(*cell, "markup", VolumeColumns.Name);
        VolumeStore = Gtk::ListStore::create(VolumeColumns);
        m_VolumesCBox->set_model(VolumeStore);
        m_VolumesCBox->signal_changed().connect( sigc::mem_fun( *this, &MLibManager::on_volumes_cbox_changed ));
        populate_volumes();

        m_ref_xml->get_widget("fstree-view", m_FSTree);
        cell = manage (new Gtk::CellRendererText());
        Gtk::CellRendererToggle * cell2 = manage (new Gtk::CellRendererToggle());
        cell2->signal_toggled().connect( sigc::mem_fun( *this, &MLibManager::on_path_toggled ));
        Gtk::TreeViewColumn * col = manage (new Gtk::TreeViewColumn());
        col->pack_start(*cell2, false);
        col->pack_start(*cell, false);
        col->add_attribute(*cell2, "active", FSTreeColumns.Active);
        col->add_attribute(*cell, "text", FSTreeColumns.SegName);
        m_FSTree->append_column(*col);
        FSTreeStore = Gtk::TreeStore::create(FSTreeColumns);
        FSTreeStore->set_default_sort_func( sigc::mem_fun( *this, &MLibManager::fstree_sort ));
        FSTreeStore->set_sort_column( -1, Gtk::SORT_ASCENDING ); 
        m_FSTree->signal_row_expanded().connect( sigc::mem_fun( *this, &MLibManager::on_fstree_row_expanded )); 
        m_FSTree->set_model(FSTreeStore);

        m_ref_xml->get_widget("b-close", m_Close);
        //m_ref_xml->get_widget("b-apply", m_Apply);

        m_Close->signal_clicked().connect( sigc::mem_fun( *this, &MLibManager::hide ));
    }

    /* ------------------------------------------------------------------------------------------------*/

    void
    MLibManager::scan_end (gint64,gint64,gint64,gint64,gint64)
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
        if(m_VolumesCBox->get_active_row_number() == -1)
        {
            m_VolumesCBox->set_active(0);
        }

        Gtk::Window::present ();
    }

    void
    MLibManager::populate_volumes ()
    {
        static boost::format volume_label_f1 ("<b>%1% %2%</b> (%3%, %4%: '%5%')");
        static boost::format volume_label_f2 ("<b>%1% %2%</b> (%3%: '%4%')");

        Hal::RefPtr<Hal::Context> Ctx = m_HAL.get_context();

        HAL::VolumesV volumes;
        m_HAL.volumes(volumes);

        for(HAL::VolumesV::const_iterator i = volumes.begin(); i != volumes.end(); ++i)
        {
            Hal::RefPtr<Hal::Volume> Vol = *i;
            std::string vol_label = Vol->get_partition_label();
            std::string vol_mount = Vol->get_mount_point();
            
            Hal::RefPtr<Hal::Drive> Drive = Hal::Drive::create_from_udi(Ctx, Vol->get_storage_device_udi()); 
            std::string drive_model = Drive->get_model();
            std::string drive_vendor = Drive->get_vendor();
            
            TreeIter iter = VolumeStore->append(); 

            if(!vol_label.empty())
                (*iter)[VolumeColumns.Name] = (volume_label_f1 % drive_vendor % drive_model % vol_label % (_("mounted on")) % vol_mount).str();
            else
                (*iter)[VolumeColumns.Name] = (volume_label_f2 % drive_vendor % drive_model % (_("Mounted on")) % vol_mount).str();
            
            (*iter)[VolumeColumns.Volume] = Vol;
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
    } 

    void
    MLibManager::append_path (std::string const& root_path, TreeIter & root_iter)
    {
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

                            if(m_ManagedPaths.count(path))
                            {
                                (*iter)[FSTreeColumns.Active] = true;
                            }

                            prescan_path (path, iter);
                        }
                    }
                } catch (Glib::Error)
                {
                }
        }

        (*root_iter)[FSTreeColumns.WasExpanded] = true;
        if(m_ManagedPaths.count(root_path))
        {
            (*root_iter)[FSTreeColumns.Active] = true;
        }
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
            if((*iter)[FSTreeColumns.Active])
                return true;
        }

        return false;
    }

    void
    MLibManager::on_volumes_cbox_changed ()
    {
        if(m_VolumesCBox->get_active_row_number() >= 0)
        {
            TreeIter iter = m_VolumesCBox->get_active();
            Hal::RefPtr<Hal::Volume> Vol = (*iter)[VolumeColumns.Volume];
            m_VolumeUDI = Vol->get_udi();
            m_DeviceUDI = Vol->get_storage_device_udi();
            m_MountPath = Vol->get_mount_point();
            m_ManagedPaths = StrSetT();

            SQL::RowV v;
            m_Library.getSQL(v, (boost::format ("SELECT DISTINCT insert_path FROM track WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s'") % m_DeviceUDI % m_VolumeUDI).str());
            for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
            {
                SQL::Row & r = *i;
                std::string full_path = build_filename(m_MountPath, boost::get<std::string>(r["insert_path"]));
                m_ManagedPaths.insert(full_path); 
            }

            build_fstree(Vol->get_mount_point());
        }
    }

    void
    MLibManager::on_fstree_row_expanded (const Gtk::TreeIter & iter, const Gtk::TreePath & path)
    {
        GtkTreeIter children;
        bool has_children = (gtk_tree_model_iter_children(GTK_TREE_MODEL(FSTreeStore->gobj()), &children, const_cast<GtkTreeIter*>(iter->gobj())));

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
        bool active = ! bool((*iter)[FSTreeColumns.Active]);
        std::string full_path = (*iter)[FSTreeColumns.FullPath];
        
        if(!active)
        {
            MessageDialog dialog ((boost::format ("Are you sure you want to remove path '%s' from the library?") % Markup::escape_text(filename_to_utf8(full_path)).c_str()).str(), true, MESSAGE_QUESTION, BUTTONS_YES_NO, true);
            if( dialog.run() == GTK_RESPONSE_YES )
            {
                std::string insert_path = full_path.substr(m_MountPath.length());
                m_Library.execSQL((boost::format ("DELETE FROM track WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s' AND insert_path = '%s'") % m_DeviceUDI % m_VolumeUDI % insert_path).str());
                m_Library.vacuum();
                (*iter)[FSTreeColumns.Active] = active;
            }
        }
        else
        {
            if(has_active_parent(iter))
            {
                std::string full_path_parent = (*iter)[FSTreeColumns.FullPath];
                MessageDialog dialog ((boost::format ("The path '%s' is a child path of '%s' which is active; please toggle the parent to activate the child path.") % Markup::escape_text(filename_to_utf8(full_path)).c_str() % Markup::escape_text(filename_to_utf8(full_path_parent)).c_str()).str(), true, MESSAGE_INFO, BUTTONS_OK, true);
                dialog.run();
                return;
            }
            else
            {
                m_Library.scanURI(filename_to_uri(full_path));
                (*iter)[FSTreeColumns.Active] = active;
            }
        }
    }
}
