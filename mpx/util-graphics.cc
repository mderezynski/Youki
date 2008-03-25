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

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <gtkmm.h>
#include <glibmm/i18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include "mpx/util-graphics.hh"

namespace
{
  gdouble
  mm_to_inch (gdouble mm)
  {
    return mm / 25.40;
  }
}

namespace MPX
{
  namespace Util
  {
    void
    window_set_busy (GtkWindow* window)
    {
        GdkCursor *cursor = gdk_cursor_new_from_name (gdk_display_get_default (), "watch");

        if (!cursor)
          cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_WATCH);

        gdk_window_set_cursor (GTK_WIDGET (window)->window, cursor);
    }

    void
    window_set_idle (GtkWindow* window)
    {
        gdk_window_set_cursor (GTK_WIDGET (window)->window, NULL);
    }

    GtkWidget *
    ui_manager_get_popup (GtkUIManager * ui_manager,
                          char const*    path)
    {
      GtkWidget *menu_item;
      GtkWidget *submenu;

      menu_item = gtk_ui_manager_get_widget (ui_manager, path);

      if (GTK_IS_MENU_ITEM (menu_item))
          submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (menu_item));
      else
          submenu = NULL;

      return submenu;
    }

    void
    window_set_busy (Gtk::Window & window)
    {
        window.get_window()->set_cursor (Gdk::Cursor (Gdk::Display::get_default(), "watch"));
    }

    void
    window_set_idle (Gtk::Window & window)
    {
        window.get_window()->set_cursor ();
    }

    void
    color_to_rgba (Gdk::Color const& color,
                   double &          r,
                   double &          g,
                   double &          b,
                   double &          a)
    {
      r = color.get_red ()   / 65535.0;
      g = color.get_green () / 65535.0;
      b = color.get_blue ()  / 65535.0;
      a = 1.0;
    }

    double
    screen_get_resolution (Glib::RefPtr<Gdk::Screen> const& screen)
    {
      // NOTE: this assumes the width and height resolutions are
      // equal. This is only true for video modes with aspect ratios that
      // match the physical screen area ratio e.g. 1024x768 for 4:3
      // screens - descender

      return std::ceil (screen_get_y_resolution (screen));
    }

    double
    screen_get_x_resolution (Glib::RefPtr<Gdk::Screen> const& screen)
    {
      return static_cast<double> (screen->get_width ()) / mm_to_inch (screen->get_height_mm ());
    }

    double
    screen_get_y_resolution (Glib::RefPtr<Gdk::Screen> const& screen)
    {
      return static_cast<double> (screen->get_height ()) / mm_to_inch (screen->get_height_mm ());
    }

    Cairo::RefPtr<Cairo::ImageSurface>
    cairo_image_surface_from_pixbuf (Glib::RefPtr<Gdk::Pixbuf> pixbuf)
    {
      Cairo::RefPtr< ::Cairo::ImageSurface> surface =
        Cairo::ImageSurface::create (Cairo::FORMAT_ARGB32, pixbuf->get_width(), pixbuf->get_height());

      Cairo::RefPtr< ::Cairo::Context> cr =
        Cairo::Context::create (surface);

      cr->set_operator (Cairo::OPERATOR_SOURCE);
      Gdk::Cairo::set_source_pixbuf (cr, pixbuf, 0, 0);
      cr->rectangle (0, 0, pixbuf->get_width(), pixbuf->get_height());
      cr->fill ();
      return surface;
    }

    void
    cairo_rounded_rect (Cairo::RefPtr<Cairo::Context>& cr,
                        double                         x,
                        double                         y,
                        double                         width,
                        double                         height,
                        double                         radius)
    {
      g_return_if_fail (width > 0 && height > 0 && radius >= 0);

      double edge_length_x = width - radius * 2;
      double edge_length_y = height - radius * 2;

      g_return_if_fail (edge_length_x >= 0 && edge_length_y >= 0);

      cr->move_to (x + radius, y);
      cr->rel_line_to (edge_length_x, 0);
      cr->rel_curve_to (radius, 0, radius, radius, radius, radius);
      cr->rel_line_to (0, edge_length_y);
      cr->rel_curve_to (0, radius, -radius, radius, -radius, radius);
      cr->rel_line_to (-edge_length_x, 0);
      cr->rel_curve_to (-radius, 0, -radius, -radius, -radius, -radius);
      cr->rel_line_to (0, -edge_length_y);
      cr->rel_curve_to (0, -radius, radius, -radius, radius, -radius);
    }

    Cairo::RefPtr<Cairo::ImageSurface>
    cairo_image_surface_scale (Cairo::RefPtr<Cairo::ImageSurface> source, double width, double height)
    {
      Cairo::RefPtr< ::Cairo::ImageSurface> dest = Cairo::ImageSurface::create (source->get_format(), int (width), int (height));
      Cairo::RefPtr< ::Cairo::Context> cr = Cairo::Context::create (dest);

      cr->set_operator (Cairo::OPERATOR_SOURCE); 
      cr->scale (width / source->get_width(), height / source->get_height());
      cr->set_source (source, 0., 0.);
      cr->rectangle (0., 0., source->get_width(), source->get_height());
      cr->fill ();
      return dest;
    }

    Cairo::RefPtr<Cairo::ImageSurface>
    cairo_image_surface_round (Cairo::RefPtr<Cairo::ImageSurface> source, double radius)
    {
      Cairo::RefPtr< ::Cairo::ImageSurface> dest = Cairo::ImageSurface::create (source->get_format(), source->get_width(), source->get_height()); 
      Cairo::RefPtr< ::Cairo::Context> cr = Cairo::Context::create (dest);

      cr->set_operator (Cairo::OPERATOR_SOURCE); 
      cairo_rounded_rect (cr, 0, 0, source->get_width(), source->get_height(), radius);

      cr->set_source (source, 0., 0.);
      cr->fill_preserve ();

      cr->set_source_rgba (0., 0., 0., .5);
      cr->set_line_width (.7);
      cr->stroke ();

      return dest;
    }

    void 
    cairo_image_surface_border (Cairo::RefPtr<Cairo::ImageSurface> & source, double width)
    {
      Cairo::RefPtr< ::Cairo::Context> cr = Cairo::Context::create (source);
      cr->set_operator (Cairo::OPERATOR_SOURCE); 
      cr->rectangle (0, 0, source->get_width(), source->get_height());
      cr->set_source_rgba (0., 0., 0., 1.);
      cr->set_line_width (width);
      cr->stroke ();
    }

    void
    draw_cairo_image (Cairo::RefPtr<Cairo::Context> const&      cr,
                      Cairo::RefPtr<Cairo::ImageSurface> const& image,
                      double                                    x,
                      double                                    y,
                      double                                    alpha)
    {
        cr->save ();

        cr->translate (x, y);

        cr->set_operator (Cairo::OPERATOR_ATOP);
        cr->set_source (image, 0.0, 0.0);
        cr->rectangle (0.0, 0.0, image->get_width (), image->get_height ());
        cr->clip ();
        cr->paint_with_alpha (alpha);

        cr->restore ();
    }

  } // namespace Util
} // namespace MPX
