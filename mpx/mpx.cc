//
//  mpx.cc
//
//  Authors:
//      Milosz Derezynski <milosz@backtrace.info>
//
//  (C) 2008 MPX Project
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

// PyGTK NO_IMPORT
#define NO_IMPORT

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mpx.hh"
#include "mpx/error.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/mpx-python.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-string.hh"
#include "mpx/util-ui.hh"
#include "mpx/xml/xspf.hh"
#include "mpx/com/mb-import-album.hh"
 
#include "dialog-about.hh"
#include "dialog-filebrowser.hh"
#include "import-share.hh"
#include "import-folder.hh"
#include "infoarea.hh"
#include "mlibmanager.hh"
#include "play.hh"
#include "plugin.hh"
#include "plugin-manager-gui.hh"
#include "preferences.hh"
#include "request-value.hh"
#include "sidebar.hh"
#include "splash-screen.hh"

#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/python.hpp>
#include <cairomm/cairomm.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <giomm.h>
#include <glib/gstdio.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <gdk/gdkx.h>
#include <gdl/gdl.h>

using namespace Glib;
using namespace Gtk;
using namespace std;
using namespace Gnome::Glade;
using namespace MPX::Util;
using boost::get;
using boost::algorithm::is_any_of;
using namespace boost::python;

namespace
{
        char const * MenubarMain =
                "<ui>"
                ""
                "<menubar name='MenubarMain'>"
                "   <menu action='MenuMusic'>"
                "         <menuitem action='action-play-files'/>"
                "         <separator name='sep00'/>"
                "         <menuitem action='action-quit'/>"
                "   </menu>"
                "   <menu action='MenuEdit'>"
#if 0
                "	      <menuitem action='action-mb-import'/>"
                "         <separator/>"
#endif
                "         <menuitem action='action-preferences'/>"
                "         <menuitem action='action-plugins'/>"
#ifdef HAVE_HAL
                "         <menuitem action='action-mlibmanager'/>"
#else
                "         <menuitem action='action-import-folder'/>"
                "         <menuitem action='action-import-share'/>"
                "	      <menuitem action='action-vacuum-lib'/>"
#endif // HAVE_HAL
                "   </menu>"
                "   <menu action='MenuView'>"
                "   </menu>"
                "   <menu action='MenuTrack'>"
                "         <placeholder name='placeholder-track-actions'/>"
                "   </menu>"
                "   <placeholder name='placeholder-source'/>"
                "   <menu action='MenuHelp'>"
                "         <menuitem action='action-about'/>"
                "   </menu>"
                "</menubar>"
                ""
                "</ui>"
                "";

        char const ACTION_PLAY [] = "action-play";
        char const ACTION_STOP [] = "action-stop";
        char const ACTION_NEXT [] = "action-next";
        char const ACTION_PREV [] = "action-prev";
        char const ACTION_PAUSE [] = "action-pause";
        char const ACTION_PLUGINS [] = "action-plugins";
        char const ACTION_PREFERENCES[] ="action-preferences";
        char const ACTION_SHOW_INFO[] = "action-show-info";
        char const ACTION_SHOW_VIDEO[] = "action-show-video";
        char const ACTION_SHOW_SOURCES[] = "action-show-sources";
}

namespace MPX
{
#define TYPE_DBUS_OBJ_MPX (DBusMPX::get_type ())
#define DBUS_OBJ_MPX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DBUS_OBJ_MPX, DBusMPX))

        struct DBusMPXClass
        {
                GObjectClass parent;
        };

        struct Player::DBusMPX
        {
                GObject parent;
                Player * player;

                enum
                {
                        SIGNAL_STARTUP_COMPLETE,
                        SIGNAL_SHUTDOWN_COMPLETE,
                        SIGNAL_QUIT,
                        N_SIGNALS,
                };

                static guint signals[N_SIGNALS];

                static gpointer parent_class;

                static GType
                        get_type ();

                static DBusMPX *
                        create (Player &, DBusGConnection*);

                static void
                        class_init (gpointer klass,
                                        gpointer class_data);

                static GObject *
                        constructor (GType                   type,
                                        guint                   n_construct_properties,
                                        GObjectConstructParam * construct_properties);

                static gboolean
                        startup (DBusMPX * self,
                                        GError ** error);

                static void
                        startup_complete (DBusMPX * self);

                static void
                        shutdown_complete (DBusMPX * self);

                static void
                        quit (DBusMPX * self);
        };

        gpointer Player::DBusMPX::parent_class       = 0;
        guint    Player::DBusMPX::signals[N_SIGNALS] = { 0 };

        // HACK: Hackery to rename functions in glue
#define mpx_startup     startup
#define mpx_quit        quit

#include "dbus-obj-MPX-glue.h"

        void
                Player::DBusMPX::class_init (gpointer klass, gpointer class_data)
                {
                        parent_class = g_type_class_peek_parent (klass);

                        GObjectClass *gobject_class = reinterpret_cast<GObjectClass*>(klass);
                        gobject_class->constructor  = &DBusMPX::constructor;

                        signals[SIGNAL_STARTUP_COMPLETE] =
                                g_signal_new ("startup-complete",
                                                G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                                                GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                                                0,
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

                        signals[SIGNAL_SHUTDOWN_COMPLETE] =
                                g_signal_new ("shutdown-complete",
                                                G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                                                GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                                                0,
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

                        signals[SIGNAL_QUIT] =
                                g_signal_new ("quit",
                                                G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                                                GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                                                0,
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
                }

        GObject *
                Player::DBusMPX::constructor (GType                   type,
                                guint                   n_construct_properties,
                                GObjectConstructParam*  construct_properties)
                {
                        GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_properties, construct_properties);
                        return object;
                }

        Player::DBusMPX *
                Player::DBusMPX::create (Player & player, DBusGConnection * session_bus)
                {
                        dbus_g_object_type_install_info (TYPE_DBUS_OBJ_MPX, &dbus_glib_mpx_object_info);

                        DBusMPX * self = DBUS_OBJ_MPX (g_object_new (TYPE_DBUS_OBJ_MPX, NULL));
                        self->player = &player;

                        if(session_bus)
                        {
                                dbus_g_connection_register_g_object (session_bus, "/MPX", G_OBJECT(self));
                                g_message("%s: /MPX Object registered on session DBus", G_STRLOC);
                        }

                        return self;
                }

        GType
                Player::DBusMPX::get_type ()
                {
                        static GType type = 0;

                        if (G_UNLIKELY (type == 0))
                        {
                                static GTypeInfo const type_info =
                                {
                                        sizeof (DBusMPXClass),
                                        NULL,
                                        NULL,
                                        &class_init,
                                        NULL,
                                        NULL,
                                        sizeof (DBusMPX),
                                        0,
                                        NULL
                                };

                                type = g_type_register_static (G_TYPE_OBJECT, "MPX", &type_info, GTypeFlags (0));
                        }

                        return type;
                }

        gboolean
                Player::DBusMPX::startup  (DBusMPX* self,
                                GError** error)
                {
                        if(self->player->m_startup_complete)
                        {
                                self->player->present();
                        }

                        return TRUE;
                }

        void
                Player::DBusMPX::startup_complete (DBusMPX* self)
                {
                        g_signal_emit (self, signals[SIGNAL_STARTUP_COMPLETE], 0);
                        self->player->m_startup_complete = true;
                }

        void
                Player::DBusMPX::shutdown_complete (DBusMPX* self)
                {
                        g_signal_emit (self, signals[SIGNAL_SHUTDOWN_COMPLETE], 0);
                }

        void
                Player::DBusMPX::quit (DBusMPX *self)
                {
                        gtk_main_quit ();
                }

#define TYPE_DBUS_OBJ_PLAYER (DBusPlayer::get_type ())
#define DBUS_OBJ_PLAYER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DBUS_OBJ_PLAYER, DBusPlayer))

        struct DBusPlayerClass
        {
                GObjectClass parent;
        };

        struct Player::DBusPlayer
        {
                GObject parent;
                Player * player;

                enum
                {
                        PLAYER_SIGNAL_CAPS,
                        PLAYER_SIGNAL_STATE,
                        N_SIGNALS,
                };

                static guint signals[N_SIGNALS];

                static gpointer parent_class;

                static GType
                        get_type ();

                static DBusPlayer *
                        create (Player &, DBusGConnection*);

                static void
                        class_init (gpointer klass,
                                        gpointer class_data);

                static GObject *
                        constructor (GType                   type,
                                        guint                   n_construct_properties,
                                        GObjectConstructParam * construct_properties);

                static gboolean
                        get_metadata (DBusPlayer* self,
                                        GHashTable** metadata,
                                        GError** error);

                static gboolean
                        play_tracks (DBusPlayer* self,
                                        char** uris,
                                        gboolean play,
                                        GError** error);
        };

        gpointer Player::DBusPlayer::parent_class       = 0;
        guint    Player::DBusPlayer::signals[N_SIGNALS] = { 0 };

        // HACK: Hackery to rename functions in glue
#define player_next next 
#define player_prev prev
#define player_pause pause
#define player_stop stop
#define player_play play
#define player_get_metadata get_metadata
#define player_play_tracks play_tracks

#include "dbus-obj-PLAYER-glue.h"

        void
                Player::DBusPlayer::class_init (gpointer klass, gpointer class_data)
                {
                        parent_class = g_type_class_peek_parent (klass);

                        GObjectClass *gobject_class = reinterpret_cast<GObjectClass*>(klass);
                        gobject_class->constructor  = &DBusPlayer::constructor;
                }

        GObject *
                Player::DBusPlayer::constructor (GType                   type,
                                guint                   n_construct_properties,
                                GObjectConstructParam*  construct_properties)
                {
                        GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_properties, construct_properties);

                        return object;
                }

        Player::DBusPlayer *
                Player::DBusPlayer::create (Player & player, DBusGConnection * session_bus)
                {
                        dbus_g_object_type_install_info (TYPE_DBUS_OBJ_PLAYER, &dbus_glib_player_object_info);

                        DBusPlayer * self = DBUS_OBJ_PLAYER (g_object_new (TYPE_DBUS_OBJ_PLAYER, NULL));
                        self->player = &player;

                        if(session_bus)
                        {
                                dbus_g_connection_register_g_object (session_bus, "/Player", G_OBJECT(self));
                                g_message("%s: /Player Object registered on session DBus", G_STRLOC);
                        }

                        return self;
                }

        GType
                Player::DBusPlayer::get_type ()
                {
                        static GType type = 0;

                        if (G_UNLIKELY (type == 0))
                        {
                                static GTypeInfo const type_info =
                                {
                                        sizeof (DBusPlayerClass),
                                        NULL,
                                        NULL,
                                        &class_init,
                                        NULL,
                                        NULL,
                                        sizeof (DBusPlayer),
                                        0,
                                        NULL
                                };

                                type = g_type_register_static (G_TYPE_OBJECT, "Player", &type_info, GTypeFlags (0));
                        }

                        return type;
                }

        const char* mpris_attribute_id_str[] =
        {
                "location",
                "title",
                "genre",
                "comment",
                "puid fingerprint",
                "mpx tag hash",
                "mb track id",
                "artist",
                "mb artist sort name",
                "mb artist id",
                "album",
                "mb album id",
                "mb release date",
                "mb release country",
                "mb release type",
                "asin",
                "mb album artist",
                "mb album artist sort name",
                "mb album artist id",
                "mime type",
                "mpx hal volume udi",
                "mpx hal device udi",
                "mpx hal volume relative path",
                "mpx insert path",
                "mpx location name",
        };

        const char* mpris_attribute_id_int[] =
        {
                "tracknumber",
                "time",
                "rating",
                "year",
                "mtime",
                "audio-bitrate",
                "audio-samplerate",
                "mpx play count",
                "mpx play date",
                "mpx insert date",
                "mpx is mb album artist",
                "mpx active",
                "mpx track id",
                "mpx album id",
        };

        gboolean
                Player::DBusPlayer::get_metadata (DBusPlayer* self,
                                GHashTable** metadata,
                                GError** error)
                {
                        MPX::Metadata m = self->player->get_metadata();

                        GHashTable * table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
                        GValue* value = 0;

                        for(int n = ATTRIBUTE_LOCATION; n < N_ATTRIBUTES_STRING; ++n)
                        {
                                if(m[n].is_initialized())
                                {
                                        value = g_new0(GValue,1);
                                        g_value_init(value, G_TYPE_STRING);
                                        g_value_set_string(value, (boost::get<std::string>(m[n].get())).c_str());
                                        g_hash_table_insert(table, g_strdup(mpris_attribute_id_str[n-ATTRIBUTE_LOCATION]), value);
                                }
                        }

                        for(int n = ATTRIBUTE_TRACK; n < N_ATTRIBUTES_INT; ++n)
                        {
                                if(m[n].is_initialized())
                                {
                                        value = g_new0(GValue,1);
                                        g_value_init(value, G_TYPE_INT64);
                                        g_value_set_int64(value, boost::get<gint64>(m[n].get()));
                                        g_hash_table_insert(table, g_strdup(mpris_attribute_id_int[n-ATTRIBUTE_TRACK]), value);
                                }
                        }

                        *metadata = table;
                        return TRUE;
                }

        gboolean
                Player::DBusPlayer::play_tracks (DBusPlayer* self,
                                char** uris,
                                gboolean play,
                                GError** error)
                {
                        Util::FileList list;
                        if( uris )
                        {
                                while( *uris )
                                {
                                        list.push_back( *uris );
                                        ++uris;
                                }
                        }
                        self->player->play_tracks(list, bool(play));
                        return TRUE;
                }


        Player::Player(
                        const Glib::RefPtr<Gnome::Glade::Xml>&  xml,
                        MPX::Service::Manager&                  services
        )
        : WidgetLoader<Gtk::Window>(xml, "mpx")
        , sigx::glib_auto_dispatchable()
        , Service::Base("mpx-service-player")
        , m_ref_xml(xml)
        , m_startup_complete(false)
        , m_Caps(C_NONE)
        , m_NextSourceId(0)
        , m_SourceUI(0)
        , m_Covers(*(services.get<Covers>("mpx-service-covers")))
#ifdef HAVE_HAL
        , m_HAL(*(services.get<HAL>("mpx-service-hal")))
#endif // HAVE_HAL
        , m_Library(*(services.get<Library>("mpx-service-library")))
        , m_Play(*(services.get<Play>("mpx-service-play")))
        , m_NewTrack(false)
        {
                                  m_MarkovThread = new MarkovAnalyzerThread(m_Library);

                                  m_ErrorManager = new ErrorManager;
                                  m_AboutDialog = new AboutDialog;

                                  Splashscreen splash;
                                  splash.set_message(_("Startup..."), 0.);

                                  mpx_py_init ();

                                  m_ref_xml->get_widget("statusbar", m_Statusbar);
                                  m_ref_xml->get_widget("notebook-info", m_InfoNotebook);
                                  m_ref_xml->get_widget("volumebutton", m_Volume);
                                  m_ref_xml->get_widget("label-time", m_TimeLabel);
                                  m_ref_xml->get_widget("scale-seek", m_Seek);

                                  m_PluginManager = new PluginManager(this);
                                  m_Sidebar = new Sidebar(m_ref_xml, *m_PluginManager);

                                  try{

                                          std::list<Glib::RefPtr<Gdk::Pixbuf> > icon_list;

                                          icon_list.push_back(Gdk::Pixbuf::create_from_file(
                                                                  build_filename(
                                                                          DATA_DIR,
                                                                          "icons" G_DIR_SEPARATOR_S "mpx.png"
                                                                          )));

                                          icon_list.push_back(Gdk::Pixbuf::create_from_file(
                                                                  build_filename(
                                                                          DATA_DIR,
                                                                          "icons" G_DIR_SEPARATOR_S "mpx_128.png"
                                                                          )));

                                          icon_list.push_back(Gdk::Pixbuf::create_from_file(
                                                                  build_filename(
                                                                          DATA_DIR,
                                                                          "icons" G_DIR_SEPARATOR_S "mpx_64.png"
                                                                          )));

                                          icon_list.push_back(Gdk::Pixbuf::create_from_file(
                                                                  build_filename(
                                                                          DATA_DIR,
                                                                          "icons" G_DIR_SEPARATOR_S "mpx_48.png"
                                                                          )));

                                          icon_list.push_back(Gdk::Pixbuf::create_from_file(
                                                                  build_filename(
                                                                          DATA_DIR,
                                                                          "icons" G_DIR_SEPARATOR_S "mpx_16.png"
                                                                          )));

                                          set_icon_list(icon_list);

                                  } catch (Gdk::PixbufError & cxe)
                                  {
                                          g_warning("%s: Couldn't set main window icon", G_STRLOC);
                                  }

                                  IconTheme::get_default()->prepend_search_path(build_filename(DATA_DIR,"icons"));
                                  register_default_stock_icons();

                                  splash.set_message(_("Initializing Playback Engine"), 0.2);

                                  m_Play.signal_eos().connect(
                                                  sigc::mem_fun( *this, &MPX::Player::on_play_eos
                                                          ));

                                  m_Play.signal_position().connect(
                                                  sigc::mem_fun( *this, &MPX::Player::on_play_position
                                                          ));

                                  m_Play.signal_metadata().connect(
                                                  sigc::mem_fun( *this, &MPX::Player::on_play_metadata
                                                          ));

                                  m_Play.signal_buffering().connect(
                                                  sigc::mem_fun( *this, &MPX::Player::on_play_buffering
                                                          ));

                                  m_Play.property_status().signal_changed().connect(
                                                  sigc::mem_fun( *this, &MPX::Player::on_play_status_changed
                                                          ));

                                  m_Play.signal_stream_switched().connect(
                                                  sigc::mem_fun( *this, &MPX::Player::on_play_stream_switched
                                                          ));

                                  m_Play.signal_spectrum().connect(
                                                  sigc::mem_fun( *this, &MPX::Player::on_play_update_spectrum )
                                                  );

                                  m_VideoWidget = manage(new VideoWidget(&m_Play));
                                  m_VideoWidget->show ();
                                  gtk_widget_realize (GTK_WIDGET (m_VideoWidget->gobj()));

                                  if( m_Play.has_video() )
                                  {
                                          m_Play.signal_request_window_id ().connect
                                                  (sigc::mem_fun( *this, &Player::on_gst_set_window_id ));
                                          m_Play.signal_video_geom ().connect
                                                  (sigc::mem_fun( *this, &Player::on_gst_set_window_geom ));
                                  }

                                  m_Preferences = Preferences::create(m_Play);

#ifdef HAVE_HAL
                                  m_MLibManager = MLibManager::create(m_HAL, m_Library);
#endif // HAVE_HAL

                                  m_scrolllock_mask   = 0;
                                  m_numlock_mask      = 0;
                                  m_capslock_mask     = 0;
                                  mmkeys_get_offending_modifiers ();
                                  on_mm_edit_done (); // bootstraps the settings

                                  m_Preferences->signal_mm_edit_begin().connect(
                                                  sigc::mem_fun( *this, &Player::on_mm_edit_begin
                                                          ));

                                  m_Preferences->signal_mm_edit_done().connect(
                                                  sigc::mem_fun( *this, &Player::on_mm_edit_done
                                                          ));

                                  signals[PSIGNAL_NEW_TRACK] =
                                          g_signal_new ("new-track",
                                                          G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                          GSignalFlags (G_SIGNAL_RUN_FIRST),
                                                          0,
                                                          NULL, NULL,
                                                          g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

                                  signals[PSIGNAL_TRACK_PLAYED] =
                                          g_signal_new ("track-played",
                                                          G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                          GSignalFlags (G_SIGNAL_RUN_FIRST),
                                                          0,
                                                          NULL, NULL,
                                                          g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

                                  signals[PSIGNAL_INFOAREA_CLICK] =
                                          g_signal_new ("infoarea-click",
                                                          G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                          GSignalFlags (G_SIGNAL_RUN_FIRST),
                                                          0,
                                                          NULL, NULL,
                                                          g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

                                  signals[PSIGNAL_STATUS_CHANGED] =
                                          g_signal_new ("play-status-changed",
                                                          G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                          GSignalFlags (G_SIGNAL_RUN_FIRST),
                                                          0,
                                                          NULL, NULL,
                                                          g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT); 

                                  signals[PSIGNAL_METADATA_PREPARE] =
                                          g_signal_new ("metadata-prepare",
                                                          G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                          GSignalFlags (G_SIGNAL_RUN_FIRST),
                                                          0,
                                                          NULL, NULL,
                                                          g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

                                  signals[PSIGNAL_METADATA_UPDATED] =
                                          g_signal_new ("metadata-updated",
                                                          G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                          GSignalFlags (G_SIGNAL_RUN_FIRST),
                                                          0,
                                                          NULL, NULL,
                                                          g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

                                  signals[PSIGNAL_NEW_COVERART] =
                                          g_signal_new ("new-coverart",
                                                          G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                          GSignalFlags (G_SIGNAL_RUN_FIRST),
                                                          0,
                                                          NULL, NULL,
                                                          g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 0);

                                  signals[PSIGNAL_NEW_SOURCE] =
                                          g_signal_new ("new-source",
                                                          G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                                                          GSignalFlags (G_SIGNAL_RUN_FIRST),
                                                          0,
                                                          NULL, NULL,
                                                          g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1, g_type_from_name("PyObject")); 

                                  splash.set_message(_("Setting up DBus"), 0.4);

                                  init_dbus ();
                                  DBusObjects.mpx = DBusMPX::create(*this, m_SessionBus);
                                  DBusObjects.player = DBusPlayer::create(*this, m_SessionBus);

                                  splash.set_message(_("Loading Plugins"), 0.6);

                                  /*- Connect Library -----------------------------------------------*/ 

                                  m_Library.scanner().signal_scan_start().connect(
                                                  sigc::mem_fun( *this, &Player::on_library_scan_start
                                                          ));

                                  m_Library.scanner().signal_scan_run().connect(
                                                  sigc::mem_fun( *this, &Player::on_library_scan_run
                                                          ));

                                  m_Library.scanner().signal_scan_end().connect(
                                                  sigc::mem_fun( *this, &Player::on_library_scan_end
                                                          ));

                                  /*- UIManager + Menus + Proxy Widgets -----------------------------*/

                                  m_ui_manager = UIManager::create ();

                                  m_actions = ActionGroup::create ("Actions_Player");

                                  m_actions->add(Action::create(
                                                          "MenuMusic",
                                                          _("_Music")
                                                          ));

                                  m_actions->add(Action::create(
                                                          "MenuEdit",
                                                          _("_Edit")
                                                          ));

                                  m_actions->add(Action::create(
                                                          "MenuView",
                                                          _("_View")
                                                          ));

                                  m_actions->add(Action::create(
                                                          "MenuTrack",
                                                          _("_Track")
                                                          ));

                                  m_actions->add(Action::create(
                                                          "MenuHelp",
                                                          _("_Help")
                                                          ));

                                  m_actions->add (Action::create ("action-play-files",
                                                          Gtk::Stock::OPEN,
                                                          _("_Play Files...")),
                                                  sigc::mem_fun (*this, &Player::on_action_cb_play_files));

#ifndef HAVE_HAL
                                  m_actions->add (Action::create ("action-import-folder",
                                                          Gtk::Stock::HARDDISK,
                                                          _("Import _Folder")),
                                                  sigc::mem_fun (*this, &Player::on_import_folder));

                                  m_actions->add (Action::create ("action-import-share",
                                                          Gtk::Stock::NETWORK,
                                                          _("Import _Share")),
                                                  sigc::mem_fun (*this, &Player::on_import_share));

                                  m_actions->add (Action::create("action-vacuum-lib",
                                                          _("_Vacuum Music Library"),
                                                          _("Remove dangling files")),
                                                  sigc::mem_fun (m_Library, &Library::vacuum));
#else

                                  m_actions->add (Action::create ("action-mlibmanager",
                                                          _("_Music Library..."),
                                                          _("Add or Remove Music")),
                                                  sigc::mem_fun (*m_MLibManager, &MLibManager::present));
#endif

                                  m_actions->add (Action::create("action-mb-import",
                                                          _("MusicBrainz: Import Album"),
                                                          _("Import an album using MusicBrainz")),
                                                  sigc::mem_fun (*this, &Player::on_import_album));


                                  m_actions->add (Action::create (ACTION_PLUGINS,
                                                          Gtk::StockID(MPX_STOCK_PLUGIN),
                                                          _("Plugins..."),
                                                          _("View and Enable or Disable Plugins")),
                                                  sigc::mem_fun (*this, &Player::on_action_cb_show_plugins ));



                                  m_actions->add (Action::create (ACTION_PREFERENCES,
                                                          Gtk::Stock::EXECUTE,
                                                          _("_Preferences..."),
                                                          _("Set up Audio and other Settings")),
                                                  sigc::mem_fun (*m_Preferences, &Gtk::Widget::show ));



                                  m_actions->add (Action::create ("action-quit",
                                                          Gtk::Stock::QUIT,
                                                          _("_Quit")),
                                                  AccelKey("<ctrl>Q"),
                                                  sigc::ptr_fun ( &Gtk::Main::quit ));


                                  m_actions->add (Action::create ("action-about",
                                                          Gtk::Stock::ABOUT,
                                                          _("_About")),
                                                  sigc::mem_fun ( *m_AboutDialog, &AboutDialog::present ));


                                  m_actions->add (Action::create (ACTION_PLAY,
                                                          Gtk::Stock::MEDIA_PLAY,
                                                          _("Play")),
                                                  sigc::mem_fun (*this, &Player::on_controls_play ));



                                  m_actions->add (ToggleAction::create (ACTION_PAUSE,
                                                          Gtk::Stock::MEDIA_PAUSE,
                                                          _("Pause")),
                                                  sigc::mem_fun (*this, &Player::on_controls_pause ));


                                  m_actions->add (Action::create (ACTION_STOP,
                                                          Gtk::Stock::MEDIA_STOP,
                                                          _("Stop")),
                                                  sigc::mem_fun (*this, &Player::stop ));


                                  m_actions->add (Action::create (ACTION_NEXT,
                                                          Gtk::Stock::MEDIA_NEXT,
                                                          _("Next")),
                                                  sigc::mem_fun (*this, &Player::on_controls_next ));


                                  m_actions->add (Action::create (ACTION_PREV,
                                                          Gtk::Stock::MEDIA_PREVIOUS,
                                                          _("Prev")),
                                                  sigc::mem_fun (*this, &Player::on_controls_prev ));


                                  m_ui_manager->insert_action_group (m_actions);

                                  if(Util::ui_manager_add_ui(m_ui_manager, MenubarMain, *this, _("Main Menubar")))
                                  {
                                          dynamic_cast<Alignment*>(
                                                          m_ref_xml->get_widget("alignment-menu")
                                                          )->add(
                                                                  *(m_ui_manager->get_widget ("/MenubarMain"))
                                                                );
                                  }

                                  /*- Playback Controls ---------------------------------------------*/ 

                                  m_actions->get_action (ACTION_PLAY)->connect_proxy
                                          (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-play"))));
                                  m_actions->get_action (ACTION_PREV)->connect_proxy
                                          (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-previous"))));
                                  m_actions->get_action (ACTION_NEXT)->connect_proxy
                                          (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-next"))));
                                  m_actions->get_action (ACTION_PAUSE)->connect_proxy
                                          (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-pause"))));
                                  m_actions->get_action (ACTION_STOP)->connect_proxy
                                          (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-stop"))));

                                  add_accel_group (m_ui_manager->get_accel_group());

                                  /*- Setup Window Geometry -----------------------------------------*/ 

                                  resize(
                                                  mcs->key_get<int>("mpx","window-w"),
                                                  mcs->key_get<int>("mpx","window-h")
                                        );

                                  move(
                                                  mcs->key_get<int>("mpx","window-x"),
                                                  mcs->key_get<int>("mpx","window-y")
                                      );


                                  while (gtk_events_pending())
                                    gtk_main_iteration();

                                  /*- Load Sources --------------------------------------------------*/ 

                                  splash.set_message(_("Loading Sources"), 0.8);

                                  m_Sidebar->signal_id_changed().connect(
                                                  sigc::mem_fun(
                                                          *this,
                                                          &Player::on_source_changed
                                                          ));

                                  m_Sidebar->addItem(
                                                  _("Now Playing"),
                                                  m_VideoWidget,
                                                  0,
                                                  Gdk::Pixbuf::create_from_file(
                                                          build_filename(
                                                                  DATA_DIR,
                                                                  "images" G_DIR_SEPARATOR_S "icon-now-playing-tab.png"
                                                                  )
                                                          )->scale_simple(20,20,Gdk::INTERP_BILINEAR),
                                                  0 
                                                  );

                                  m_Sidebar->addItem(
                                                  _("Plugins"),
                                                  m_InfoNotebook,
                                                  0,
                                                  Gdk::Pixbuf::create_from_file(
                                                          build_filename(
                                                                  DATA_DIR,
                                                                  "images" G_DIR_SEPARATOR_S "icon-plugins-tab.png"
                                                                  )
                                                          )->scale_simple(20,20,Gdk::INTERP_BILINEAR),
                                                  1 
                                                  );

                                  m_NextSourceId = 2;

                                  std::string sources_path = build_filename(PLUGIN_DIR, "sources");
                                  if(file_test(sources_path, FILE_TEST_EXISTS))
                                  {
                                          Util::dir_for_each_entry(
                                                          sources_path,
                                                          sigc::mem_fun(*this, &MPX::Player::source_load)
                                                          );  
                                  }

                                  /*- Volume ---------------------------------------------------------*/

                                  std::vector<Glib::ustring> Icons;
                                  Icons.push_back("audio-volume-muted");
                                  Icons.push_back("audio-volume-high");
                                  Icons.push_back("audio-volume-low");
                                  Icons.push_back("audio-volume-medium");
                                  m_Volume->property_size() = Gtk::ICON_SIZE_SMALL_TOOLBAR;
                                  m_Volume->set_icons(Icons);

                                  m_Volume->signal_value_changed().connect(
                                                  sigc::mem_fun( *this, &Player::on_volume_value_changed
                                                          ));

                                  m_Volume->set_value(double(mcs->key_get<int>("mpx", "volume")));

                                  /*- Seek -----------------------------------------------------------*/

                                  m_Seek->signal_event().connect(
                                                  sigc::mem_fun( *this, &Player::on_seek_event
                                                          ));

                                  g_atomic_int_set(&m_Seeking,0);

                                  /*- Infoarea--------------------------------------------------------*/

                                  m_InfoArea = InfoArea::create(m_ref_xml, "infoarea");

                                  m_InfoArea->signal_cover_clicked().connect(
                                        sigc::mem_fun(
                                                *this,
                                                &Player::on_cb_album_cover_clicked
                                  ));

                                  m_InfoArea->signal_uris_dropped().connect(
                                        sigc::bind(
                                                sigc::mem_fun(
                                                    *this,
                                                    &Player::play_tracks
                                                ),
                                                true
                                  ));

                                  /*- Load Plugins -------------------------------------------------*/

                                  std::string const user_path =
                                          build_filename(
                                                          build_filename(
                                                                  g_get_user_data_dir(),
                                                                  "mpx"),
                                                          "python-plugins"
                                                        );

                                  if(file_test(user_path, FILE_TEST_EXISTS))
                                  {
                                          m_PluginManager->append_search_path (user_path);
                                  }

                                  m_PluginManager->append_search_path
                                          (build_filename(
                                                          DATA_DIR,
                                                          "python-plugins"
                                                         ));

                                  m_PluginManager->load_plugins();
                                  m_PluginManager->activate_plugins();
                                  m_PluginManagerGUI = PluginManagerGUI::create(*m_PluginManager);

                                  m_MarkovThread->run();

                                  translate_caps(); // sets all actions intially insensitive as we have C_NONE

                                  m_Covers.signal_got_cover().connect(
                                    sigc::mem_fun(
                                        *this,
                                        &Player::on_got_cover
                                  ));

                                  m_MB_ImportAlbum = MB_ImportAlbum::create(m_Library,m_Covers);


                                  /*- Auto Rescanning Volumes -------------------------------------------------*/

                                  if(mcs->key_get<bool>("library","rescan-at-startup"))
                                  {
                                    splash.set_message(_("Rescanning Volumes"), 1.0);
                                    m_MLibManager->rescan_all_volumes();
                                  }

                                  mcs->subscribe(
                                    "MPX-Player-RescanSubscription",
                                    "library",
                                    "rescan-in-intervals",
                                    sigc::mem_fun(
                                        *this,
                                        &Player::on_rescan_in_intervals_changed
                                  ));

                                  sigc::slot<bool> slot = sigc::mem_fun(*this, &Player::on_rescan_timeout);
                                  sigc::connection conn = Glib::signal_timeout().connect(slot, 1000);
                                  m_rescan_timer.start();

                                  show ();

                                  DBusObjects.mpx->startup_complete(DBusObjects.mpx);

                                  splash.set_message(_("Ready"), 1.0);
                          }

        Player*
                Player::create (MPX::Service::Manager&services)
                {
                        const std::string path (build_filename(DATA_DIR, build_filename("glade","mpx.glade")));
                        Player * p = new Player(Gnome::Glade::Xml::create (path), services);
                        return p;
                }

        Player::~Player()
        {
                for( SourcesMap::iterator i = m_Sources.begin(); i != m_Sources.end(); ++i )
                {
                    delete (*i).second;
                }

                Gtk::Window::get_position( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-x")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-y")));
                Gtk::Window::get_size( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-w")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-h")));
                DBusObjects.mpx->shutdown_complete(DBusObjects.mpx); 
                g_object_unref(G_OBJECT(DBusObjects.mpx));
                delete m_PluginManager;
        }

        void
                Player::init_dbus ()
                {
                        DBusGProxy * m_SessionBus_proxy;
                        guint dbus_request_name_result;
                        GError * error = NULL;

                        m_SessionBus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
                        if (!m_SessionBus)
                        {
                                g_error_free (error);
                                return;
                        }

                        m_SessionBus_proxy = dbus_g_proxy_new_for_name (m_SessionBus,
                                        "org.freedesktop.DBus",
                                        "/org/freedesktop/DBus",
                                        "org.freedesktop.DBus");

                        if (!dbus_g_proxy_call (m_SessionBus_proxy, "RequestName", &error,
                                                G_TYPE_STRING, "info.backtrace.mpx",
                                                G_TYPE_UINT, 0,
                                                G_TYPE_INVALID,
                                                G_TYPE_UINT, &dbus_request_name_result,
                                                G_TYPE_INVALID))
                        {
                                g_critical ("%s: RequestName Request Failed: %s", G_STRFUNC, error->message);
                                g_error_free (error);
                                error = NULL;
                        }
                        else
                        {
                                switch (dbus_request_name_result)
                                {
                                        case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
                                                {
                                                        break;
                                                }

                                        case DBUS_REQUEST_NAME_REPLY_EXISTS:
                                                {
                                                        g_object_unref(m_SessionBus);
                                                        m_SessionBus = NULL;
                                                        return;
                                                }
                                }
                        }

                        g_message("%s: info.backtrace.mpx service registered on session DBus", G_STRLOC);
                }

        void
                Player::play_tracks (Util::FileList const& uris, bool play)
                {
                        if( uris.size() )
                        {
                                try{
                                        URI u (uris[0]); //TODO/FIXME: We assume all URIs are of the same scheme
                                        UriSchemeMap::const_iterator i = m_UriMap.find (u.scheme);
                                        if( i != m_UriMap.end ())
                                        {
                                                m_Sources[i->second]->process_uri_list( uris, play );
                                        }
                                }
                                catch (URI::ParseError & cxe)
                                {
                                        g_warning (G_STRLOC ": Couldn't parse URI %s", uris[0].c_str());
                                }
                        }
                }

        void
                Player::play_uri (std::string const& uri)
                {
                        Util::FileList uris;
                        uris.push_back(uri);
                        try{
                                URI u (uris[0]); //TODO/FIXME: We assume all URIs are of the same scheme
                                UriSchemeMap::const_iterator i = m_UriMap.find (u.scheme);
                                if( i != m_UriMap.end ())
                                {
                                        m_Sources[i->second]->process_uri_list( uris, true );
                                }
                        }
                        catch (URI::ParseError & cxe)
                        {
                                g_warning (G_STRLOC ": Couldn't parse URI %s", uris[0].c_str());
                        }
                }

        void
                Player::on_action_cb_play_files ()
                {
                        typedef boost::shared_ptr<FileBrowser> FBRefP;
                        FBRefP p (FileBrowser::create());
                        int response = p->run ();
                        p->hide ();

                        if( response == GTK_RESPONSE_OK )
                        {
                                Util::FileList uris = p->get_uris ();

                                if( uris.size() ) 
                                {
                                        try{
                                                URI u (uris[0]); //TODO/FIXME: We assume all URIs are of the same scheme
                                                UriSchemeMap::const_iterator i = m_UriMap.find (u.scheme);
                                                if( i != m_UriMap.end ())
                                                {
                                                        m_Sources[i->second]->process_uri_list( uris, true );
                                                }
                                        }
                                        catch (URI::ParseError & cxe)
                                        {
                                                g_warning (G_STRLOC ": Couldn't parse URI %s", uris[0].c_str());
                                        }
                                }
                        }
                }

        void
                Player::on_action_cb_show_plugins ()
                {
                        m_PluginManagerGUI->present ();
                }

        void
                Player::get_object (PAccess<MPX::Library> & pa)
                {
                        pa = PAccess<MPX::Library>(m_Library);
                }

        void	
                Player::get_object (PAccess<MPX::Covers> & pa)
                {
                        pa = PAccess<MPX::Covers>(m_Covers);
                }

        void
                Player::get_object (PAccess<MPX::Play> & pa)
                {
                        pa = PAccess<MPX::Play>(m_Play);
                }

#ifdef HAVE_HAL
        void
                Player::get_object (PAccess<MPX::HAL> & pa)
                {
                        pa = PAccess<MPX::HAL>(m_HAL);
                }
#endif // HAVE_HAL

        Glib::RefPtr<Gtk::UIManager>&
                Player::ui ()
                {
                        return m_ui_manager;
                }

        PlayStatus
                Player::get_status ()
                {
                        return PlayStatus(m_Play.property_status().get_value());
                }


        Metadata&
                Player::get_metadata ()
                {
                        if(m_Metadata)
                        {
                                return m_Metadata.get();
                        }
                        else
                        {
                                throw std::runtime_error("No Current Metadata");
                        }
                }

        void
                Player::add_widget (Gtk::Widget *widget)
                {
                        dynamic_cast<Gtk::Box*>(m_ref_xml->get_widget("vbox3"))->pack_start(*widget);
                }

        void
                Player::add_info_widget (Gtk::Widget *widget, std::string const& name)
                {
                        widget->show();
                        m_InfoNotebook->append_page(*widget, name);
                        m_InfoWidgetMap.insert(std::make_pair(widget, widget));
                }

        void
                Player::remove_widget (Gtk::Widget *widget)
                {
                        Gtk::VBox * box;
                        m_ref_xml->get_widget("vbox-top", box);
                        box->remove(*widget);
                }

        void
                Player::remove_info_widget (Gtk::Widget *widget)
                {
                        WidgetWidgetMap::iterator i = m_InfoWidgetMap.find(widget);
                        if( i != m_InfoWidgetMap.end() )
                        {
                                m_InfoNotebook->remove(*((*i).second));
                        }
                }

        void
                Player::on_volume_value_changed (double volume)
                {
                        m_Play.property_volume() = volume*100;	
                        mcs->key_set("mpx","volume", int(volume*100));
                }

        bool
                Player::on_seek_event (GdkEvent *event)
                {
                        if( event->type == GDK_KEY_PRESS )
                        {
                                GdkEventKey * ev = ((GdkEventKey*)(event));
                                gint64 status = m_Play.property_status().get_value();
                                if((status == PLAYSTATUS_PLAYING) || (status == PLAYSTATUS_PAUSED))
                                {
                                        gint64 pos = m_Play.property_position().get_value();

                                        int delta = (ev->state & GDK_SHIFT_MASK) ? 1 : 15;

                                        if(ev->keyval == GDK_Left)
                                        {
                                                m_Play.seek( pos - delta );
                                                m_TrackSeekedSeconds -= delta;
                                                if( m_TrackSeekedSeconds < 0 )
                                                {
                                                    m_TrackPlayedSeconds += m_TrackSeekedSeconds;
                                                    m_TrackSeekedSeconds = 0;
                                                }
                                                return true;
                                        }
                                        else if(ev->keyval == GDK_Right)
                                        {
                                                m_Play.seek( pos + delta );
                                                m_TrackSeekedSeconds += delta;
                                                return true;
                                        }
                                }
                                return false;
                        }
                        else if( event->type == GDK_BUTTON_PRESS )
                        {
                                g_atomic_int_set(&m_Seeking,1);
                                m_PreSeekPosition = m_Play.property_position().get_value();
                                goto SET_SEEK_POSITION;
                        }
                        else if( event->type == GDK_BUTTON_RELEASE && g_atomic_int_get(&m_Seeking))
                        {
                                g_atomic_int_set(&m_Seeking,0);
                                if(m_PreSeekPosition > m_Seek->get_value())
                                {
                                        double delta = (m_PreSeekPosition - m_Seek->get_value());
                                        m_TrackSeekedSeconds -= delta;
                                        if( m_TrackSeekedSeconds < 0 )
                                        {
                                            m_TrackPlayedSeconds += m_TrackSeekedSeconds;
                                            m_TrackSeekedSeconds = 0;
                                        }
                                }
                                m_Play.seek (gint64(m_Seek->get_value()));
                        }
                        else if( event->type == GDK_MOTION_NOTIFY && g_atomic_int_get(&m_Seeking))
                        {
SET_SEEK_POSITION:

                                guint64 duration = m_Play.property_duration().get_value();
                                guint64 position = m_Seek->get_value();

                                guint64 m_pos = position / 60;
                                guint64 m_dur = duration / 60;
                                guint64 s_pos = position % 60;
                                guint64 s_dur = duration % 60;

                                static boost::format time_f ("%02d:%02d … %02d:%02d");

                                m_TimeLabel->set_text ((time_f % m_pos % s_pos % m_dur % s_dur).str());
                        }
                        return false;
                }

        void
                Player::on_play_update_spectrum (Spectrum const& spectrum)
                {
                        m_InfoArea->update_spectrum(spectrum);
                }

        void
                Player::on_play_metadata (GstMetadataField field)
                {
                        Glib::Mutex::Lock L (m_MetadataLock);

                        GstMetadata const& m = m_Play.get_metadata();

                        switch (field)
                        {
                                case FIELD_IMAGE:
                                        if(!m_Metadata.get().Image)
                                        {
                                                m_Metadata.get().Image = m.m_image.get();

                                                if(m_Metadata.get().Image)
                                                {
                                                        m_InfoArea->set_cover (m.m_image.get()->scale_simple (72, 72, Gdk::INTERP_HYPER), m_NewTrack);

                                                        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                                                        g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_NEW_COVERART], 0);
                                                        check_py_error();
                                                        pyg_gil_state_release(state);
                                                }

                                                g_atomic_int_set(&m_NewTrack, 0);
                                        }
                                        return;

                                case FIELD_TITLE:
                                        if(m_Metadata.get()[ATTRIBUTE_TITLE])
                                        {
                                                std::string album = get<std::string>(m_Metadata.get()[ATTRIBUTE_TITLE].get());
                                                if(album == m.m_title.get())
                                                        return;
                                        }

                                        m_Metadata.get()[ATTRIBUTE_TITLE] = std::string(m.m_title.get());
                                        metadata_reparse();
                                        return;

                                case FIELD_ALBUM:
                                        if(m_Metadata.get()[ATTRIBUTE_ALBUM])
                                        {
                                                std::string album = get<std::string>(m_Metadata.get()[ATTRIBUTE_ALBUM].get());
                                                if(album == m.m_album.get())
                                                        return;
                                        }

                                        m_Metadata.get()[ATTRIBUTE_ALBUM] = std::string(m.m_album.get());
                                        metadata_reparse();
                                        break;

                                case FIELD_AUDIO_BITRATE:
                                        break;

                                case FIELD_AUDIO_CODEC:
                                        break;

                                case FIELD_VIDEO_CODEC:
                                        break;
                        }
                }

        void
                Player::on_play_position (guint64 position)
                {
                        if (g_atomic_int_get(&m_Seeking))
                                return;

                        guint64 duration = m_Play.property_duration().get_value();

                        if( (duration > 0) && (position <= duration) && (position >= 0) )
                        {
                                if (duration <= 0)
                                        return;

                                if (position < 0)
                                        return;

                                guint64 m_pos = position / 60;
                                guint64 s_pos = position % 60;
                                guint64 m_dur = duration / 60;
                                guint64 s_dur = duration % 60;

                                static boost::format time_f ("%02d:%02d … %02d:%02d");

                                m_TimeLabel->set_text((time_f % m_pos % s_pos % m_dur % s_dur).str());
                                m_Seek->set_range(0., duration);
                                m_Seek->set_value(double (position));

                                m_TrackPlayedSeconds += 0.5; // this is slightly incorrect, the tick is every 500ms, but nothing says that the time always also progresses by exactly 0.5s
                                m_TrackDuration = m_Play.property_duration().get_value();
                        }
                }

        void
                Player::on_play_buffering (double size)
                {
                        if( size >= 1.0 )
                        {
                                m_InfoArea->set_info((boost::format (_("Buffering: %d")) % (int(size*100))).str());
                                m_InfoArea->clear_info();
                        }
                        else
                        {
                                m_InfoArea->set_info((boost::format (_("Buffering: %d")) % (int(size*100))).str());
                        }
                }

        void
                Player::on_cb_album_cover_clicked ()
                {
                        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                        g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_INFOAREA_CLICK], 0);
                        check_py_error();
                        pyg_gil_state_release(state);
                }

        //////////////////////////////// Internal Playback Control

        void
                Player::switch_stream(std::string const& uri, std::string const& type)
                {
                        m_Play.switch_stream( uri, type );
                }

        void
                Player::track_played ()
                {
                        Glib::Mutex::Lock L (m_MetadataLock);

                        if(
                                    m_Metadata
                                        && 
                                    m_Metadata.get()[ATTRIBUTE_MPX_TRACK_ID]
                                        && 
                                    ((m_TrackPlayedSeconds >= 240) || (m_TrackPlayedSeconds >= m_TrackDuration/2))
                          )
                        {
                                m_Library.trackPlayed(
                                    get<gint64>(m_Metadata.get()[ATTRIBUTE_MPX_TRACK_ID].get()),
                                    get<gint64>(m_Metadata.get()[ATTRIBUTE_MPX_ALBUM_ID].get()),
                                    time(NULL)
                                );

                                PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                                g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_TRACK_PLAYED], 0);
                                check_py_error();
                                pyg_gil_state_release(state);
                        }

                        m_TrackPlayedSeconds = 0.;
                }

        void
                Player::on_play_stream_switched ()
                {
                        g_return_if_fail(m_PreparingSource || m_ActiveSource);

                        g_atomic_int_set(&m_NewTrack, 1);

                        m_TrackPlayedSeconds = 0;
                        m_TrackSeekedSeconds = 0;

                        if(m_PreparingSource)
                        {
                                m_ActiveSource = m_PreparingSource.get();
                                m_PreparingSource.reset();
                        }

                        //del_caps(C_CAN_PAUSE);
                        RefPtr<ToggleAction>::cast_static (m_actions->get_action(ACTION_PAUSE))->set_active(false);

                        PlaybackSource* source = m_Sources[m_ActiveSource.get()];
                        m_Sidebar->setActiveId(m_ActiveSource.get());

                        source->send_metadata ();

                        switch( m_PlayDirection )
                        {
                                case PD_NONE:
                                        g_return_if_fail(0);
                                        break;

                                case PD_PREV:
                                        source->prev_post();
                                        break;

                                case PD_NEXT: 
                                        source->next_post();

                                case PD_PLAY:
                                        source->play_post();
                                        break;
                        };

                        source->send_caps ();

                        Mutex::Lock L (m_SourceCFLock);

                        Caps caps = m_source_c[m_ActiveSource.get()];
                        set_caps(C_CAN_SEEK, caps & C_CAN_SEEK);
                        set_caps(C_CAN_PAUSE, caps & C_CAN_PAUSE);

                        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                        g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_NEW_TRACK], 0);
                        check_py_error();
                        pyg_gil_state_release(state);
                }

        void
                Player::play ()
                {
                        ItemKey const& source_id = m_Sidebar->getVisibleId(); 

                        if( m_Sources.count(source_id) ) 
                        {
                                Caps c = m_source_c[source_id];

                                if( c & C_CAN_PLAY )
                                {
                                        if( m_ActiveSource && (m_Play.property_status().get_value() != PLAYSTATUS_STOPPED))
                                        {
                                                track_played ();
                                                m_Sources[m_ActiveSource.get()]->stop ();
                                        }

                                        if( m_PreparingSource )
                                        {
                                                m_Sources[m_PreparingSource.get()]->stop ();
                                        }

                                        PlaybackSource* source = m_Sources[source_id];
                                        m_PreparingSource = source_id;

                                        if( m_source_f[source_id] & F_ASYNC)
                                        {
                                                source->play_async ();
                                                return;
                                        }
                                        else
                                        {
                                                if( source->play() )
                                                {
                                                        m_PlayDirection = PD_PLAY;
                                                        switch_stream (source->get_uri(), source->get_type());
                                                        return;
                                                }
                                        }
                                }

                                stop ();
                        }
                }

        void
                Player::pause ()
                {
                        g_return_if_fail(m_ActiveSource);

                        Caps c = m_source_c[m_ActiveSource.get()];

                        if(c & C_CAN_PAUSE )
                        {
                                if( m_Play.property_status() == PLAYSTATUS_PAUSED)
                                {
                                        m_Play.request_status (PLAYSTATUS_PLAYING);
                                }
                                else
                                {
                                        m_Play.request_status (PLAYSTATUS_PAUSED);
                                }
                        }
                }

        void
                Player::prev ()
                {
                        ItemKey const& source_id = m_ActiveSource.get(); 

                        PlaybackSource*           source = m_Sources[source_id];
                        Flags          f = m_source_f[source_id];
                        Caps           c = m_source_c[source_id];

                        if( c & C_CAN_GO_PREV )
                        {
                                //del_caps(C_CAN_PAUSE);
                                track_played();

                                if( f & F_ASYNC )
                                {
                                        del_caps(C_CAN_GO_PREV);
                                        source->go_prev_async ();
                                        return;
                                }
                                else
                                        if( source->go_prev () )
                                        {
                                                prev_async_cb (source_id);
                                                return;
                                        }
                        }

                        stop ();
                }

        void
                Player::on_play_eos ()
                {
                        ItemKey const& source_id = m_ActiveSource.get(); 

                        PlaybackSource *source = m_Sources[source_id];
                        Flags           f      = m_source_f[source_id];
                        Caps            c      = m_source_c[source_id];

                        if( c & C_CAN_GO_NEXT )
                        {
                                //del_caps(C_CAN_PAUSE);
                                track_played();

                                if( f & F_ASYNC )
                                {
                                        del_caps(C_CAN_GO_NEXT);
                                        source->go_next_async ();
                                        return;
                                }
                                else
                                        if( source->go_next () )
                                        {
                                                next_async_cb (m_ActiveSource.get());
                                                return;
                                        }
                        }

                        stop ();
                }

        void
                Player::next ()
                {
                        g_return_if_fail(m_ActiveSource);
                        m_Sources[m_ActiveSource.get()]->skipped();
                        on_play_eos ();
                }

        void
                Player::stop ()
                {
                        //del_caps(C_CAN_PAUSE);
                        RefPtr<ToggleAction>::cast_static (m_actions->get_action(ACTION_PAUSE))->set_active(false);

                        if(m_PreparingSource)
                        {
                                m_Sources[m_PreparingSource.get()]->stop ();
                                m_Sources[m_PreparingSource.get()]->send_caps ();
                                m_Play.request_status( PLAYSTATUS_STOPPED );
                        }
                        else
                        if(m_ActiveSource)
                        {
                                track_played();
                                m_Sources[m_ActiveSource.get()]->stop ();
                                m_Sources[m_ActiveSource.get()]->send_caps ();
                                m_Play.request_status( PLAYSTATUS_STOPPED );
                        }
                }

        void
                Player::on_play_status_changed ()
                {
                        PlayStatus status = PlayStatus (m_Play.property_status().get_value());

                        switch( status )
                        {
                                case PLAYSTATUS_NONE:
                                        /* ... */
                                        break;

                                case PLAYSTATUS_SEEKING:
                                        /* ... */
                                        break;

                                case PLAYSTATUS_STOPPED:
                                        {
                                                Glib::Mutex::Lock L (m_MetadataLock);

                                                if( m_ActiveSource ) 
                                                {
                                                        m_Sources[m_ActiveSource.get()]->stop ();
                                                        m_Sources[m_ActiveSource.get()]->send_caps ();
                                                }

                                                if( m_PreparingSource ) 
                                                {
                                                        m_Sources[m_PreparingSource.get()]->stop ();
                                                }

                                                g_atomic_int_set(&m_Seeking, 0);
                                                m_Seek->set_range(0., 1.);
                                                m_Seek->set_value(0.);

                                                del_caps(Caps(C_CAN_SEEK | C_CAN_PAUSE | C_CAN_STOP | C_CAN_GO_NEXT | C_CAN_GO_PREV | C_CAN_PAUSE));

                                                m_VideoWidget->property_playing() = false; 
                                                m_VideoWidget->queue_draw();

                                                m_TimeLabel->set_text("      …      ");
                                                set_title (_("AudioSource: (Not Playing)"));

                                                m_InfoArea->reset();
                                                m_Metadata.reset();	
                                                m_ActiveSource.reset();
                                                m_PreparingSource.reset();
                                                m_Sidebar->clearActiveId();

                                                break;
                                        }

                                case PLAYSTATUS_WAITING:
                                        {
                                                g_atomic_int_set(&m_Seeking, 0);

                                                m_TimeLabel->set_text("      …      ");

                                                m_Seek->set_range(0., 1.);
                                                m_Seek->set_value(0.);
                                                m_Seek->set_sensitive(false);

                                                m_VideoWidget->property_playing() = false;
                                                m_VideoWidget->queue_draw();

                                                m_Metadata.reset();	

                                                break;
                                        }

                                case PLAYSTATUS_PLAYING:
                                        {
                                                set_caps(C_CAN_STOP);
                                                break;
                                        }

                                case PLAYSTATUS_PAUSED:
                                        {
                                                break;
                                        }
                        }

                        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                        g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_STATUS_CHANGED], 0, int(status));
                        check_py_error();
                        pyg_gil_state_release(state);
                }


        void
                Player::on_controls_play ()
                {
                        play ();
                }

        void
                Player::on_controls_pause ()
                {
                        pause ();
                }

        void
                Player::on_controls_next ()
                {
                        next ();
                }

        void
                Player::on_controls_prev ()
                {
                        prev ();
                }	

        bool
                Player::on_key_press_event (GdkEventKey* event)
                {
                        if( event->keyval == GDK_Escape )
                        {
                                iconify ();
                                return true;
                        }
                        return Widget::on_key_press_event (event);
                }

        void
                Player::on_import_album ()
                {
                        m_MB_ImportAlbum->run();
                }

        void
                Player::on_import_folder()
                {
                        boost::shared_ptr<DialogImportFolder> d = boost::shared_ptr<DialogImportFolder>(DialogImportFolder::create());

                        if(d->run() == 0) // Import
                        {
                                Glib::ustring uri; 
                                d->get_folder_infos(uri);
                                d->hide();
                                StrV v;
                                v.push_back(uri);
                                m_Library.initScan(v);
                        }
                }

        void
                Player::on_import_share()
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
                                m_MountOperation->signal_ask_password().connect(sigc::mem_fun(*this, &Player::ask_password_cb));
                                m_MountFile->mount_mountable(m_MountOperation, sigc::mem_fun(*this, &Player::mount_ready_callback));
                        }
                }

        void
                Player::ask_password_cb(
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
                Player::mount_ready_callback (Glib::RefPtr<Gio::AsyncResult>& res)
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
                        m_Library.initScan(v);
                }

        void
                Player::unmount_ready_callback( Glib::RefPtr<Gio::AsyncResult>& res )
                {
                }

        void
                Player::on_library_scan_start()
                {
                        m_Statusbar->pop();        
                        m_Statusbar->push(_("Library Scan Starting..."));
                }

        void
                Player::on_library_scan_run( gint64 n, bool deep )
                {
                        m_Statusbar->pop();
                        m_Statusbar->push((
                            boost::format(_("Library Scan: %1% %2%"))
                            % n
                            % (deep ? _("Files") : _("Folders"))
                        ).str());
                }

        void
                Player::on_library_scan_end( ScanSummary const& summary )
                {
                        time_t curtime = time(NULL);
                        struct tm ctm;
                        localtime_r(&curtime, &ctm);

                        char bdate[256];
                        strftime(bdate, 256, "%a %b %d %H:%M:%S %Z %Y", &ctm);

                        m_Statusbar->pop();        
                        m_Statusbar->push((boost::format(
                            _("Library Scan finished on %1% (%2% %3% scanned, %4% Files added, %5% Files up to date, %6% updated, %7% erroneous, see log)"))
                            % bdate 
                            % summary.FilesTotal
                            % (summary.DeepRescan ? _("Files") : _("Folders"))
                            % summary.FilesAdded
                            % summary.FilesUpToDate
                            % summary.FilesUpdated
                            % summary.FilesErroneous
                        ).str());

                        m_Library.execSQL((boost::format ("INSERT INTO meta (last_scan_date) VALUES (%lld)") % (gint64(time(NULL)))).str());
                }

        /*static*/ bool
                Player::window_focus_cb (GdkEventFocus *event)
                {
                        dbus_g_proxy_call (m_mmkeys_dbusproxy,
                                        "GrabMediaPlayerKeys", NULL,
                                        G_TYPE_STRING, "MPX",
                                        G_TYPE_UINT, 0,
                                        G_TYPE_INVALID, G_TYPE_INVALID);

                        return false;
                }

        ::Window
                Player::on_gst_set_window_id ()
                {
                        m_VideoWidget->property_playing() = true; //FIXME: This does not really belong here.
                        m_VideoWidget->queue_draw();
                        return m_VideoWidget->get_video_xid(); 
                }

        void
                Player::on_gst_set_window_geom (int width, int height, GValue const* par)
                {
                        m_VideoWidget->property_geometry() = Geometry( width, height );
                        m_VideoWidget->set_par( par );
                }

        void
                Player::metadata_reparse ()
                {
                        m_InfoArea->set_metadata(m_Metadata.get(), m_NewTrack );

                        if( m_NewTrack && m_Metadata.get().Image )
                        {
                            g_atomic_int_set(&m_NewTrack, 0);
                        }

                        if(m_Metadata.get()[ATTRIBUTE_TITLE] && m_Metadata.get()[ATTRIBUTE_ARTIST])
                        {
                                set_title(
                                                (boost::format ("AudioSource: %1% - %2%") 
                                                 % get<std::string>(m_Metadata.get()[ATTRIBUTE_ARTIST].get())
                                                 % get<std::string>(m_Metadata.get()[ATTRIBUTE_TITLE].get())
                                                ).str()
                                         );
                        }
                        else
                        {
                                set_title(_("AudioSource: [Untitled Track]"));
                        }

                        signal_idle().connect(
                                        sigc::mem_fun(
                                                *this,
                                                &Player::metadata_updated
                                                ));
                }

        void
                Player::on_got_cover (const Glib::ustring& mbid)
                {
                    g_message("%s: Got Cover: %s", G_STRLOC, mbid.c_str());
        
                    if( m_Metadata && m_Metadata.get()[ATTRIBUTE_MB_ALBUM_ID] && get<std::string>(m_Metadata.get()[ATTRIBUTE_MB_ALBUM_ID].get()) == mbid)
                    {
                        Glib::RefPtr<Gdk::Pixbuf> cover;
                        m_Covers.fetch(mbid, cover);
                        m_InfoArea->set_cover( cover->scale_simple(72,72, Gdk::INTERP_HYPER), m_NewTrack );
                        g_atomic_int_set(&m_NewTrack, 0);
                    }
                    else
                        g_message("%s: Not identical MBID", G_STRLOC);
                }

        bool
                Player::metadata_updated ()
                {
                        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                        g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_METADATA_PREPARE], 0);
                        check_py_error();
                        pyg_gil_state_release(state);

                        state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                        g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_METADATA_UPDATED], 0);
                        check_py_error();
                        pyg_gil_state_release(state);

                        return false;
                }

        void
                Player::deactivate_plugin(gint64 id)
                {
                        m_PluginManager->deactivate(id);
                }

        void
                Player::activate_plugin(gint64 id)
                {
                        m_PluginManager->activate(id);
                }

        void
                Player::info_set (const std::string& info)
                {
                        m_InfoArea->set_info(info);
                }

        void
                Player::info_clear ()
                {
                        m_InfoArea->clear_info();
                }

        void
                Player::check_py_error ()
                {
                }

        void
                Player::del_caps(Caps caps)
                {
                        m_Caps = Caps(m_Caps & ~caps);
                        translate_caps();
                }

        void
                Player::set_caps(Caps caps, bool argument)
                {
                        if( argument )
                                m_Caps = Caps(m_Caps | caps);
                        else
                                m_Caps = Caps(m_Caps & ~caps);

                        translate_caps();
                }

        void
                Player::translate_caps ()
                {
                        m_actions->get_action( ACTION_PLAY )->set_sensitive( m_Caps & C_CAN_PLAY );
                        m_actions->get_action( ACTION_PAUSE )->set_sensitive( m_Caps & C_CAN_PAUSE );
                        m_actions->get_action( ACTION_PREV )->set_sensitive( m_Caps & C_CAN_GO_PREV );
                        m_actions->get_action( ACTION_NEXT )->set_sensitive( m_Caps & C_CAN_GO_NEXT );
                        m_actions->get_action( ACTION_STOP )->set_sensitive( m_Caps & C_CAN_STOP );
                        m_Seek->set_sensitive( m_Caps & C_CAN_SEEK );
                }

        // XXX: Public API we need when we split off SourceController

        void
                Player::metadata_reparse_with_lock ()
                {
                        Glib::Mutex::Lock L (m_MetadataLock);

                        if( (m_Play.property_status() == PLAYSTATUS_STOPPED ) ||
                            (m_Play.property_status() == PLAYSTATUS_WAITING ))
                            return;

                        metadata_reparse ();
                }

        void
                Player::set_metadata (Metadata const& metadata, ItemKey const& source_id)
                {
                        g_return_if_fail(m_ActiveSource && m_ActiveSource.get() == source_id);

                        Glib::Mutex::Lock L (m_MetadataLock);

                        if( (m_Play.property_status() == PLAYSTATUS_STOPPED ) ||
                            (m_Play.property_status() == PLAYSTATUS_WAITING ))
                            return;
 
                        m_Metadata = metadata;

                        if( !m_Metadata.get().Image && m_Metadata.get()[ATTRIBUTE_MB_ALBUM_ID]) 
                        {
                                m_Covers.fetch(
                                                get<std::string>(m_Metadata.get()[ATTRIBUTE_MB_ALBUM_ID].get()),
                                                m_Metadata.get().Image
                                              );
                        }

                        if(!m_Metadata.get()[ATTRIBUTE_LOCATION])
                        {
                                m_Metadata.get()[ATTRIBUTE_LOCATION] = m_Play.property_stream().get_value();
                        }

                        metadata_reparse ();
                }

        void
                Player::push_message (std::string const& message)
                {
                        m_Statusbar->pop();
                        m_Statusbar->push(message);
                        while (gtk_events_pending())
                            gtk_main_iteration();
                }

        void
                Player::on_rescan_in_intervals_changed (MCS_CB_DEFAULT_SIGNATURE)
                {
                        m_rescan_timer.reset();
                }

        bool
                Player::on_rescan_timeout()
                {
                        if(!m_MLibManager->is_present() && mcs->key_get<bool>("library","rescan-in-intervals") && m_rescan_timer.elapsed() >= mcs->key_get<int>("library","rescan-interval") * 60)
                        {
                          m_MLibManager->rescan_all_volumes();
                          m_rescan_timer.reset();
                        }
                        return true;
                }

}
