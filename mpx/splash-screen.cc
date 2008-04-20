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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <glib/gi18n.h>
#include <gtkmm.h>
#include <gdkmm.h>
#include "splash-screen.hh"
using namespace Glib;

namespace MPX
{
    Splashscreen::Splashscreen ()
    : Gtk::Window (Gtk::WINDOW_TOPLEVEL)
    , m_image (Gdk::Pixbuf::create_from_file (build_filename(DATA_DIR, "images" G_DIR_SEPARATOR_S "splash.png")))
    , m_image_w (m_image->get_width())
    , m_image_h (m_image->get_height())
    , m_bar_w (m_image_w - 220)
    , m_bar_h (4)
    , m_bar_x (12)
    , m_bar_y (m_image_h - 30)
    , m_percent (0.0)
    {
        set_size_request (m_image_w, m_image_h);
        set_title (_("MPX Starting..."));
        set_skip_taskbar_hint (true);
        set_skip_pager_hint (true);
        set_keep_above (true);
        set_resizable (false);
        set_decorated (false);
        set_app_paintable (true);

        set_position (Gtk::WIN_POS_CENTER);
        set_type_hint (Gdk::WINDOW_TYPE_HINT_SPLASHSCREEN);

        Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
        m_has_alpha = screen->is_composited();

        if (m_has_alpha)
        {
          set_colormap (screen->get_rgba_colormap());
        }
        else
        {
          Glib::RefPtr<Gdk::Pixmap> mask_pixmap_window1, mask_pixmap_window2;
          Glib::RefPtr<Gdk::Bitmap> mask_bitmap_window1, mask_bitmap_window2;
          m_image->render_pixmap_and_mask (mask_pixmap_window1, mask_bitmap_window1, 0);
          m_image->render_pixmap_and_mask (mask_pixmap_window2, mask_bitmap_window2, 128);
          shape_combine_mask (mask_bitmap_window2, 0, 0);
        }

        show ();
    }

    void
    Splashscreen::set_message (char const * message, double percent)
    {
        m_percent = percent;
        m_message = message;
        queue_draw ();
        while (gtk_events_pending()) gtk_main_iteration();
    }

    void
    Splashscreen::on_realize ()
    {
        Gtk::Window::on_realize ();
        m_layout = create_pango_layout ("");
        while (gtk_events_pending()) gtk_main_iteration();
    }

    bool
    Splashscreen::on_expose_event (GdkEventExpose *event)
    {
        Cairo::RefPtr<Cairo::Context> m_cr = get_window ()->create_cairo_context ();

        m_cr->set_operator (Cairo::OPERATOR_SOURCE);
        if (m_has_alpha)
        {
            m_cr->set_source_rgba (.0, .0, .0, .0);
            m_cr->paint ();
        }

        gdk_cairo_set_source_pixbuf (m_cr->cobj(), m_image->gobj(), 0, 0);
        m_cr->paint ();

        m_cr->set_operator( Cairo::OPERATOR_ATOP );
        m_cr->set_source_rgba( 1., 1., 1., 1.); 
        m_cr->rectangle( m_bar_x - 2, m_bar_y - 2, m_bar_w + 4 , m_bar_h + 4);
        m_cr->set_line_width (0.5);
        m_cr->stroke ();

        m_cr->set_operator( Cairo::OPERATOR_ATOP );
        m_cr->set_source_rgba( 1., 1., 1., 1.); 
        m_cr->rectangle( m_bar_x , m_bar_y , m_bar_w * m_percent , m_bar_h );
        m_cr->fill ();

        int lw, lh;

        Pango::FontDescription desc = get_style()->get_font ();
        pango_font_description_set_absolute_size (desc.gobj(), 10 * PANGO_SCALE);
        pango_layout_set_font_description (m_layout->gobj(), desc.gobj ()); 

        m_layout->set_markup (m_message); 
        m_layout->get_pixel_size (lw, lh);

        m_cr->set_operator( Cairo::OPERATOR_ATOP );
        m_cr->move_to( m_bar_x - 2, m_bar_y - 17); 
        m_cr->set_source_rgba( 1., 1., 1., 1.);
        pango_cairo_show_layout (m_cr->cobj(), m_layout->gobj());

        desc = get_style()->get_font ();
        pango_font_description_set_absolute_size (desc.gobj(), 8 * PANGO_SCALE);
        pango_layout_set_font_description (m_layout->gobj(), desc.gobj ()); 

        m_layout->set_text (PACKAGE_VERSION); 
        m_layout->get_pixel_size (lw, lh);

        m_cr->set_operator( Cairo::OPERATOR_ATOP );
        m_cr->move_to( get_allocation().get_width()  - lw - 20, get_allocation().get_height() - lh - 8); 
        m_cr->set_source_rgba( 1., 1., 1., 1.);
        pango_cairo_show_layout (m_cr->cobj(), m_layout->gobj());

        return false;
    }
} // namespace MPX
