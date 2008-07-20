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
#include "mpx/widgets/cairoextensions.hh"

namespace
{
  gdouble
  mm_to_inch (gdouble mm)
  {
    return mm / 25.40;
  }

  inline guint8
  convert_color_channel (guint8 src,
                         guint8 alpha)
  {
    return alpha ? ((guint (src) << 8) - src) / alpha : 0;
  }

  void
  convert_bgra_to_rgba (guint8 const* src,
                        guint8*       dst,
                        int           width,
                        int           height)
  {
    guint8 const* src_pixel = src;
    guint8*       dst_pixel = dst;

    for (int y = 0; y < height; y++)
      for (int x = 0; x < width; x++)
      {
        dst_pixel[0] = convert_color_channel (src_pixel[2],
                                              src_pixel[3]);
        dst_pixel[1] = convert_color_channel (src_pixel[1],
                                              src_pixel[3]);
        dst_pixel[2] = convert_color_channel (src_pixel[0],
                                              src_pixel[3]);
        dst_pixel[3] = src_pixel[3];

        dst_pixel += 4;
        src_pixel += 4;
      }
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

    void color_to_hsb(
            Gdk::Color const& color,
            double & hue,
            double & saturation,
            double & brightness
    )
    {
        double min, max, delta;
        double red = color.get_red()/65535.;
        double green = color.get_green()/65535.;
        double blue = color.get_blue()/65535.;

        hue = 0;
        saturation = 0;
        brightness = 0;

        if(red > green) {
            max = fmax(red, blue);
            min = fmin(green, blue);
        } else {
            max = fmax(green, blue);
            min = fmin(red, blue);
        }

        brightness = (max + min) / 2;

        if(fabs(max - min) < 0.0001) {
            hue = 0;
            saturation = 0;
        } else {
            saturation = brightness <= 0.5
                ? (max - min) / (max + min)
                : (max - min) / (2 - max - min);

            delta = max - min;

            if(red == max) {
                hue = (green - blue) / delta;
            } else if(green == max) {
                hue = 2 + (blue - red) / delta;
            } else if(blue == max) {
                hue = 4 + (red - green) / delta;
            }

            hue *= 60;
            if(hue < 0) {
                hue += 360;
            }
        }
    }

    Gdk::Color
    color_from_hsb(double hue, double saturation, double brightness)
    {
        int i;
        double hue_shift[] = { 0, 0, 0 };
        double color_shift[] = { 0, 0, 0 };
        double m1, m2, m3;

        m2 = brightness <= 0.5
            ? brightness * (1 + saturation)
            : brightness + saturation - brightness * saturation;

        m1 = 2 * brightness - m2;

        hue_shift[0] = hue + 120;
        hue_shift[1] = hue;
        hue_shift[2] = hue - 120;

        color_shift[0] = color_shift[1] = color_shift[2] = brightness;

        i = saturation == 0 ? 3 : 0;

        for(; i < 3; i++) {
            m3 = hue_shift[i];

            if(m3 > 360) {
                m3 = fmod(m3, 360);
            } else if(m3 < 0) {
                m3 = 360 - fmod(fabs(m3), 360);
            }

            if(m3 < 60) {
                color_shift[i] = m1 + (m2 - m1) * m3 / 60;
            } else if(m3 < 180) {
                color_shift[i] = m2;
            } else if(m3 < 240) {
                color_shift[i] = m1 + (m2 - m1) * (240 - m3) / 60;
            } else {
                color_shift[i] = m1;
            }
        }

        Gdk::Color color;
        color.set_red(color_shift[0]*65535);
        color.set_green(color_shift[1]*65535);
        color.set_blue(color_shift[2]*65535);

        return color;
    }

    Gdk::Color
    color_shade(Gdk::Color const& base, double ratio)
    {
        double h, s, b;

        color_to_hsb(base, h, s, b);

        b = fmax(fmin(b * ratio, 1), 0);
        s = fmax(fmin(s * ratio, 1), 0);

        return color_from_hsb(h, s, b);
    }

    Gdk::Color
    color_adjust_brightness(Gdk::Color const& base, double br)
    {
        double h, s, b;
        color_to_hsb(base, h, s, b);
        b = fmax(fmin(br, 1), 0);
        return color_from_hsb(h, s, b);
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

      RoundedRectangle( cr, x, y, width, height, radius );
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
    cairo_image_surface_rounded_border (Cairo::RefPtr<Cairo::ImageSurface> & source, double width, double radius)
    {
      Cairo::RefPtr< ::Cairo::Context> cr = Cairo::Context::create (source);
      cairo_rounded_rect(cr, 0, 0, source->get_width(), source->get_height(), radius);
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

    Glib::RefPtr<Gdk::Pixbuf>
    cairo_image_surface_to_pixbuf (Cairo::RefPtr<Cairo::ImageSurface> image)
    {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, image->get_width(), image->get_height());
        convert_bgra_to_rgba(((guint8 const*)image->get_data()), pixbuf->get_pixels(), image->get_width(), image->get_height());
        return pixbuf;
    }

  } // namespace Util
} // namespace MPX
