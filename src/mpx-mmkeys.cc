//
//  mpx-mmkeys.cc
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

#include "config.h"
#include "mpx.hh"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glibmm/i18n.h>
#include <gdk/gdkx.h>
#include <gdl/gdl.h>
#include <boost/algorithm/string.hpp>

#include "mpx/mpx-preferences.hh"

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>

using namespace Glib;
using namespace Gtk;
using namespace std;
using namespace MPX::Util;
using boost::get;
using boost::algorithm::is_any_of;
using namespace boost::python;

namespace MPX
{
    /* Taken from xbindkeys */
    void
        Player::mmkeys_get_offending_modifiers ()
    {
        Display * dpy = gdk_x11_display_get_xdisplay (gdk_display_get_default());
        int i;
        XModifierKeymap *modmap;
        KeyCode nlock, slock;
        static int mask_table[8] =
        {
            ShiftMask, LockMask, ControlMask, Mod1Mask,
            Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
        };

        nlock = XKeysymToKeycode (dpy, XK_Num_Lock);
        slock = XKeysymToKeycode (dpy, XK_Scroll_Lock);

        /*
         * Find out the masks for the NumLock and ScrollLock modifiers,
         * so that we can bind the grabs for when they are enabled too.
         */
        modmap = XGetModifierMapping (dpy);

        if (modmap != NULL && modmap->max_keypermod > 0)
        {
            for (i = 0; i < 8 * modmap->max_keypermod; i++)
            {
                if (modmap->modifiermap[i] == nlock && nlock != 0)
                    m_numlock_mask = mask_table[i / modmap->max_keypermod];
                else if (modmap->modifiermap[i] == slock && slock != 0)
                    m_scrolllock_mask = mask_table[i / modmap->max_keypermod];
            }
        }

        m_capslock_mask = LockMask;

        if (modmap)
            XFreeModifiermap (modmap);
    }

    /*static*/ void
        Player::grab_mmkey(
            int        key_code,
            GdkWindow *root
        )
    {
        gdk_error_trap_push ();

        XGrabKey (GDK_DISPLAY (), key_code,
            0,
            GDK_WINDOW_XID (root), True,
            GrabModeAsync, GrabModeAsync);
        XGrabKey (GDK_DISPLAY (), key_code,
            Mod2Mask,
            GDK_WINDOW_XID (root), True,
            GrabModeAsync, GrabModeAsync);
        XGrabKey (GDK_DISPLAY (), key_code,
            Mod5Mask,
            GDK_WINDOW_XID (root), True,
            GrabModeAsync, GrabModeAsync);
        XGrabKey (GDK_DISPLAY (), key_code,
            LockMask,
            GDK_WINDOW_XID (root), True,
            GrabModeAsync, GrabModeAsync);
        XGrabKey (GDK_DISPLAY (), key_code,
            Mod2Mask | Mod5Mask,
            GDK_WINDOW_XID (root), True,
            GrabModeAsync, GrabModeAsync);
        XGrabKey (GDK_DISPLAY (), key_code,
            Mod2Mask | LockMask,
            GDK_WINDOW_XID (root), True,
            GrabModeAsync, GrabModeAsync);
        XGrabKey (GDK_DISPLAY (), key_code,
            Mod5Mask | LockMask,
            GDK_WINDOW_XID (root), True,
            GrabModeAsync, GrabModeAsync);
        XGrabKey (GDK_DISPLAY (), key_code,
            Mod2Mask | Mod5Mask | LockMask,
            GDK_WINDOW_XID (root), True,
            GrabModeAsync, GrabModeAsync);

        gdk_flush ();

        if (gdk_error_trap_pop ())
        {
            g_message(G_STRLOC ": Error grabbing key");
        }
    }

    /*static*/ void
        Player::grab_mmkey(
            int        key_code,
            int        modifier,
            GdkWindow *root
        )
    {
        gdk_error_trap_push ();

        modifier &= ~(m_numlock_mask | m_capslock_mask | m_scrolllock_mask);

        XGrabKey (GDK_DISPLAY (), key_code, modifier, GDK_WINDOW_XID (root),
            False, GrabModeAsync, GrabModeAsync);

        if (modifier == AnyModifier)
            return;

        if (m_numlock_mask)
            XGrabKey (GDK_DISPLAY (), key_code, modifier | m_numlock_mask,
                GDK_WINDOW_XID (root),
                False, GrabModeAsync, GrabModeAsync);

        if (m_capslock_mask)
            XGrabKey (GDK_DISPLAY (), key_code, modifier | m_capslock_mask,
                GDK_WINDOW_XID (root),
                False, GrabModeAsync, GrabModeAsync);

        if (m_scrolllock_mask)
            XGrabKey (GDK_DISPLAY (), key_code, modifier | m_scrolllock_mask,
                GDK_WINDOW_XID (root),
                False, GrabModeAsync, GrabModeAsync);

        if (m_numlock_mask && m_capslock_mask)
            XGrabKey (GDK_DISPLAY (), key_code, modifier | m_numlock_mask | m_capslock_mask,
                GDK_WINDOW_XID (root),
                False, GrabModeAsync, GrabModeAsync);

        if (m_numlock_mask && m_scrolllock_mask)
            XGrabKey (GDK_DISPLAY (), key_code, modifier | m_numlock_mask | m_scrolllock_mask,
                GDK_WINDOW_XID (root),
                False, GrabModeAsync, GrabModeAsync);

        if (m_capslock_mask && m_scrolllock_mask)
            XGrabKey (GDK_DISPLAY (), key_code, modifier | m_capslock_mask | m_scrolllock_mask,
                GDK_WINDOW_XID (root),
                False, GrabModeAsync, GrabModeAsync);

        if (m_numlock_mask && m_capslock_mask && m_scrolllock_mask)
            XGrabKey (GDK_DISPLAY (), key_code,
                modifier | m_numlock_mask | m_capslock_mask | m_scrolllock_mask,
                GDK_WINDOW_XID (root), False, GrabModeAsync,
                GrabModeAsync);

        gdk_flush ();

        if (gdk_error_trap_pop ())
        {
            g_message(G_STRLOC ": Error grabbing key");
        }
    }

    /*static*/ void
        Player::ungrab_mmkeys (GdkWindow *root)
    {
        gdk_error_trap_push ();

        XUngrabKey (GDK_DISPLAY (), AnyKey, AnyModifier, GDK_WINDOW_XID (root));

        gdk_flush ();

        if (gdk_error_trap_pop ())
        {
            g_message(G_STRLOC ": Error grabbing key");
        }
    }

    /*static*/ GdkFilterReturn
        Player::filter_mmkeys(
            GdkXEvent   *xevent,
            GdkEvent    *event,
            gpointer     data
        )
    {
        Player & player = *reinterpret_cast<Player*>(data);

        if( !player.m_ActiveSource )
            return GDK_FILTER_CONTINUE;

        Caps c = player.m_source_c[player.m_ActiveSource.get()];

        XEvent * xev = (XEvent *) xevent;

        if (xev->type != KeyPress)
        {
            return GDK_FILTER_CONTINUE;
        }

        XKeyEvent * key = (XKeyEvent *) xevent;

        guint keycodes[] = {0, 0, 0, 0, 0};
        int sys = mcs->key_get<int>("hotkeys","system");

        if( sys == 0)
        {
            keycodes[0] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPlay);
            keycodes[1] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPause);
            keycodes[2] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev);
            keycodes[3] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext);
            keycodes[4] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop);
        }
        else
        {
            keycodes[0] = mcs->key_get<int>("hotkeys","key-1");
            keycodes[1] = mcs->key_get<int>("hotkeys","key-2");
            keycodes[2] = mcs->key_get<int>("hotkeys","key-3");
            keycodes[3] = mcs->key_get<int>("hotkeys","key-4");
            keycodes[4] = mcs->key_get<int>("hotkeys","key-5");
        }

        if (keycodes[0] == key->keycode)
        {
            (dynamic_cast<Gtk::Button*>(player.m_Xml->get_widget("controls-play")))->clicked();
            //player.play ();
            return GDK_FILTER_REMOVE;
        }
        else if (keycodes[1] == key->keycode)
        {
            if( c & C_CAN_PAUSE )
                (dynamic_cast<Gtk::ToggleButton*>(player.m_Xml->get_widget("controls-pause")))->set_active(!(dynamic_cast<Gtk::ToggleButton*>(player.m_Xml->get_widget("controls-pause")))->get_active());
            //player.pause ();
            return GDK_FILTER_REMOVE;
        }
        else if (keycodes[2] == key->keycode)
        {
            if( c & C_CAN_GO_PREV )
                (dynamic_cast<Gtk::Button*>(player.m_Xml->get_widget("controls-previous")))->clicked();
            //player.prev ();
            return GDK_FILTER_REMOVE;
        }
        else if (keycodes[3] == key->keycode)
        {
            if( c & C_CAN_GO_NEXT )
                (dynamic_cast<Gtk::Button*>(player.m_Xml->get_widget("controls-next")))->clicked();
            //player.next ();
            return GDK_FILTER_REMOVE;
        }
        else if (keycodes[4] == key->keycode)
        {
            (dynamic_cast<Gtk::Button*>(player.m_Xml->get_widget("controls-stop")))->clicked();
            //player.stop ();
            return GDK_FILTER_REMOVE;
        }
        else
        {
            return GDK_FILTER_CONTINUE;
        }
    }

    /*static*/ void
        Player::mmkeys_grab (bool grab)
    {
        gint keycodes[] = {0, 0, 0, 0, 0};
        keycodes[0] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPlay);
        keycodes[1] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop);
        keycodes[2] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev);
        keycodes[3] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext);
        keycodes[4] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPause);

        GdkDisplay *display;
        GdkScreen *screen;
        GdkWindow *root;

        display = gdk_display_get_default ();
        int sys = mcs->key_get<int>("hotkeys","system");

        for (int i = 0; i < gdk_display_get_n_screens (display); i++)
        {
            screen = gdk_display_get_screen (display, i);

            if (screen != NULL)
            {
                root = gdk_screen_get_root_window (screen);
                if(!grab)
                {
                    ungrab_mmkeys (root);
                    continue;
                }

                for (guint j = 1; j < 6 ; ++j)
                {
                    if( sys == 2 )
                    {
                        int key = mcs->key_get<int>("hotkeys", (boost::format ("key-%d") % j).str());
                        int mask = mcs->key_get<int>("hotkeys", (boost::format ("key-%d-mask") % j).str());

                        if (key)
                        {
                            grab_mmkey (key, mask, root);
                        }
                    }
                    else
                    if( sys == 0 )
                    {
                        grab_mmkey (keycodes[j-1], root);
                    }
                }

                if (grab)
                    gdk_window_add_filter (root, filter_mmkeys, this);
                else
                    gdk_window_remove_filter (root, filter_mmkeys, this);
            }
        }
    }

    /*static*/ void
        Player::mmkeys_activate ()
    {
        DBusGConnection *bus;

        g_message(G_STRLOC ": Activating media player keys");

        m_mmkeys_dbusproxy = 0;

        if( m_mmkeys_grab_type == SETTINGS_DAEMON )
        {
            bus = dbus_g_bus_get( DBUS_BUS_SESSION, NULL ) ;

            if( bus )
            {
                GError *error = NULL;

                m_mmkeys_dbusproxy = dbus_g_proxy_new_for_name( bus,
                    "org.gnome.SettingsDaemon",
                    "/org/gnome/SettingsDaemon/MediaKeys",
                    "org.gnome.SettingsDaemon.MediaKeys");

                if( !m_mmkeys_dbusproxy )
                {
                    m_mmkeys_dbusproxy = dbus_g_proxy_new_for_name( bus,
                        "org.gnome.SettingsDaemon",
                        "/org/gnome/SettingsDaemon",
                        "org.gnome.SettingsDaemon");
                }

                if( m_mmkeys_dbusproxy )
                {
                    dbus_g_proxy_call( m_mmkeys_dbusproxy,
                        "GrabMediaPlayerKeys", &error,
                        G_TYPE_STRING, "MPX",
                        G_TYPE_UINT, 0,
                        G_TYPE_INVALID,
                        G_TYPE_INVALID);

                    if( error == NULL )
                    {
                        g_message(G_STRLOC ": created dbus proxy for org.gnome.SettingsDaemon; grabbing keys");

                        dbus_g_object_register_marshaller (mpx_dbus_marshal_VOID__STRING_STRING,
                            G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

                        dbus_g_proxy_add_signal (m_mmkeys_dbusproxy,
                            "MediaPlayerKeyPressed",
                            G_TYPE_STRING,G_TYPE_STRING,G_TYPE_INVALID);

                        dbus_g_proxy_connect_signal (m_mmkeys_dbusproxy,
                            "MediaPlayerKeyPressed",
                            G_CALLBACK (media_player_key_pressed),
                            this, NULL);

                        mWindowFocusConn = signal_focus_in_event().connect( sigc::mem_fun( *this, &Player::window_focus_cb ) );
                    }
                    else if (error->domain == DBUS_GERROR &&
                        (error->code != DBUS_GERROR_NAME_HAS_NO_OWNER ||
                        error->code != DBUS_GERROR_SERVICE_UNKNOWN))
                    {
                        /* settings daemon dbus service doesn't exist.
                         * just silently fail.
                         */
                        g_message(G_STRLOC ": org.gnome.SettingsDaemon dbus service not found: %s", error->message);
                        g_error_free (error);
                        g_object_unref(m_mmkeys_dbusproxy);
                        m_mmkeys_dbusproxy = 0;
                    }
                    else
                    {
                        g_warning (G_STRLOC ": Unable to grab media player keys: %s", error->message);
                        g_error_free (error);
                        g_object_unref(m_mmkeys_dbusproxy);
                        m_mmkeys_dbusproxy = 0;
                    }
                }

                dbus_g_connection_unref( bus );
            }
            else
            {
                g_message(G_STRLOC ": couldn't get dbus session bus");
            }
        }
        else if( m_mmkeys_grab_type == X_KEY_GRAB )
        {
            g_message(G_STRLOC ": attempting old-style key grabs");
            mmkeys_grab (true);
        }
    }

    /*static*/ void
        Player::mmkeys_deactivate ()
    {
        if( m_mmkeys_dbusproxy )
        {
            if( m_mmkeys_grab_type == SETTINGS_DAEMON )
            {
                GError *error = NULL;

                dbus_g_proxy_call (m_mmkeys_dbusproxy,
                    "ReleaseMediaPlayerKeys", &error,
                    G_TYPE_STRING, "MPX",
                    G_TYPE_INVALID, G_TYPE_INVALID);

                if (error != NULL)
                {
                    g_warning (G_STRLOC ": Could not release media player keys: %s", error->message);
                    g_error_free (error);
                }

                mWindowFocusConn.disconnect ();
                m_mmkeys_grab_type = NONE;
            }

            g_object_unref( m_mmkeys_dbusproxy ) ;
            m_mmkeys_dbusproxy = 0;
        }

        if (m_mmkeys_grab_type == X_KEY_GRAB)
        {
            g_message(G_STRLOC ": undoing old-style key grabs");
            mmkeys_grab( false ) ;
            m_mmkeys_grab_type = NONE;
        }
    }

    void
        Player::on_mm_edit_begin ()
    {
        mmkeys_deactivate ();
    }

    void
        Player::on_mm_edit_done ()
    {
        if( mcs->key_get<bool>("hotkeys","enable") )
        {
            int sys = mcs->key_get<int>("hotkeys","system");

            if( (sys == 0) || (sys == 2))
                m_mmkeys_grab_type = X_KEY_GRAB;
            else
                m_mmkeys_grab_type = SETTINGS_DAEMON;

            mmkeys_activate ();
        }
    }

    // MMkeys code (C) Rhythmbox Developers 2007

    /*static*/ void
        Player::media_player_key_pressed(
            DBusGProxy  *proxy,
            const gchar *application,
            const gchar *key,
            gpointer     data
        )
    {
        Player & player = *reinterpret_cast<Player*>(data);

        if( strcmp (application, "MPX"))
            return;

        if( !player.m_ActiveSource )
            return;

        Caps c = player.m_source_c[player.m_ActiveSource.get()];

        if (strcmp (key, "Play") == 0)
        {
            if( player.m_Play->property_status() == PLAYSTATUS_PAUSED)
                player.pause ();
            else
            if( player.m_Play->property_status() != PLAYSTATUS_WAITING)
                player.play ();
        }
        else if (strcmp (key, "Pause") == 0)
        {
            if( c & C_CAN_PAUSE )
                player.pause ();
        }
        else if (strcmp (key, "Stop") == 0)
        {
            player.stop ();
        }
        else if (strcmp (key, "Previous") == 0)
        {
            if( c & C_CAN_GO_PREV )
                player.prev ();
        }
        else if (strcmp (key, "Next") == 0)
        {
            if( c & C_CAN_GO_NEXT )
                player.next ();
        }
    }
}
