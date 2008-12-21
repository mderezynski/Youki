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

#include "libhal++/hal++.hh"

#include "mpx/mpx-main.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/mpx-uri.hh"

#include "mpx/widgets/task-dialog.hh"
#include "mpx/widgets/timed-confirmation.hh"

#include "mpx/util-file.hh"
#include "mpx/util-string.hh"
#include "mpx/util-ui.hh"

#include "import-share.hh"
#include "import-folder.hh"
#include "request-value.hh"

#include "mlibmanager.hh"

using namespace Glib;
using namespace Gtk;
using boost::get;

namespace
{
        char MenubarMLibMan [] =

                "<ui>"
                "<menubar name='MenubarMLibMan'>"
                "   <menu action='MenuMLib'>"
                "         <menuitem action='action-import-folder'/>"
                "         <menuitem action='action-import-share'/>"
                "         <separator/>"
                "         <menuitem action='action-mlib-remove-dupes'/>"
                "         <menuitem action='action-mlib-rescan'/>"
                "         <menuitem action='action-mlib-rescan-deep'/>"
                "         <menuitem action='action-mlib-vacuum'/>"
                "         <separator/>"
                "         <menuitem action='action-mlib-update-statistics'/>"
                "         <menuitem action='action-mlib-recache-covers'/>"
                "         <separator/>"
                "         <menuitem action='action-close'/>"
                "   </menu>"
#ifdef HAVE_HAL
                "   <menu action='MenuVolume'>"
                "         <menuitem action='action-volume-rescan'/>"
                "         <menuitem action='action-volume-rescan-deep'/>"
                "         <menuitem action='action-volume-vacuum'/>"
                "   </menu>"
#endif
                "</menubar>"
                "</ui>"
                "";
}

namespace MPX
{
    MLibManager*
    MLibManager::create(
    )
    {
        const std::string path (build_filename (DATA_DIR, "glade" G_DIR_SEPARATOR_S "mlibmanager.glade"));
        MLibManager *p = new MLibManager(
            Gnome::Glade::Xml::create(path)
        );
        return p;
    }

    MLibManager::~MLibManager ()
    {
        Gtk::Window::get_position( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-mlib-x")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-mlib-y")));
        Gtk::Window::get_size( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-mlib-w")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-mlib-h")));
    }

    void
    MLibManager::update_filestats()
    {
        using namespace MPX::SQL;

        m_FileStats_Store->clear();

        RowV v1;
        services->get<Library>("mpx-service-library")->getSQL(
              v1
            , "SELECT DISTINCT type, count(*) AS cnt FROM track GROUP BY type"
        );

        for( RowV::iterator i = v1.begin(); i != v1.end(); ++i )
        {
            TreeIter iter = m_FileStats_Store->append();
            (*iter)[m_FileStats_Columns.Type]       = get<std::string>((*i)["type"]);
            (*iter)[m_FileStats_Columns.Count]      = get<gint64>((*i)["cnt"]);

            RowV v2; 
            services->get<Library>("mpx-service-library")->getSQL(
                  v2
                , (boost::format("SELECT bitrate AS rate FROM track WHERE type = '%s' AND bitrate != '0' ORDER BY bitrate DESC LIMIT 1") % get<std::string>((*i)["type"])).str()
            );
            (*iter)[m_FileStats_Columns.HighBitrate] = (boost::format("%lld kbit/s") % get<gint64>(v2[0]["rate"])).str();

            v2.clear();
            services->get<Library>("mpx-service-library")->getSQL(
                  v2
                , (boost::format("SELECT bitrate AS rate FROM track WHERE type = '%s' AND bitrate != '0' ORDER BY bitrate ASC LIMIT 1") % get<std::string>((*i)["type"])).str()
            );
            (*iter)[m_FileStats_Columns.LowBitrate] = (boost::format("%lld kbit/s") % get<gint64>(v2[0]["rate"])).str();
        }

        Gtk::Label * label;

        m_Xml->get_widget( "label-count-of-tracks", label );
        v1.clear();
        services->get<Library>("mpx-service-library")->getSQL(
              v1
            , "SELECT DISTINCT count(*) AS cnt FROM track"
        );
        label->set_text((boost::format("%lld") % get<gint64>(v1[0]["cnt"])).str());

        m_Xml->get_widget( "label-count-of-albums", label );
        v1.clear();
        services->get<Library>("mpx-service-library")->getSQL(
              v1
            , "SELECT DISTINCT count(*) AS cnt FROM album"
        );
        label->set_text((boost::format("%lld") % get<gint64>(v1[0]["cnt"])).str());

        m_Xml->get_widget( "label-library-playtime", label );
        v1.clear();
        services->get<Library>("mpx-service-library")->getSQL(
              v1
            , "SELECT sum(time) AS time FROM track"
        );
        gint64 total = get<gint64>(v1[0]["time"]) ;
        gint64 days    = (total / 86400) ;
        gint64 hours   = (total % 86400) / 3600;
        gint64 minutes = ((total % 86400) % 3600) / 60 ;
        gint64 seconds = ((total % 86400) % 3600) % 60 ;
        label->set_text((boost::format(_("%lld Days, %lld Hours, %lld Minutes, %lld Seconds")) % days % hours % minutes % seconds).str());
    }

    MLibManager::MLibManager(
        const RefPtr<Gnome::Glade::Xml>& xml
    )
    : Gnome::Glade::WidgetLoader<Gtk::Window>(xml, "window")
    , sigx::glib_auto_dispatchable()
    , Service::Base("mpx-service-mlibman")
    , m_InnerdialogMainloop(0)
    {
       /*- Widgets -------------------------------------------------------*/ 

        m_Xml->get_widget("treeview-filestats", m_FileStats_View );

        m_FileStats_Store = Gtk::ListStore::create( m_FileStats_Columns ) ;

        m_FileStats_View->append_column(
              _("Type")
            , m_FileStats_Columns.Type
        );

        m_FileStats_View->append_column(
              _("Track Count")
            , m_FileStats_Columns.Count
        );

        m_FileStats_View->append_column(
              _("Highest Bitrate")
            , m_FileStats_Columns.HighBitrate
        );

        m_FileStats_View->append_column(
              _("Lowest Bitrate")
            , m_FileStats_Columns.LowBitrate
        );


        m_FileStats_View->set_model( m_FileStats_Store );

        m_Xml->get_widget("statusbar", m_Statusbar );
        m_Xml->get_widget("vbox-inner", m_VboxInner );

        m_Xml->get_widget("innerdialog-hbox", m_InnerdialogHBox );
        m_Xml->get_widget("innerdialog-label", m_InnerdialogLabel );
        m_Xml->get_widget("innerdialog-yes", m_InnerdialogYes );
        m_Xml->get_widget("innerdialog-cancel", m_InnerdialogCancel );

        m_InnerdialogYes->signal_clicked().connect(
            sigc::bind(
                sigc::mem_fun(
                        *this,
                        &MLibManager::innerdialog_response
                ),
                GTK_RESPONSE_YES
        ));


        m_InnerdialogCancel->signal_clicked().connect(
            sigc::bind(
                sigc::mem_fun(
                        *this,
                        &MLibManager::innerdialog_response
                ),
                GTK_RESPONSE_CANCEL
        ));

        /*- Details Textview/Buffer ---------------------------------------*/ 

        m_TextBufferDetails = (dynamic_cast<Gtk::TextView*>(m_Xml->get_widget("textview-details")))->get_buffer();

#ifdef HAVE_HAL
        /*- Volumes View --------------------------------------------------*/ 

        Gtk::CellRendererText * cell = 0; 
        Gtk::TreeViewColumn * col = 0; 

        services->get<HAL>("mpx-service-hal")->signal_volume_added().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::on_volume_added
        ));

        services->get<Library>("mpx-service-library")->scanner()->connect().signal_scan_start().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::scan_start
        ));

        services->get<Library>("mpx-service-library")->scanner()->connect().signal_scan_end().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::scan_end
        ));

        services->get<Library>("mpx-service-library")->scanner()->connect().signal_scan_summary().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::scan_summary
        ));

        services->get<Library>("mpx-service-library")->scanner()->connect().signal_scan_run().connect(
            sigc::mem_fun(
                *this,
                &MLibManager::scan_run
        ));

        m_Xml->get_widget("volumes-view", m_VolumesView);
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
            sigc::hide( sigc::hide (
            sigc::mem_fun(
                *this,
                &MLibManager::on_volumes_changed
        ))));
#endif

#ifdef HAVE_HAL
        /*- FSTree --------------------------------------------------------*/ 

        m_Xml->get_widget("fstree-view", m_FSTree);

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
#endif

        /*- Buttons -------------------------------------------------------*/ 

        m_Xml->get_widget("b-close", m_Close);

#ifdef HAVE_HAL
        m_Xml->get_widget("b-rescan", m_Rescan);
        m_Xml->get_widget("b-rescan-deep", m_DeepRescan);
        m_Xml->get_widget("b-vacuum", m_Vacuum);
#endif

        /*- Rescanning ----------------------------------------------------*/ 

        if( mcs->key_get<bool>("library","rescan-at-startup") )
        {
            rescan_all_volumes();
        }

        mcs->subscribe(
          "library",
          "rescan-in-intervals",
          sigc::mem_fun(
              *this,
              &MLibManager::on_library_rescan_in_intervals_changed
        ));

#ifdef HAVE_HAL
        mcs->subscribe(
          "library",
          "use-hal",
          sigc::mem_fun(
              *this,
              &MLibManager::on_library_use_hal_changed
        ));
#endif

        sigc::slot<bool> slot = sigc::mem_fun(*this, &MLibManager::on_rescan_timeout);
        sigc::connection conn = Glib::signal_timeout().connect(slot, 1000);
        m_RescanTimer.start();

        /*- Actions -------------------------------------------------------*/ 

        m_UIManager    = UIManager::create ();
        m_Actions       = ActionGroup::create ("Actions_MLibMan");

        m_Actions->add(Action::create(
            "MenuMLib",
            _("_Music Library")
        ));

#ifdef HAVE_HAL
        m_Actions->add(Action::create(
            "MenuVolume",
            _("_Volume")
        ));
#endif

        m_Actions->add( Action::create(
            "action-mlib-remove-dupes",
            _("_Remove Duplicates")),
            sigc::mem_fun(
                *this,
                &MLibManager::on_mlib_remove_dupes
        ));

        m_Actions->add( Action::create(
            "action-mlib-rescan",
            Gtk::Stock::REFRESH,
            _("_Find New Files")),
            sigc::bind(
                    sigc::mem_fun(
                        *this,
                        &MLibManager::rescan_all_volumes
                    ),
                    false
        ));

        m_Actions->add( Action::create(
            "action-mlib-rescan-deep",
            Gtk::Stock::HARDDISK,
            _("_Update Existing & Remove Stale Files")),
            sigc::bind(
                    sigc::mem_fun(
                        *this,
                        &MLibManager::rescan_all_volumes
                    ),
                    true
        ));

        m_Actions->add( Action::create(
            "action-mlib-vacuum",
            Gtk::Stock::UNDO,
            _("_Clean Up")),
            sigc::mem_fun(
                *this,
                &MLibManager::on_vacuum_all
        ));

        m_Actions->add( Action::create(
            "action-mlib-update-statistics",
            Gtk::Stock::FIND,
            _("_Update Additional Metadata")),
            sigc::mem_fun(
                *this,
                &MLibManager::on_update_statistics
        ));

        m_Actions->add( Action::create(
            "action-mlib-recache-covers",
            _("Refresh _Covers")),
            sigc::mem_fun(
                *this,
                &MLibManager::on_recache_covers
        ));

#ifdef HAVE_HAL
        m_Actions->add( Action::create(

            "action-volume-rescan",

            Gtk::Stock::REFRESH,
            _("_Find New Files")),
            sigc::bind(
                    sigc::mem_fun(
                        *this,
                        &MLibManager::on_rescan_volume
                    ),
                    false
        ));

        m_Actions->add( Action::create(

            "action-volume-rescan-deep",

            Gtk::Stock::HARDDISK,
            _("_Update Existing & Remove Stale Files")),
            sigc::bind(
                    sigc::mem_fun(
                        *this,
                        &MLibManager::on_rescan_volume
                    ),
                    true
        ));

        m_Actions->add( Action::create(
            "action-volume-vacuum",
            Gtk::Stock::UNDO,
            _("_Clean Up")),
            sigc::mem_fun(
                *this,
                &MLibManager::on_vacuum_volume
        ));
#endif

        m_Actions->add( Action::create(

                          "action-import-folder",

                          Gtk::Stock::HARDDISK,
                          _("Import _Folder")),
                          sigc::mem_fun (*this, &MLibManager::on_import_folder));

        m_Actions->add( Action::create( 

                          "action-import-share",

                          Gtk::Stock::NETWORK,
                          _("Import _Share")),
                          sigc::mem_fun (*this, &MLibManager::on_import_share));


        m_Actions->add( Action::create(
            "action-close",
            Gtk::Stock::CLOSE,
            _("_Close")),
            sigc::mem_fun(
                *this,
                &MLibManager::hide
        ));

#ifdef HAVE_HAL
        m_Actions->get_action("action-volume-rescan")->set_sensitive( false );
        m_Actions->get_action("action-volume-rescan-deep")->set_sensitive( false );
        m_Actions->get_action("action-volume-vacuum")->set_sensitive( false );

        m_Actions->get_action("action-volume-rescan")->connect_proxy( *m_Rescan );
        m_Actions->get_action("action-volume-rescan-deep")->connect_proxy( *m_DeepRescan );
        m_Actions->get_action("action-volume-vacuum")->connect_proxy( *m_Vacuum );
#endif

        m_Actions->get_action("action-close")->connect_proxy( *m_Close );

        m_UIManager->insert_action_group(m_Actions);

        guint dummy;

        if(
            Util::ui_manager_add_ui(
                  m_UIManager
                , MenubarMLibMan
                , *this
                , _("MLibMan Menubar")
                , dummy
            )
        )
        {
            dynamic_cast<Alignment*>(
                m_Xml->get_widget("alignment-menu")
            )->add(
                *(m_UIManager->get_widget ("/MenubarMLibMan"))
            );

            update_filestats();
        }

        /*- Init the UI ---------------------------------------------------*/ 

#ifdef HAVE_HAL
        if( mcs->key_get<bool>("library", "use-hal"))
        {
            populate_volumes();
            m_Actions->get_action("action-import-folder")->set_visible( false );
            m_Actions->get_action("action-import-share")->set_visible( false );
        }
        else
#endif
        {
            m_Xml->get_widget("notebook1")->hide();
        }

        /*- Setup Window Geometry -----------------------------------------*/ 
    
        gtk_widget_realize(GTK_WIDGET(gobj()));

        resize(
           mcs->key_get<int>("mpx","window-mlib-w"),
           mcs->key_get<int>("mpx","window-mlib-h")
        );

        move(
            mcs->key_get<int>("mpx","window-mlib-x"),
            mcs->key_get<int>("mpx","window-mlib-y")
        );
    }

#ifdef HAVE_HAL
    void
    MLibManager::on_library_use_hal_changed (MCS_CB_DEFAULT_SIGNATURE)
    {
        bool use_hal = mcs->key_get<bool>("library","use-hal");

        if( use_hal )
        {
            populate_volumes(); 
            m_Xml->get_widget("notebook1")->show();
            m_Actions->get_action("action-import-folder")->set_visible( false );
            m_Actions->get_action("action-import-share")->set_visible( false );
            set_resizable( true );
        }
        else
        {
            clear_volumes();
            m_Xml->get_widget("notebook1")->hide();
            m_Actions->get_action("action-import-folder")->set_visible( true );
            m_Actions->get_action("action-import-share")->set_visible( true );
            set_resizable( false );
        }

        m_Actions->get_action("MenuVolume")->set_sensitive( use_hal ); 
        m_Actions->get_action("action-mlib-rescan")->set_visible( use_hal );
    }
#endif

    void
    MLibManager::on_library_rescan_in_intervals_changed (MCS_CB_DEFAULT_SIGNATURE)
    {
        m_RescanTimer.reset();
    }

    void
    MLibManager::innerdialog_response(
        int response
    )
    {
        m_InnerdialogResponse = response;
        m_InnerdialogMainloop->quit();
    }

    /* ------------------------------------------------------------------------------------------------*/

    void
    MLibManager::scan_end ()
    {
        update_filestats();
        m_VboxInner->set_sensitive(true);
    }

    void
    MLibManager::scan_summary( ScanSummary const& summary )
    {
        time_t curtime = time(NULL);
        struct tm ctm;
        localtime_r(&curtime, &ctm);

        char bdate[64];
        strftime(bdate, 64, "%H:%M:%S", &ctm);

        m_Statusbar->pop();        
        m_Statusbar->push((boost::format(
            _("Library Scan finished at %1% (%2% %3% scanned, %4% Files added, %5% Files up to date, %6% updated, %7% erroneous, see log)"))
            % bdate 
            % summary.FilesTotal
            % (summary.DeepRescan ? _("Files") : _("Folders"))
            % summary.FilesAdded
            % summary.FilesUpToDate
            % summary.FilesUpdated
            % summary.FilesErroneous
        ).str());

        services->get<Library>("mpx-service-library")->execSQL((boost::format ("UPDATE meta SET last_scan_date = %lld WHERE rowid = 1") % (gint64(time(NULL)))).str());

        m_TextBufferDetails->set_text("");
        Glib::ustring text;
        if( !summary.FileListErroneous.empty() )
        {
                text.append("Erroneous Files:\n");
                for(std::vector<SSFileInfo>::const_iterator i = summary.FileListErroneous.begin(); i != summary.FileListErroneous.end(); ++i) 
                {
                    text.append((boost::format ("\t%1%:  %2%\n") % (*i).second % (*i).first).str());
                }
        }

        if( !summary.FileListUpdated.empty() )
        {
                text.append("\nUpdated Files:\n");
                for(std::vector<SSFileInfo>::const_iterator i = summary.FileListUpdated.begin(); i != summary.FileListUpdated.end(); ++i) 
                {
                    text.append((boost::format ("\t%1%:  %2%\n") % (*i).second % (*i).first).str());
                }
        }

        m_TextBufferDetails->set_text(text);
        update_filestats();
        m_VboxInner->set_sensitive(true);
    }
    
    void
    MLibManager::scan_start ()
    {
        m_VboxInner->set_sensitive(false);
    }

    void
    MLibManager::scan_run( gint64 n, bool deep )
    {
        m_Statusbar->pop();
        m_Statusbar->push((
            boost::format(_("Scanning: %1% %2%"))
            % n
            % (deep ? _("Files") : _("Folders"))
        ).str());
    }

    void
    MLibManager::hide ()
    {
        Gtk::Window::get_position( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-mlib-x")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-mlib-y")));
        Gtk::Window::get_size( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-mlib-w")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-mlib-h")));

        Gtk::Widget::hide();
    }

    void
    MLibManager::present ()
    {
        resize(
           mcs->key_get<int>("mpx","window-mlib-w"),
           mcs->key_get<int>("mpx","window-mlib-h")
        );

        move(
            mcs->key_get<int>("mpx","window-mlib-x"),
            mcs->key_get<int>("mpx","window-mlib-y")
        );

        Gtk::Window::show ();
        Gtk::Window::raise ();
    }

    bool
    MLibManager::is_present()
    {
        return is_visible();
    }

    void
    MLibManager::push_message(std::string const& message)
    {
        m_Statusbar->pop();
        m_Statusbar->push(message);

        while (gtk_events_pending())
            gtk_main_iteration();
    }

    void
    MLibManager::rescan_all_volumes(bool deep)
    {
        Gtk::TreeModel::Children children = m_VolumesView->get_model()->children();

        if( children.empty() ) 
            return;
    
        m_VboxInner->set_sensitive(false);

#ifdef HAVE_HAL
        if( mcs->key_get<bool>("library","use-hal"))
        {
              for(Gtk::TreeModel::iterator iter = children.begin(); iter != children.end(); ++iter)
              {
                    Hal::RefPtr<Hal::Volume> Vol = (*iter)[VolumeColumns.Volume];

                    std::string VolumeUDI     = Vol->get_udi();
                    std::string DeviceUDI     = Vol->get_storage_device_udi();
                    std::string MountPoint    = Vol->get_mount_point();

                    reread_paths:

                    SQL::RowV v;
                    services->get<Library>("mpx-service-library")->getSQL(
                            v,
                            (boost::format ("SELECT DISTINCT insert_path FROM track WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s'")
                             % DeviceUDI
                             % VolumeUDI
                             ).str()
                    );

                    StrSetT ManagedPaths; 
                    for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                    {
                        ManagedPaths.insert(build_filename(MountPoint, boost::get<std::string>((*i)["insert_path"])));
                    }

                    if(!ManagedPaths.empty())
                    {
                        StrV v;
                        for(StrSetT::const_iterator i = ManagedPaths.begin(); i != ManagedPaths.end(); ++i)
                        {
                            PathTestResult r = path_test(*i);
                            switch( r )
                            {
                                case IS_PRESENT:
                                case IGNORED:
                                    v.push_back(filename_to_uri(*i));
                                    break;

                                case RELOCATED:
                                case DELETED:
                                    goto reread_paths;
                            }
                        }

                        services->get<Library>("mpx-service-library")->initScan(v, deep);                  
                    }
              }
        }
        else
#endif
        {
            services->get<Library>("mpx-service-library")->initScanAll();                  
        }
    }

    void
    MLibManager::on_mlib_remove_dupes ()
    {
        SQL::RowV rows;
        services->get<Library>("mpx-service-library")->getSQL(
            rows,
            "SELECT id FROM (track NATURAL JOIN (SELECT artist, album, title, track FROM track GROUP BY artist, album, title, track HAVING count(title) > 1)) EXCEPT SELECT id FROM track GROUP BY artist, album, title, track HAVING count(title) > 1" 
        );

        if( rows.empty() )
        {
                MessageDialog dialog (
                      *this
                    , _("No duplicates have been found. Click OK to close this dialog.") 
                    , false
                    , MESSAGE_INFO
                    , BUTTONS_OK
                    , true
                );

                dialog.set_title(_("AudioSource: Remove Duplicates"));
                dialog.run();
                return;
        }

        MessageDialog dialog (
              *this
            , (boost::format(_("Please confirm the deletion of <b>%lld</b> tracks from the Library <b>and the filesystem</b>. <i>This can not be undone!</i>\n\nClick OK to proceed.")) % gint64(rows.size())).str()
            , true
            , MESSAGE_WARNING
            , BUTTONS_OK_CANCEL
            , true
        );

        dialog.set_title(_("AudioSource: Remove Duplicates"));

        int response = dialog.run();
        dialog.hide();
    
        if( response == GTK_RESPONSE_OK )
        {
            services->get<Library>("mpx-service-library")->removeDupes();
        }
    }

#ifdef HAVE_HAL
    void
    MLibManager::on_volume_removed (const HAL::Volume& volume)
    {
    } 

    void
    MLibManager::on_volume_added (const HAL::Volume& volume)
    {
        static boost::format volume_label_f1 ("%1%: %2%");

        try{
            Hal::RefPtr<Hal::Context> Ctx = services->get<HAL>("mpx-service-hal")->get_context();
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
    MLibManager::clear_volumes ()
    {
        VolumeStore->clear();
        FSTreeStore->clear();

        m_ManagedPaths  = StrSetT();
        m_VolumeUDI     = std::string(); 
        m_DeviceUDI     = std::string(); 
        m_MountPoint    = std::string();
    }

    void
    MLibManager::populate_volumes ()
    {
        static boost::format volume_label_f1 ("%1%: %2%");
        
        FSTreeStore->clear();
        VolumeStore->clear();

        m_ManagedPaths  = StrSetT();
        m_VolumeUDI     = std::string(); 
        m_DeviceUDI     = std::string(); 
        m_MountPoint    = std::string();

        try { 
                Hal::RefPtr<Hal::Context> Ctx = services->get<HAL>("mpx-service-hal")->get_context();
                HAL::VolumesHALCC_v volumes;
                services->get<HAL>("mpx-service-hal")->volumes(volumes);

                for(HAL::VolumesHALCC_v::const_iterator i = volumes.begin(); i != volumes.end(); ++i)
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
    MLibManager::fstree_sort(
          const TreeIter & iter_a
        , const TreeIter & iter_b
    )
    {
        Glib::ustring a ((*iter_a)[FSTreeColumns.SegName]);
        Glib::ustring b ((*iter_b)[FSTreeColumns.SegName]);
        return a.compare(b);
    }

    PathTestResult
    MLibManager::path_test(
        const std::string& path
    )
    {
        if( Glib::file_test(path, Glib::FILE_TEST_EXISTS) )
        {
          return IS_PRESENT;
        }
        else
        {
                TaskDialog dialog(
                      this,
                      _("AudioSource: Music Path is missing"),
                      _("Music Path is missing"),
                      Gtk::MESSAGE_WARNING,
                      (boost::format (_("A path managed by AudioSource containing music can not be found.\nYour intervention is required.\n\nPath: %s")) % path).str()
                );

                dialog.add_button(
                      _("Relocate Path"),
                      _("The path has been moved somewhere else, and AudioSource needs to know the new location."),
                      Gtk::Stock::HARDDISK,
                      0
                );
              
                dialog.add_button(
                      _("Delete Path from Library"),
                      _("The path has been permanently deleted, and AudioSource should not manage it anymore."),
                      Gtk::Stock::DELETE,
                      1
                );

                dialog.add_button(
                      _("Check Next Time"),
                      _("The situation is different than described above, and deeper intervention is neccessary.\nAudioSource will check the path at the next rescan again."),
                      Gtk::Stock::OK,
                      2
                );

                // Show the relocate dialog and process the result

                rerun_taskdialog:

                int result = dialog.run();

                std::string volume_udi_source, device_udi_source;
                std::string volume_udi_target, device_udi_target;
                std::string vrp_source, vrp_target;
                HAL::Volume volume_source, volume_target;
                std::string uri;

                FileChooserDialog fcdialog (
                      _("AudioSource: Select relocated path target"),
                      FILE_CHOOSER_ACTION_SELECT_FOLDER
                );

                fcdialog.add_button(
                      Gtk::Stock::CANCEL,
                      Gtk::RESPONSE_CANCEL
                );

                fcdialog.add_button(
                      Gtk::Stock::OK,
                      Gtk::RESPONSE_OK
                );

                int response;

                switch(result)
                {
                  case 0:
                      response = fcdialog.run();
                      fcdialog.hide();

                      if( response == Gtk::RESPONSE_CANCEL ) 
                          goto rerun_taskdialog;

                      uri = fcdialog.get_current_folder_uri();

                      volume_source = services->get<HAL>("mpx-service-hal")->get_volume_for_uri(Glib::filename_to_uri(path));
                      volume_udi_source =
                          volume_source.volume_udi ;
                      device_udi_source =  
                          volume_source.device_udi ;
                      vrp_source = 
                          path.substr(volume_source.mount_point.length()) ;

                      volume_target = services->get<HAL>("mpx-service-hal")->get_volume_for_uri(uri);
                      volume_udi_target =
                          volume_target.volume_udi ;
                      device_udi_target =  
                          volume_target.device_udi ;
                      vrp_target = 
                          Glib::filename_from_uri(uri).substr(volume_target.mount_point.length()) ;

                      services->get<Library>("mpx-service-library")->execSQL(
                          (boost::format("UPDATE track SET hal_device_udi = '%s', hal_volume_udi = '%s', insert_path = '%s' "
                                         "WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s' AND insert_path = '%s'")
                              % device_udi_target
                              % volume_udi_target
                              % vrp_target
                              % device_udi_source
                              % volume_udi_source
                              % vrp_source
                      ).str());

                      on_volumes_changed ();

                      /*
                      services->get<Library>("mpx-service-library")->vacuumVolume(
                          device_udi_target,
                          volume_udi_target 
                      );*/

                      return RELOCATED;
                    break;

                  case 1:

                      volume_target = services->get<HAL>("mpx-service-hal")->get_volume_for_uri(Glib::filename_to_uri(path));

                      volume_udi_target =
                          volume_target.volume_udi ;

                      device_udi_target =  
                          volume_target.device_udi ;

                      vrp_target = 
                          path.substr(volume_target.mount_point.length()) ;

                      services->get<Library>("mpx-service-library")->execSQL(
                          (boost::format("DELETE FROM track WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s' AND insert_path LIKE '%s%%'")
                              % device_udi_target
                              % volume_udi_target
                              % vrp_target
                      ).str());

                      on_volumes_changed();

                      /*
                      services->get<Library>("mpx-service-library")->vacuumVolume(
                          device_udi_target,
                          volume_udi_target 
                      );*/

                      return DELETED;
                    break;

                  case 2:
                      return IGNORED;
                    break;
                }
        }
        return IGNORED;
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
                        if(!services->get<HAL>("mpx-service-hal")->path_is_mount_path(path))
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
                                if(!services->get<HAL>("mpx-service-hal")->path_is_mount_path(path))
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
        TreePath path (TreePath::size_type(1), TreePath::value_type(0));
        m_FSTree->scroll_to_row(path, 0.0);
    }

    bool
    MLibManager::has_active_parent(Gtk::TreeIter & iter)
    {
        TreePath path = FSTreeStore->get_path(iter);

        while(path.up())
        {
            iter = FSTreeStore->get_iter(path);
            std::string FullPath = std::string((*iter)[FSTreeColumns.FullPath])+"/";
            if(m_ManagedPaths.count(FullPath))
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
    MLibManager::on_volumes_changed ()
    {
        m_ManagedPaths  = StrSetT();
        m_VolumeUDI     = std::string(); 
        m_DeviceUDI     = std::string(); 
        m_MountPoint    = std::string();

        bool has_selection = m_VolumesView->get_selection()->count_selected_rows();

        if( has_selection )
        {
            TreeIter iter = m_VolumesView->get_selection()->get_selected();

            Hal::RefPtr<Hal::Volume> Vol = (*iter)[VolumeColumns.Volume];

            m_VolumeUDI     = Vol->get_udi();
            m_DeviceUDI     = Vol->get_storage_device_udi();
            m_MountPoint    = Vol->get_mount_point();

            SQL::RowV v;
            services->get<Library>("mpx-service-library")->getSQL(
                    v,
                    (boost::format ("SELECT DISTINCT insert_path FROM track WHERE hal_device_udi = '%s' AND hal_volume_udi = '%s'")
                        % m_DeviceUDI
                        % m_VolumeUDI
                    ).str()
            );

            for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
            {
                m_ManagedPaths.insert(build_filename(m_MountPoint, boost::get<std::string>((*i)["insert_path"]))+"/");
            }

            m_Actions->get_action("action-volume-rescan")->set_sensitive( 
                has_selection && !m_ManagedPaths.empty()
            );

            m_Actions->get_action("action-volume-rescan-deep")->set_sensitive( 
                has_selection && !m_ManagedPaths.empty()
            );

            m_Actions->get_action("action-volume-vacuum")->set_sensitive( 
                has_selection && !m_ManagedPaths.empty()
            );

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

        std::string FullPath = std::string((*iter)[FSTreeColumns.FullPath]);
        TreeIter iter_copy = iter;

        append_path(FullPath, iter_copy);

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
        if( m_InnerdialogMainloop )
            m_InnerdialogMainloop->quit();

        TreeIter iter = FSTreeStore->get_iter(path_str);
        TreeIter iter_copy = iter;

        std::string FullPath = std::string((*iter)[FSTreeColumns.FullPath])+"/";

        if(has_active_parent(iter))
            return;
        
        if(m_ManagedPaths.count(FullPath))
        {
            dynamic_cast<Gtk::Label*>(m_Xml->get_widget("innerdialog-label-yes"))->set_text_with_mnemonic("_Remove");
            m_InnerdialogLabel->set_markup((
                boost::format(
                    "<b>Remove</b> %s <b>?</b>")
                    % Markup::escape_text(filename_to_utf8(FullPath)).c_str()
                ).str()
            );

            m_InnerdialogHBox->show();
            m_InnerdialogMainloop = Glib::MainLoop::create();

            GDK_THREADS_LEAVE();
            m_InnerdialogMainloop->run();
            GDK_THREADS_ENTER();
            
            m_InnerdialogHBox->hide();
            m_InnerdialogLabel->set_text("");

            if( m_InnerdialogResponse == GTK_RESPONSE_YES )
            {
                m_VboxInner->set_sensitive( false );

                std::string FullPath_Sub = FullPath.substr(m_MountPoint.length()) ; 
                services->get<Library>("mpx-service-library")->deletePath( m_DeviceUDI, m_VolumeUDI, FullPath_Sub.substr( 0, FullPath_Sub.size() - 1 ) );

                m_ManagedPaths.erase( FullPath );
                recreate_path_frags();

                TreePath path = FSTreeStore->get_path(iter_copy); 
                FSTreeStore->row_changed(path, iter_copy);

                m_Actions->get_action("action-volume-rescan")->set_sensitive( 
                    !m_ManagedPaths.empty()
                );

                m_Actions->get_action("action-volume-rescan-deep")->set_sensitive( 
                    !m_ManagedPaths.empty()
                );

                m_Actions->get_action("action-volume-vacuum")->set_sensitive( 
                    !m_ManagedPaths.empty()
                );

                m_VboxInner->set_sensitive( true );
            }
        }
        else
        {
            dynamic_cast<Gtk::Label*>(m_Xml->get_widget("innerdialog-label-yes"))->set_text_with_mnemonic("_Add");
            m_InnerdialogLabel->set_markup((
                boost::format(
                    "<b>Add</b> %s <b>?</b>")
                    % Markup::escape_text(filename_to_utf8(FullPath)).c_str()
                ).str()
            );

            m_InnerdialogHBox->show();
            m_InnerdialogMainloop = Glib::MainLoop::create();

            GDK_THREADS_LEAVE();
            m_InnerdialogMainloop->run();
            GDK_THREADS_ENTER();

            m_InnerdialogMainloop.reset(); 
            
            m_InnerdialogHBox->hide();
            m_InnerdialogLabel->set_text("");

            if( m_InnerdialogResponse == GTK_RESPONSE_YES )
            {
                m_ManagedPaths.insert(FullPath);

                recreate_path_frags ();
                TreePath path = FSTreeStore->get_path(iter_copy); 
                FSTreeStore->row_changed(path, iter_copy);

                m_Actions->get_action("action-volume-rescan")->set_sensitive( 
                    !m_ManagedPaths.empty()
                );

                m_Actions->get_action("action-volume-rescan-deep")->set_sensitive( 
                    !m_ManagedPaths.empty()
                );

                m_Actions->get_action("action-volume-vacuum")->set_sensitive( 
                    !m_ManagedPaths.empty()
                );

                StrV v;
                v.push_back(filename_to_uri(FullPath));
                services->get<Library>("mpx-service-library")->initAdd(v);
            }
        }
    }

    void
    MLibManager::on_rescan_volume(bool deep)
    {
        restart_rescan_volume:
    
        StrV v;
        for(StrSetT::const_iterator i = m_ManagedPaths.begin(); i != m_ManagedPaths.end(); ++i)
        {
            PathTestResult r = path_test(*i);
            switch( r )
            {
                case IS_PRESENT:
                case IGNORED:
                    v.push_back(filename_to_uri(*i));
                    break;

                case RELOCATED:
                case DELETED:
                    goto restart_rescan_volume;
            }
        }
        services->get<Library>("mpx-service-library")->initScan(v, deep);
    }

    void
    MLibManager::on_vacuum_all ()
    {
        Gtk::TreeModel::Children children = m_VolumesView->get_model()->children();

        if( children.empty() )
            return;

        m_VboxInner->set_sensitive(false);

#ifdef HAVE_HAL
        if( mcs->key_get<bool>("library", "use-hal"))
        {
              HAL::VolumeKey_v v ;

              for(Gtk::TreeModel::iterator iter = children.begin(); iter != children.end(); ++iter)
              {
                    Hal::RefPtr<Hal::Volume> Vol = (*iter)[VolumeColumns.Volume];

                    std::string VolumeUDI     = Vol->get_udi();
                    std::string DeviceUDI     = Vol->get_storage_device_udi();
                    std::string MountPoint    = Vol->get_mount_point();

                    v.push_back( HAL::VolumeKey( DeviceUDI, VolumeUDI ));
              }

              services->get<Library>("mpx-service-library")->vacuumVolumeList( v ) ;                  
        }
        else
#endif
        {
            services->get<Library>("mpx-service-library")->vacuum();                  
        }
    }

    void
    MLibManager::on_vacuum_volume ()
    {
        HAL::VolumeKey_v v (1);
        v[0] = HAL::VolumeKey( m_DeviceUDI, m_VolumeUDI );
        services->get<Library>("mpx-service-library")->vacuumVolumeList( v );
    }

    void
    MLibManager::on_update_statistics()
    {
        services->get<Library>("mpx-service-library")->scanner()->update_statistics();
    }

    void
    MLibManager::on_recache_covers()
    {
        TimedConfirmation dialog (_("Please confirm Cover Refresh"), 10);
        int response = dialog.run(_("Are you sure you want to Refresh <b>all</b> covers at this time? (previous covers will be irrevocably lost)"));
        if( response == Gtk::RESPONSE_OK )
        {
            services->get<Library>("mpx-service-library")->recacheCovers();
        }
    }

    void
    MLibManager::cell_data_func_active (CellRenderer * basecell, TreeIter const& iter)
    {
        CellRendererToggle & cell = *(dynamic_cast<CellRendererToggle*>(basecell));

        std::string FullPath = std::string((*iter)[FSTreeColumns.FullPath])+"/";
        cell.property_active() = m_ManagedPaths.count(FullPath);
    }

    void
    MLibManager::cell_data_func_text (CellRenderer * basecell, TreeIter const& iter)
    {
        TreePath path = FSTreeStore->get_path(iter); 

        if( path.size() < 1 )
        {
            return;
        }

        std::string FullPath = std::string((*iter)[FSTreeColumns.FullPath])+"/";
        std::string SegName  = (*iter)[FSTreeColumns.SegName];

        CellRendererText & cell = *(dynamic_cast<CellRendererText*>(basecell));

        for(StrSetT::const_iterator i = m_ManagedPaths.begin(); i != m_ManagedPaths.end(); ++i)
        { 
            if( ((*i).substr( 0, FullPath.size() ) == FullPath) ) 
            {
                cell.property_markup() = (boost::format("<span foreground='#0000ff'><b>%s</b></span>") % Markup::escape_text(SegName)).str();
                return;
            }
        }

        TreeIter iter_copy = iter;
        if(has_active_parent(iter_copy))
            cell.property_markup() = (boost::format("<span foreground='#a0a0a0'>%s</span>") % Markup::escape_text(SegName)).str();
        else
            cell.property_text() = SegName; 
    }

    bool
    MLibManager::slot_select (const Glib::RefPtr<Gtk::TreeModel>&model, const Gtk::TreePath &path, bool was_selected)
    {
        TreeIter iter = FSTreeStore->get_iter(path);
        return !(has_active_parent(iter));
    }

    bool
    MLibManager::on_rescan_timeout()
    {
        if(!is_present() && mcs->key_get<bool>("library","rescan-in-intervals") && m_RescanTimer.elapsed() >= mcs->key_get<int>("library","rescan-interval") * 60)
        {
          rescan_all_volumes();
          m_RescanTimer.reset();
        }
        return true;
    }
#endif

    void
            MLibManager::on_import_folder()
            {
                    boost::shared_ptr<DialogImportFolder> d = boost::shared_ptr<DialogImportFolder>(DialogImportFolder::create());

                    if(d->run() == 0) // Import
                    {
                            Glib::ustring uri; 
                            d->get_folder_infos(uri);
                            d->hide();
                            StrV v;
                            v.push_back(uri);
                            services->get<Library>("mpx-service-library")->initAdd(v);
                    }
            }

    void
            MLibManager::on_import_share()
            {
                    boost::shared_ptr<DialogImportShare> d = boost::shared_ptr<DialogImportShare>(DialogImportShare::create());

rerun_import_share_dialog:

                    if(d->run() == 0) // Import
                    {
                            Glib::ustring login, password;
                            d->get_share_infos(m_Share, m_ShareName, login, password);
                            d->hide ();

                            if(m_ShareName.empty())
                            {
                                    MessageDialog dialog (*this, (_("The Share's name can not be empty")));
                                    dialog.run();
                                    goto rerun_import_share_dialog;
                            }

                            m_MountFile = Glib::wrap (g_vfs_get_file_for_uri (g_vfs_get_default(), m_Share.c_str()));
                            if(!m_MountFile)
                            {
                                    MessageDialog dialog (*this, (boost::format (_("An Error occcured getting a handle for the share '%s'\n"
                                                                            "Please veryify the share URI and credentials")) % m_Share.c_str()).str());
                                    dialog.run();
                                    goto rerun_import_share_dialog;
                            }

                            m_MountOperation = Gio::MountOperation::create();
                            if(!m_MountOperation)
                            {
                                    MessageDialog dialog (*this, (boost::format (_("An Error occcured trying to mount the share '%s'")) % m_Share.c_str()).str());
                                    dialog.run();
                                    return; 
                            }

                            m_MountOperation->set_username(login);
                            m_MountOperation->set_password(password);
                            m_MountOperation->signal_ask_password().connect(sigc::mem_fun(*this, &MLibManager::ask_password_cb));
                            m_MountFile->mount_mountable(m_MountOperation, sigc::mem_fun(*this, &MLibManager::mount_ready_callback));
                    }
            }

    void
            MLibManager::ask_password_cb(
                const Glib::ustring& message,
                const Glib::ustring& default_user,
                const Glib::ustring& default_domain,
                Gio::AskPasswordFlags flags
            )
            {
                    Glib::ustring value;
                    if (flags & Gio::ASK_PASSWORD_NEED_USERNAME)
                    {
                            RequestValue * p = RequestValue::create();
                            p->set_question(_("Please Enter the Username:"));
                            int reply = p->run();
                            if(reply == GTK_RESPONSE_CANCEL)
                            {
                                    m_MountOperation->reply (Gio::MOUNT_OPERATION_ABORTED /*abort*/);
                                    return;
                            }
                            p->get_request_infos(value);
                            m_MountOperation->set_username (value);
                            delete p;
                            value.clear();
                    }

                    if (flags & Gio::ASK_PASSWORD_NEED_DOMAIN)
                    {
                            RequestValue * p = RequestValue::create();
                            p->set_question(_("Please Enter the Domain:"));
                            int reply = p->run();
                            if(reply == GTK_RESPONSE_CANCEL)
                            {
                                    m_MountOperation->reply (Gio::MOUNT_OPERATION_ABORTED /*abort*/);
                                    return;
                            }
                            p->get_request_infos(value);
                            m_MountOperation->set_domain (value);
                            delete p;
                            value.clear();
                    }

                    if (flags & Gio::ASK_PASSWORD_NEED_PASSWORD)
                    {
                            RequestValue * p = RequestValue::create();
                            p->set_question(_("Please Enter the Password:"));
                            int reply = p->run();
                            if(reply == GTK_RESPONSE_CANCEL)
                            {
                                    m_MountOperation->reply (Gio::MOUNT_OPERATION_ABORTED /*abort*/);
                                    return;
                            }
                            p->get_request_infos(value);
                            m_MountOperation->set_password (value);
                            delete p;
                            value.clear();
                    }

                    m_MountOperation->reply (Gio::MOUNT_OPERATION_HANDLED);
            }

    void
            MLibManager::mount_ready_callback (Glib::RefPtr<Gio::AsyncResult>& res)
            {
                    try
                    {
                            m_MountFile->mount_mountable_finish(res);
                    }
                    catch(const Glib::Error& error)
                    {
                            if(error.code() != G_IO_ERROR_ALREADY_MOUNTED)
                            {
                                    MessageDialog dialog (*this, (boost::format ("An Error occcured while mounting the share: %s") % error.what()).str());
                                    dialog.run();
                                    return;
                            }
                            else
                                    g_warning("%s: Location '%s' is already mounted", G_STRLOC, m_Share.c_str());

                    }

                    Util::FileList v (1, m_Share);
                    services->get<Library>("mpx-service-library")->initAdd(v);
            }

    void
            MLibManager::unmount_ready_callback( Glib::RefPtr<Gio::AsyncResult>& res )
            {
            }
}
