//  BMP
//  Copyright (C) 2005-2007 BMP development.
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
//  The BMPx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and BMPx. This
//  permission is above and beyond the permissions granted by the GPL license
//  BMPx is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <revision.h>
#include <build.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/select.h>

#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <boost/format.hpp>

#include <build.h>
#include <revision.h>

#include <locale.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

#include <bmp/dbus.hh>
#include <src/paths.hh>
#include <libsoup/soup.h>

#define SERVER_NAME "beep-media-player.org"

namespace
{
  GMainLoop*       mainloop = NULL;
  DBusGConnection* dbus     = NULL;
  DBusConnection*  c_bus    = NULL;
  DBusGProxy*      o_bus    = NULL;
  DBusGProxy*      o_bmp    = NULL;

  void
  send_crash ()
  {
    static boost::format uri_f ("http://beep-media-player.org/stats.php?version=%s&svnrev=%s&system=%s&action=%s");
    std::string uri = (uri_f % VERSION % RV_REVISION % BUILD_ARCH % "crash").str ();

    SoupSession * session = soup_session_sync_new ();
    SoupMessage * message = soup_message_new ("GET", uri.c_str());
  
    soup_session_send_message (session, message);
    g_object_unref (session);
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
  name_owner_changed (DBusGProxy* proxy,
                      char*       name,
                      char*       old_owner,
                      char*       new_owner,
                      gpointer    data)
  {
    if (!name || (name && std::strcmp (name, BMP_DBUS_SERVICE)))
      return;

    if (std::strlen (old_owner) && !std::strlen (new_owner))
    {
      std::exit (EXIT_SUCCESS);
    }
  }

  void
  on_bmp_shutdown_complete (DBusGProxy* proxy,
                            gpointer    data)
  {
    dbus_g_proxy_disconnect_signal (o_bus, "NameOwnerChanged", G_CALLBACK (name_owner_changed), NULL);
    g_main_loop_quit (mainloop);
  }
} 

int
main (int    argc,
      char** argv)
{
  setup_i18n ();

  g_type_init ();
  dbus_g_type_specialized_init ();

  mainloop = g_main_loop_new (NULL, FALSE);

  GError*   error = NULL;
  DBusError dbus_error;

  dbus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (error)
  {
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "DBus Error: %s", error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

  if (!dbus) // We've been activated by it but can't access it now -> Something's very wrong
  {
    return EXIT_FAILURE;
  }

  dbus_error_init (&dbus_error);

  c_bus = dbus_g_connection_get_connection (dbus);
  dbus_connection_setup_with_g_main (c_bus, g_main_context_default());

  o_bus = dbus_g_proxy_new_for_name (dbus, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus");

  unsigned int request_name_result = 0;
  if (!dbus_g_proxy_call (o_bus, "RequestName", &error,
                          G_TYPE_STRING, "org.beepmediaplayer.sentinel",
                          G_TYPE_UINT, 0,
                          G_TYPE_INVALID,
                          G_TYPE_UINT, &request_name_result,
                          G_TYPE_INVALID))
  {
    g_error ("%s: Failed RequestName request: %s", G_STRFUNC, error->message);
    g_error_free (error);
    error = NULL;
  }

  switch (request_name_result)
  {
    case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
      break;

    case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
    case DBUS_REQUEST_NAME_REPLY_EXISTS:
    case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
      g_object_unref (o_bus);
      g_object_unref (c_bus);
      return EXIT_SUCCESS;
  }

  dbus_g_proxy_add_signal (o_bus, "NameOwnerChanged", G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (o_bus, "NameOwnerChanged", G_CALLBACK (name_owner_changed), NULL, NULL);

  o_bmp = dbus_g_proxy_new_for_name_owner (dbus, BMP_DBUS_SERVICE, BMP_DBUS_PATH__BMP, BMP_DBUS_INTERFACE__BMP, &error);
  if (!o_bmp)
  {
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "BMP DBus Interface not present");
  }

  if (error)
  {
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "DBus Error: %s", error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

  dbus_g_proxy_add_signal (o_bmp, "ShutdownComplete", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (o_bmp, "ShutdownComplete", G_CALLBACK (on_bmp_shutdown_complete), NULL, NULL);
  g_main_loop_run (mainloop);
  g_main_loop_unref (mainloop);

  return EXIT_SUCCESS;
}
