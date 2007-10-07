/* gtktrayicon.h
 * Copyright (C) 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BMP_TRAY_ICON_H
#define BMP_TRAY_ICON_H

#include <gtk/gtkplug.h>

G_BEGIN_DECLS

#define BMP_TYPE_TRAY_ICON		(bmp_tray_icon_get_type ())
#define BMP_TRAY_ICON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BMP_TYPE_TRAY_ICON, BmpTrayIcon))
#define BMP_TRAY_ICON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BMP_TYPE_TRAY_ICON, BmpTrayIconClass))
#define BMP_IS_TRAY_ICON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BMP_TYPE_TRAY_ICON))
#define BMP_IS_TRAY_ICON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BMP_TYPE_TRAY_ICON))
#define BMP_TRAY_ICON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BMP_TYPE_TRAY_ICON, BmpTrayIconClass))
	
typedef struct _BmpTrayIcon	   BmpTrayIcon;
typedef struct _BmpTrayIconPrivate BmpTrayIconPrivate;
typedef struct _BmpTrayIconClass   BmpTrayIconClass;

struct _BmpTrayIcon
{
  GtkPlug parent_instance;

  BmpTrayIconPrivate *priv;
};

struct _BmpTrayIconClass
{
  GtkPlugClass parent_class;

  void (*destroyed) (BmpTrayIcon *tray);

  void (*__gtk_reserved1);
  void (*__gtk_reserved2);
  void (*__gtk_reserved3);
  void (*__gtk_reserved4);
  void (*__gtk_reserved5);
  void (*__gtk_reserved6);
};

GType          bmp_tray_icon_get_type         (void) G_GNUC_CONST;

BmpTrayIcon   *_bmp_tray_icon_new_for_screen  (GdkScreen   *screen,
					       const gchar *name);

BmpTrayIcon   *_bmp_tray_icon_new             (const gchar *name);

guint          _bmp_tray_icon_send_message    (BmpTrayIcon *icon,
					       gint         timeout,
					       const gchar *message,
					       gint         len);
void           _bmp_tray_icon_cancel_message  (BmpTrayIcon *icon,
					       guint        id);

GtkOrientation _bmp_tray_icon_get_orientation (BmpTrayIcon *icon);
					    
G_END_DECLS

#endif /* __BMP_TRAY_ICON_H__ */
