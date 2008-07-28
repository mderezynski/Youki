//  MPX
//  Copyright (C) 2008 MPX 
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
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <glibmm/miscutils.h>
#include <gtk/gtk.h>

#include <boost/format.hpp>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

namespace
{
    GOptionEntry options[] =
    {
    };

    void
    clear_error(GError**error)
    {
        g_error_free(*error);
        *error = NULL;
    }

    void
    print_version ()
    {
        g_print ("%s %s",
                 PACKAGE,
                 VERSION);

        g_print (_("\nAudioSource Player Copyright (c) 2008 MPX Project <http://mpx.backtrace.info>\n\n"));
    }

    void
    print_configure ()
    {
        g_print ("%s\n",
                 CONFIGURE_ARGS);
    }

    void
    setup_i18n ()
    {
        setlocale (LC_ALL, "");
        bindtextdomain (PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (PACKAGE, "UTF-8");
        textdomain (PACKAGE);
    }

    void
    display_startup_crash ()
    {
        // We assume MPX started up, but crashed again
        if (!g_getenv ("DISPLAY"))
        {
            char *message = g_strconcat
              (_("\n       MPX seems to have crashed.\n\n       Please try starting it from a terminal using '"),
               PREFIX,
               _("/libexec/mpx-bin --no-log'\n       for further information on what could have caused the crash\n"),
               _("       and report it to our IRC channel, #mpx on irc.freenode.net\n\n"), NULL);
            g_printf (message);
            g_free (message);
        }
        else
        {
            gtk_init (0, NULL);
            GtkWidget *dialog = gtk_message_dialog_new_with_markup (NULL, GtkDialogFlags (0), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                      "MPX tried to start up, but crashed.\n\n"
                                      "Please file a bug at:\n\n"
                                      "http://mpx.backtrace.info/newticket");

            gtk_window_set_title (GTK_WINDOW (dialog), _("MPX Crashed"));
            gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
        }

        exit (EXIT_FAILURE);
    }

    void
    name_owner_changed (DBusGProxy* proxy,
                        char*       name,
                        char*       old_owner,
                        char*       new_owner,
                        gpointer    data)
    {
        if (!name || (name && std::strcmp (name, "info.backtrace.mpx")))
          return;

        if (std::strlen (old_owner) && !std::strlen (new_owner))
        {
            display_startup_crash ();
        }
    }

    void
    print_error (DBusError* error)
    {
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "DBus Error: (%s): %s", error->name, error->message);
    }

    void
    print_g_error (GError** error)
    {
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Error: %s", (*error)->message);
        clear_error(error);
    }
} // anonymous namespace

int
main (int    argc,
      char **argv)
{
    GError* error = NULL;

    setup_i18n ();
    g_type_init ();
    dbus_g_type_specialized_init ();

    GOptionContext* option_context = g_option_context_new (_(" - Run MPX and/or perform actions on MPX"));
    g_option_context_add_main_entries (option_context, options, PACKAGE);

    if (!g_option_context_parse (option_context, &argc, &argv, &error))
    {
        g_printerr (_("\nInvalid argument: '%s'\nPlease run '%s --help' for help on usage\n\n"), error->message, argv[0]);
        print_g_error(&error);
        return EXIT_FAILURE;
    }

    if ((argc > 1) && !strcmp(argv[1], "--version"))
    {
        print_version ();
        print_configure ();
        return EXIT_SUCCESS;
    }

    DBusGConnection* dbus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

    if (!dbus)
    {
        if(error)
        {
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "DBus Error: %s", error->message);
        }

        if (!g_getenv ("DISPLAY"))
        {
            g_printerr (_("%s: Couldn't connect to session bus: %s\n\n"), argv[0], error->message);
            return EXIT_FAILURE;
        }

        gtk_init (&argc, &argv);

        boost::format error_fmt (_("<big><b>MPX/DBus Error</b></big>\n\nMPX can not be started trough DBus activation.\n"
                                   "The following error occured trying to start up MPX:\n\n'<b>%s</b>'\n\n"
                                   "DBus might not be running at all or have even crashed, please consult further "
                                   "help with this problem from DBus or MPX"));

        std::string message;

        if (error)
        {
            message = (error_fmt % error->message).str ();
        }
        else
        {
            message = (error_fmt % _("(Unknown error. Perhaps DBus is not running at all?)")).str ();
        }

        GtkWidget *dialog = gtk_message_dialog_new_with_markup (NULL, GtkDialogFlags (0), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message.c_str ());

        if(error)
        {
            print_g_error(&error);
        }

        gtk_window_set_title (GTK_WINDOW (dialog), _("MPX/DBus Error - MPX"));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return EXIT_FAILURE;
    }

    DBusConnection* bus_conn = dbus_g_connection_get_connection (dbus);
    dbus_connection_setup_with_g_main (bus_conn, g_main_context_default());
    DBusGProxy* obj_bus = dbus_g_proxy_new_for_name (dbus, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus");

    guint request_name_result = 0;
    if (!dbus_g_proxy_call(

        obj_bus,
        "RequestName",
        &error,

        G_TYPE_STRING,
        "info.backtrace.startup",

        G_TYPE_UINT,
        0,

        G_TYPE_INVALID,

        G_TYPE_UINT,
        &request_name_result,

        G_TYPE_INVALID
    ))
    {
        g_critical ("%s: Failed RequestName request: %s", G_STRFUNC, error->message);
        print_g_error(&error);
        return EXIT_FAILURE;
    }

    switch (request_name_result)
    {
      case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
      {
          break;
      }

      case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
      case DBUS_REQUEST_NAME_REPLY_EXISTS:
      case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
      {
          g_object_unref (obj_bus);
          g_object_unref (bus_conn);
          return EXIT_SUCCESS;
      }
    }

    DBusGProxy* obj_mpx = dbus_g_proxy_new_for_name_owner(
        dbus,
        "info.backtrace.mpx",
        "/info/backtrace/mpx/Player",
        "info.backtrace.mpx",
        &error
    );

    if (!obj_mpx)
    {
        dbus_uint32_t reply;
        DBusError     dbus_error;

        dbus_error_init(&dbus_error);

        dbus_bus_start_service_by_name(
            dbus_g_connection_get_connection(dbus),
            "info.backtrace.mpx",
            0,
            &reply,
            &dbus_error                        
        );
    }
    else
    {
        g_object_unref(obj_mpx);
    }

    if (error)
    {
        print_g_error(&error);
        g_message("%s: Note: This is not an actual error, please ignore.", G_STRLOC);
    }

    obj_mpx = dbus_g_proxy_new_for_name(
        dbus,
        "info.backtrace.mpx",
        "/info/backtrace/mpx/Player",
        "info.backtrace.mpx"
    );

#if 0
    dbus_g_proxy_add_signal (obj_bus, "NameOwnerChanged", G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (obj_bus, "NameOwnerChanged", G_CALLBACK (name_owner_changed), NULL, NULL);
#endif

    dbus_g_proxy_call(

        obj_mpx,
        "Startup",
        &error,

        G_TYPE_INVALID,
        G_TYPE_INVALID

    );

    if(error)
    {
        print_g_error(&error);
        //display_startup_crash ();
    }

    if (argc > 1)
    {
        char** uri_list = g_new0 (char*, argc+1);

        for (int n = 1; n < argc; n++)
        {
            if(argv[n][0] == '/')
            {
                uri_list[n-1] = g_filename_to_uri(argv[n], NULL, NULL);
            }
            else
            if(strncmp(argv[0], "file://", 7))
            {
                char path[PATH_MAX];
                (void)getcwd(path, PATH_MAX);
                char * full_path = g_build_filename(path, argv[n], NULL);
                char * uri = g_filename_to_uri(full_path, NULL, NULL);
                g_free(full_path);
                uri_list[n-1] = uri;
            }
            else
            {
                uri_list[n-1] = g_strdup (argv[n]);
            }
        }

        if(! dbus_g_proxy_call(

            obj_mpx,
            "PlayTracks",
            &error,

            G_TYPE_STRV,
            uri_list,

            G_TYPE_INVALID,
            G_TYPE_INVALID

        ))
        {
            if (error)
            {
                print_g_error (&error);
            }

            goto abort;
        }

        g_strfreev (uri_list);
    }

    if (obj_mpx)
    {
        g_object_unref (obj_mpx);
    }

    if (obj_bus)
    {
        g_object_unref (obj_bus);
    }

    return EXIT_SUCCESS;

  abort:

    if (obj_mpx)
    {
        g_object_unref (obj_mpx);
    }

    if (obj_bus)
    {
        g_object_unref (obj_bus);
    }

    std::abort (); 
}
