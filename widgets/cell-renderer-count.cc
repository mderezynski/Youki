//  MPX
//  Copyright (C) 2007 beep-media-player.org 
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

#include <glibmm.h>
#include <gtkmm.h>

#include "cairoextensions.hh"
#include "cell-renderer-count.hh"

using namespace Gtk;

namespace MPX
{
    CellRendererCount::CellRendererCount ()
    : ObjectBase        (typeid(CellRendererCount))
    , property_text_    (*this, "text", "") 
    , property_box_     (*this, "box", false)
    , property_offset_  (*this, "offset", 0.)
    {
    }

    CellRendererCount::~CellRendererCount ()
    {
    }

    void
    CellRendererCount::get_size_vfunc  (Gtk::Widget & widget,
                                         const Gdk::Rectangle * cell_area,
                                         int *   x_offset,
                                         int *   y_offset,
                                         int *   width,
                                         int *   height) const 
    {
      if(x_offset)
        *x_offset = 0;

      if(y_offset)
        *y_offset = 0;

      if(width)
        *width = 52;

      if(height)
        *height = 64;
    }

    void
    CellRendererCount::render_vfunc    (Glib::RefPtr<Gdk::Drawable> const& window,
                                        Gtk::Widget                      & widget,
                                        Gdk::Rectangle              const& background_area,
                                        Gdk::Rectangle              const& cell_area,
                                        Gdk::Rectangle              const& expose_area,
                                        Gtk::CellRendererState             state)
    {
      ::Cairo::RefPtr< ::Cairo::Context> cr = window->create_cairo_context (); 

      int xoff, yoff;
      xoff = cell_area.get_x();
      yoff = cell_area.get_y();

      cr->set_operator (::Cairo::OPERATOR_SOURCE);
      RoundedRectangle (cr, xoff+8, yoff+6, 40, 16, 4);

      guint64 v = property_box().get_value();

      if(v != BOX_NOBOX) {

        if(! (state & Gtk::CELL_RENDERER_SELECTED))
          Gdk::Cairo::set_source_color(cr, widget.get_style()->get_text (Gtk::STATE_NORMAL));
        else
          Gdk::Cairo::set_source_color(cr, widget.get_style()->get_text (Gtk::STATE_SELECTED));

        cr->set_line_width (1.);

        if(property_box().get_value() == BOX_DASHED) {
          std::valarray<double> v;
          v.resize (1);
          v[0] = 4.;
          cr->set_dash (v, 0.);
        }

        cr->stroke ();

        if(property_box().get_value() != BOX_DASHED) {
          Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create (widget.get_pango_context());
          int text_width, text_height;
          layout->set_markup (property_text().get_value());
          layout->get_pixel_size(text_width, text_height);
          cr->move_to (xoff + 8 + (40 - text_width)/2, yoff+ 6 + (16 - text_height)/2);

          if(! (state & Gtk::CELL_RENDERER_SELECTED))
            Gdk::Cairo::set_source_color(cr, widget.get_style()->get_text (Gtk::STATE_NORMAL));
          else
            Gdk::Cairo::set_source_color(cr, widget.get_style()->get_text (Gtk::STATE_SELECTED));

          pango_cairo_show_layout (cr->cobj(), layout->gobj());
        }
      }
    }

    ///////////////////////////////////////////////
    /// Object Properties
    ///////////////////////////////////////////////

    ProxyOf<PropText>::ReadWrite
    CellRendererCount::property_text()
    {
      return ProxyOf<PropText>::ReadWrite (this, "text");
    }

    ProxyOf<PropInt>::ReadWrite
    CellRendererCount::property_box()
    {
      return ProxyOf<PropInt>::ReadWrite (this, "box");
    }

    ProxyOf<PropFloat>::ReadWrite
    CellRendererCount::property_offset()
    {
      return ProxyOf<PropFloat>::ReadWrite (this, "offset");
    }

};
