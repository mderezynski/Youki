//  MPX
//  Copyright (C) 2003-2007 MPX Development
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
//  The MPX project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif 

#include <utility>
#include <iostream>

#include <glibmm.h>
#include <glib/gi18n.h>
#include <gtk/gtkstock.h>
#include <gtkmm.h>
#include <libglademm.h>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#ifdef HAVE_ALSA
#  define ALSA_PCM_NEW_HW_PARAMS_API
#  define ALSA_PCM_NEW_SW_PARAMS_API

#  include <alsa/global.h>
#  include <alsa/asoundlib.h>
#  include <alsa/pcm_plugin.h>
#  include <alsa/control.h>
#endif

#include <mcs/mcs.h>

#include "mpx/mpx-preferences.hh"

#include "mpx/mpx-audio.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/util-string.hh"
#include "mpx/widgets/widgetloader.hh"

using namespace Glib;
using namespace Gtk;
using namespace MPX::Audio;

namespace MPX
{
    namespace
    {
        class SignalBlocker
        {
            public:

                SignalBlocker (sigc::connection & connection)
                    : m_conn(connection)
                {
                    m_conn.block ();
                }

                ~SignalBlocker ()
                {
                    m_conn.unblock();
                }

            private:
                sigc::connection & m_conn;
        };

        char const* sources[] =
        {
            N_("Local files (folder.jpg, cover.jpg, etc.)"),
            N_("MusicBrainz Advanced Relations"),
            N_("Amazon ASIN"),
            N_("Amazon Search API"),
            N_("Inline Covers (File Metadata)")
        };

        struct FileFormatInfo
        {
            std::string     MIME;
            std::string     Name;
            std::string     Description;
        };

        FileFormatInfo formats[] =
        {
            {
                "audio/x-flac",
                "FLAC",
                "FLAC (Free Lossless Audio Codec)"
            },

            {
                "audio/x-ape",
                "APE",
                "Monkey's Audio"
            },

            {
                "audio/x-vorbis+ogg",
                "OGG",
                "Ogg Vorbis"
            },

            {
                "audio/x-musepack",
                "MPC",
                "Musepack"
            },

            {
                "audio/mp4",
                "MP4",
                "AAC/MP4 (Advanced Audio Codec/MPEG 4 Audio)"
            },

            {
                "audio/mpeg",
                "MP3",
                "MPEG 1 Layer III Audio"
            },

            {
                "audio/x-ms-wma",
                "WMA",
                "Microsoft Windows Media Audio"
            }
        };

    }                            // namespace

    using namespace Gnome::Glade;

    typedef sigc::signal<void, int, bool> SignalColumnState;

    class CoverArtSourceView : public WidgetLoader<Gtk::TreeView>
    {
        public:

            struct Signals_t
            {
                SignalColumnState ColumnState;
            };

            Signals_t Signals;

            class Columns_t : public Gtk::TreeModelColumnRecord
            {
                public:

                    Gtk::TreeModelColumn<Glib::ustring> Name;
                    Gtk::TreeModelColumn<int>           ID;
                    Gtk::TreeModelColumn<bool>          Active;

                    Columns_t ()
                    {
                        add(Name);
                        add(ID);
                        add(Active);
                    };
            };

            Columns_t                       Columns;
            Glib::RefPtr<Gtk::ListStore>    Store;

            CoverArtSourceView (const Glib::RefPtr<Gnome::Glade::Xml>& xml)
                : WidgetLoader<Gtk::TreeView>(xml, "preferences-treeview-coverartsources")
            {
                Store = Gtk::ListStore::create(Columns);
                set_model(Store);

                TreeViewColumn *col = manage( new TreeViewColumn(_("Active")));
                CellRendererToggle *cell1 = manage( new CellRendererToggle );
                cell1->property_xalign() = 0.5;
                cell1->signal_toggled().connect(
                    sigc::mem_fun(
                    *this,
                    &CoverArtSourceView::on_cell_toggled
                    ));
                col->pack_start(*cell1, true);
                col->add_attribute(*cell1, "active", Columns.Active);
                append_column(*col);

                append_column(_("Column"), Columns.Name);

                for( unsigned i = 0; i < G_N_ELEMENTS(sources); ++i )
                {
                    TreeIter iter = Store->append();

                    int source = mcs->key_get<int>("Preferences-CoverArtSources", (boost::format ("Source%d") % i).str());

                    (*iter)[Columns.Name]   = _(sources[source]);
                    (*iter)[Columns.ID]     = source;
                    (*iter)[Columns.Active] = mcs->key_get<bool>("Preferences-CoverArtSources", (boost::format ("SourceActive%d") % i).str());
                }

                Store->signal_row_deleted().connect(
                    sigc::mem_fun(
                        *this,
                        &CoverArtSourceView::on_row_deleted
                ));
            };

            void
                update_configuration ()
            {
                using namespace Gtk;

                TreeNodeChildren const& children = Store->children();

                for(TreeNodeChildren::const_iterator i = children.begin(); i != children.end(); ++i)
                {
                    int n = std::distance(children.begin(), i);

                    mcs->key_set<int>("Preferences-CoverArtSources", (boost::format ("Source%d") % n).str(), (*i)[Columns.ID]);
                    mcs->key_set<bool>("Preferences-CoverArtSources", (boost::format ("SourceActive%d") % n).str(), (*i)[Columns.Active]);
                }
            }

            void
                on_cell_toggled(Glib::ustring const& path)
            {
                TreeIter iter = Store->get_iter(path);
                (*iter)[Columns.Active] = !bool((*iter)[Columns.Active]);
                Signals.ColumnState.emit((*iter)[Columns.ID], (*iter)[Columns.Active]); 

                update_configuration ();
            }

            virtual void
                on_row_deleted(
                      const TreeModel::Path&        G_GNUC_UNUSED
            )
            {
                update_configuration ();
            }
    };

    class FileFormatPrioritiesView : public WidgetLoader<Gtk::TreeView>
    {
        public:

            struct Signals_t
            {
                SignalColumnState ColumnState;
            };

            Signals_t Signals;

            class Columns_t : public Gtk::TreeModelColumnRecord
            {
                public:

                    Gtk::TreeModelColumn<std::string>   MIME;
                    Gtk::TreeModelColumn<std::string>   Name;
                    Gtk::TreeModelColumn<std::string>   Description;

                    Columns_t ()
                    {
                        add(MIME);
                        add(Name);
                        add(Description);
                    };
            };

            Columns_t                       Columns;
            Glib::RefPtr<Gtk::ListStore>    Store;

            FileFormatPrioritiesView (const Glib::RefPtr<Gnome::Glade::Xml>& xml)
                : WidgetLoader<Gtk::TreeView>(xml, "preferences-treeview-fileformatprefs")
            {
                Store = Gtk::ListStore::create(Columns);
                set_model(Store);

                append_column(_("Name"), Columns.Name);
                append_column(_("Description"), Columns.Description);
                append_column(_("MIME Type"), Columns.MIME);

                typedef std::map<std::string, FileFormatInfo> map_t;
                map_t m;

                for( unsigned n = 0; n < G_N_ELEMENTS(formats); ++n )
                {
                    m.insert( std::make_pair( formats[n].MIME, formats[n] ));
                }

                for( unsigned n = 0; n < G_N_ELEMENTS(formats); ++n )
                {
                    TreeIter iter = Store->append();

                    std::string mime = mcs->key_get<std::string>("Preferences-FileFormatPriorities", (boost::format ("Format%d") % n).str());

                    const FileFormatInfo& info = m.find( mime )->second;

                    (*iter)[Columns.MIME]           = info.MIME ;
                    (*iter)[Columns.Name]           = info.Name ;
                    (*iter)[Columns.Description]    = info.Description ;
                }

                Store->signal_row_deleted().connect(
                    sigc::mem_fun(
                        *this,
                        &FileFormatPrioritiesView::on_row_deleted
                ));
            };

            void
                update_configuration ()
            {
                using namespace Gtk;

                TreeNodeChildren const& children = Store->children();
                for( TreeNodeChildren::const_iterator i = children.begin(); i != children.end(); ++i )
                {
                    int n = std::distance( children.begin(), i );
                    mcs->key_set<std::string>("Preferences-FileFormatPriorities", (boost::format ("Format%d") % n).str(), (*i)[Columns.MIME]);
                }
            }

            virtual void
                on_row_deleted(
                  const TreeModel::Path&        G_GNUC_UNUSED
            )
            {
                update_configuration ();
            }
    };

    //// Preferences
    Preferences*
        Preferences::create ()
    {
        return new Preferences (Gnome::Glade::Xml::create (build_filename(DATA_DIR, "glade" G_DIR_SEPARATOR_S "preferences.glade")));
    }

    Preferences::~Preferences ()
    {
        hide();
        mcs->key_set<int>("mpx","preferences-notebook-page", m_notebook_preferences->get_current_page());
    }

    bool
    Preferences::on_delete_event(GdkEventAny* G_GNUC_UNUSED)
    {
        hide();
        return true;
    }

    void
    Preferences::present ()
    {
        resize(
            mcs->key_get<int>("mpx","window-prefs-w"),
            mcs->key_get<int>("mpx","window-prefs-h")
            );

        move(
            mcs->key_get<int>("mpx","window-prefs-x"),
            mcs->key_get<int>("mpx","window-prefs-y")
            );

        Gtk::Window::show ();
        Gtk::Window::raise ();
    }

    void
    Preferences::hide ()
    {
        Gtk::Window::get_position( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-prefs-x")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-prefs-y")));
        Gtk::Window::get_size( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-prefs-w")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-prefs-h")));
        Gtk::Widget::hide();
    }

    void
    Preferences::add_page(
          Gtk::Widget*          widget
        , const std::string&    name
    )
    {
        g_object_ref(G_OBJECT(widget->gobj())) ;
        widget->unparent() ;
        m_notebook_preferences->append_page(
              *widget
            , name
        ) ;
        g_object_unref(G_OBJECT(widget->gobj())) ;
    }

    Preferences::Preferences(
        const Glib::RefPtr<Gnome::Glade::Xml>&  xml
    )
        : Gnome::Glade::WidgetLoader<Gtk::Window>(xml, "preferences")
        , MPX::Service::Base("mpx-service-preferences")
    {
        // Preferences
        dynamic_cast<Button*>(m_Xml->get_widget ("close"))->signal_clicked().connect(
            sigc::mem_fun(
                *this
              , &Preferences::hide
        ));

        m_Xml->get_widget ("notebook", m_notebook_preferences);

        // Coverart Sources
        m_Covers_CoverArtSources = new CoverArtSourceView(m_Xml);

        // File Format Priorities
        m_Fmts_PrioritiesView = new FileFormatPrioritiesView(m_Xml);
        m_Xml->get_widget("cb-prioritize-by-filetype", m_Fmts_PrioritizeByFileType);
        m_Xml->get_widget("cb-prioritize-by-bitrate", m_Fmts_PrioritizeByBitrate);

        m_Fmts_PrioritizeByFileType->signal_toggled().connect(
            sigc::compose(
            sigc::mem_fun(*m_Fmts_PrioritiesView, &Gtk::Widget::set_sensitive),
            sigc::mem_fun(*m_Fmts_PrioritizeByFileType, &Gtk::ToggleButton::get_active)
            ));

        mcs_bind->bind_toggle_button(
            *m_Fmts_PrioritizeByFileType
            , "Preferences-FileFormatPriorities"
            , "prioritize-by-filetype"
            );

        mcs_bind->bind_toggle_button(
            *m_Fmts_PrioritizeByBitrate
            , "Preferences-FileFormatPriorities"
            , "prioritize-by-bitrate"
            );

        m_Fmts_PrioritiesView->set_sensitive( mcs->key_get<bool>("Preferences-FileFormatPriorities", "prioritize-by-filetype" ));

        // Library Stuff

        mcs_bind->bind_filechooser(
            *dynamic_cast<Gtk::FileChooser*>( m_Xml->get_widget( "preferences-fc-music-import-path" ))
            , "mpx"
            , "music-import-path"
            );

        m_Xml->get_widget("rescan-at-startup", m_Library_RescanAtStartup);
        m_Xml->get_widget("rescan-in-intervals", m_Library_RescanInIntervals);
        m_Xml->get_widget("rescan-interval", m_Library_RescanInterval);
        m_Xml->get_widget("hbox-rescan-interval", m_Library_RescanIntervalBox);

        mcs_bind->bind_spin_button(
            *m_Library_RescanInterval
            , "library"
            , "rescan-interval"
            );

        mcs_bind->bind_toggle_button(
            *m_Library_RescanAtStartup
            , "library"
            , "rescan-at-startup"
            );

        mcs_bind->bind_toggle_button(
            *m_Library_RescanInIntervals
            , "library"
            , "rescan-in-intervals"
            );

        m_Library_RescanInIntervals->signal_toggled().connect(
            sigc::compose(
            sigc::mem_fun(*m_Library_RescanIntervalBox, &Gtk::Widget::set_sensitive),
            sigc::mem_fun(*m_Library_RescanInIntervals, &Gtk::ToggleButton::get_active)
            ));

        #ifdef HAVE_HAL
        m_Xml->get_widget("lib-use-hal-rb1", m_Library_UseHAL_Yes);
        m_Xml->get_widget("lib-use-hal-rb2", m_Library_UseHAL_No);

        if( mcs->key_get<bool>("library","use-hal") )
            m_Library_UseHAL_Yes->set_active();
        else
            m_Library_UseHAL_No->set_active();

        m_Library_UseHAL_Yes->signal_toggled().connect(
            sigc::mem_fun(
            *this,
            &Preferences::on_library_use_hal_toggled
            ));

#else
        m_Xml->get_widget("vbox135")->hide();
#endif // HAVE_HAL

        // Radio

        m_Xml->get_widget("radio-minimal-bitrate", m_Radio_MinimalBitrate);
        mcs_bind->bind_spin_button(*m_Radio_MinimalBitrate, "radio", "minimal-bitrate");

        /*- Setup Window Geometry -----------------------------------------*/

        gtk_widget_realize(GTK_WIDGET(gobj()));

        resize(
            mcs->key_get<int>("mpx","window-prefs-w"),
            mcs->key_get<int>("mpx","window-prefs-h")
            );

        move(
            mcs->key_get<int>("mpx","window-prefs-x"),
            mcs->key_get<int>("mpx","window-prefs-y")
            );

        mcs->key_register("mpx","preferences-notebook-page", 0);
        m_notebook_preferences->set_current_page( mcs->key_get<int>("mpx","preferences-notebook-page") );
    }

    #ifdef HAVE_HAL
    void
        Preferences::on_library_use_hal_toggled()
    {
        boost::shared_ptr<Library> l = services->get<Library>("mpx-service-library");

        if( m_Library_UseHAL_Yes->get_active() )
        {
            m_Xml->get_widget("vbox135")->set_sensitive( false );
            g_message("%s: Switching to HAL mode", G_STRLOC);
            //l->switch_mode( true );
            mcs->key_set("library","use-hal", true);
            m_Xml->get_widget("vbox135")->set_sensitive( true );
        }
        else
        if( m_Library_UseHAL_No->get_active() )
        {
            m_Xml->get_widget("vbox135")->set_sensitive( false );
            g_message("%s: Switching to NO HAL mode", G_STRLOC);
            //l->switch_mode( false );
            mcs->key_set("library","use-hal", false);
            m_Xml->get_widget("vbox135")->set_sensitive( true );
        }
    }
    #endif
}  // namespace MPX
