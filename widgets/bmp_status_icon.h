/* gtkstatusicon.h:
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *      Mark McLoughlin <mark@skynet.ie>
 */

#ifndef BMP_STATUS_ICON_H
#define BMP_STATUS_ICON_H

#include <gtk/gtkimage.h>
#include <gtk/gtkmenu.h>

#include "bmp_tray_icon.h"

G_BEGIN_DECLS

#define BMP_TYPE_STATUS_ICON         (bmp_status_icon_get_type ())
#define BMP_STATUS_ICON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BMP_TYPE_STATUS_ICON, BmpStatusIcon))
#define BMP_STATUS_ICON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BMP_TYPE_STATUS_ICON, BmpStatusIconClass))
#define BMP_IS_STATUS_ICON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BMP_TYPE_STATUS_ICON))
#define BMP_IS_STATUS_ICON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BMP_TYPE_STATUS_ICON))
#define BMP_STATUS_ICON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BMP_TYPE_STATUS_ICON, BmpStatusIconClass))

typedef struct _BmpStatusIcon	     BmpStatusIcon;
typedef struct _BmpStatusIconClass   BmpStatusIconClass;
typedef struct _BmpStatusIconPrivate BmpStatusIconPrivate;

struct _BmpStatusIcon
{
  GObject               parent_instance;

  BmpStatusIconPrivate *priv;
};

struct _BmpStatusIconClass
{
  GObjectClass parent_class;

  void     (* scroll_up)    (BmpStatusIcon *status_icon);
  void     (* scroll_down)  (BmpStatusIcon *status_icon);
  void     (* click)        (BmpStatusIcon *status_icon);
  void     (* activate)     (BmpStatusIcon *status_icon);
  void     (* popup_menu)   (BmpStatusIcon *status_icon,
	                  		     guint          button,
                  			     guint32        activate_time);
  gboolean (* size_changed) (BmpStatusIcon *status_icon,
                  			     gint           size);

  void (*__gtk_reserved1);
  void (*__gtk_reserved2);
  void (*__gtk_reserved3);
  void (*__gtk_reserved4);
  void (*__gtk_reserved5);
  void (*__gtk_reserved6);  
};

GType                 bmp_status_icon_get_type           (void) G_GNUC_CONST;

BmpStatusIcon        *bmp_status_icon_new                (void);
BmpStatusIcon        *bmp_status_icon_new_from_pixbuf    (GdkPixbuf          *pixbuf);
BmpStatusIcon        *bmp_status_icon_new_from_file      (const gchar        *filename);
BmpStatusIcon        *bmp_status_icon_new_from_stock     (const gchar        *stock_id);
BmpStatusIcon        *bmp_status_icon_new_from_icon_name (const gchar        *icon_name);

void                  bmp_status_icon_set_from_pixbuf    (BmpStatusIcon      *status_icon,
                                                          GdkPixbuf          *pixbuf);
void                  bmp_status_icon_set_from_file      (BmpStatusIcon      *status_icon,
                                                          const gchar        *filename);
void                  bmp_status_icon_set_from_stock     (BmpStatusIcon      *status_icon,
                                                          const gchar        *stock_id);
void                  bmp_status_icon_set_from_icon_name (BmpStatusIcon      *status_icon,
                                                          const gchar        *icon_name);

GtkImageType          bmp_status_icon_get_storage_type   (BmpStatusIcon      *status_icon);

GdkPixbuf            *bmp_status_icon_get_pixbuf         (BmpStatusIcon      *status_icon);
G_CONST_RETURN gchar *bmp_status_icon_get_stock          (BmpStatusIcon      *status_icon);
G_CONST_RETURN gchar *bmp_status_icon_get_icon_name      (BmpStatusIcon      *status_icon);

gint                  bmp_status_icon_get_size           (BmpStatusIcon      *status_icon);

void                  bmp_status_icon_set_visible        (BmpStatusIcon      *status_icon,
                                                          gboolean            visible);
gboolean              bmp_status_icon_get_visible        (BmpStatusIcon      *status_icon);

void                  bmp_status_icon_set_blinking       (BmpStatusIcon      *status_icon,
                                                          gboolean            blinking);
gboolean              bmp_status_icon_get_blinking       (BmpStatusIcon      *status_icon);

gboolean              bmp_status_icon_is_embedded        (BmpStatusIcon      *status_icon);

void                  bmp_status_icon_position_menu      (GtkMenu            *menu,
                                                          gint               *x,
                                                          gint               *y,
                                                          gboolean           *push_in,
                                                          gpointer            user_data);

gboolean              bmp_status_icon_get_geometry       (BmpStatusIcon      *status_icon,
                                                          GdkScreen         **screen,
                                                          GdkRectangle       *area,
                                                          GtkOrientation     *orientation);

GtkWidget*            bmp_status_icon_get_tray_icon      (BmpStatusIcon      *status_icon);


G_END_DECLS

#endif /* __BMP_STATUS_ICON_H__ */
