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
#ifndef MPX_UTIL_GRAPHICS_HH
#define MPX_UTIL_GRAPHICS_HH
#include "config.h"
#include <string>
#include <glibmm/ustring.h>
#include <gtk/gtk.h>
#include <cairomm/context.h>
#include <gdkmm/color.h>
#include <gdkmm/colormap.h>
#include <gdkmm/drawable.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/screen.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/widget.h>
#include <gtkmm/window.h>

namespace MPX
{
  namespace Util
  {
    Gtk::Widget*
    get_popup (Glib::RefPtr<Gtk::UIManager> ui_manager, Glib::ustring const& menupath);

    void
    window_set_busy      (GtkWindow* window);

    void
    window_set_idle      (GtkWindow* window);

    void
    window_set_busy      (Gtk::Window & window);

    void
    window_set_idle      (Gtk::Window & window);

    double
    screen_get_resolution   (Glib::RefPtr<Gdk::Screen> const& screen);

    double
    screen_get_x_resolution (Glib::RefPtr<Gdk::Screen> const& screen);

    double
    screen_get_y_resolution (Glib::RefPtr<Gdk::Screen> const& screen);

    void
    color_to_rgba (Gdk::Color const& color,
                   double&           r,
                   double&           g,
                   double&           b,
                   double&           a);

    Cairo::RefPtr<Cairo::ImageSurface>
    cairo_image_surface_from_pixbuf (Glib::RefPtr<Gdk::Pixbuf>);

    Glib::RefPtr<Gdk::Pixbuf>
    cairo_image_surface_to_pixbuf (Cairo::RefPtr<Cairo::ImageSurface>);

    void
    cairo_rounded_rect (Cairo::RefPtr<Cairo::Context>& cr,
                        double                         x,
                        double                         y,
                        double                         width,
                        double                         height,
                        double                         radius);

    Cairo::RefPtr<Cairo::ImageSurface>
    cairo_image_surface_scale (Cairo::RefPtr<Cairo::ImageSurface> source, double width, double height);

    Cairo::RefPtr<Cairo::ImageSurface>
    cairo_image_surface_round (Cairo::RefPtr<Cairo::ImageSurface> source, double radius);

    void 
    cairo_image_surface_border (Cairo::RefPtr<Cairo::ImageSurface> & source, double width);

    void 
    cairo_image_surface_rounded_border (Cairo::RefPtr<Cairo::ImageSurface> & source, double width, double radius);

    void
    draw_cairo_image (Cairo::RefPtr<Cairo::Context> const&      cr,
                      Cairo::RefPtr<Cairo::ImageSurface> const& image,
                      double                                    x,
                      double                                    y,
                      double                                    alpha);
  } // Util
} // MPX

#endif // MPX_UTIL_GRAPHICS_HH
