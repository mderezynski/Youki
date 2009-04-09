//  MPX
//  Copyright (C) 2005-2007 MPX development.
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
//  The MPX project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include "kobo-titleinfo.hh"

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdkmm/general.h>
#include <cairomm/cairomm.h>
#include <cmath>

#include "mpx/util-graphics.hh"
#include "mpx/widgets/cairo-extensions.hh"

#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-main.hh"

namespace
{
    int const mask_alpha_threshold = 128;

    // Animation settings

    int const animation_fps = 24;
    int const animation_frame_period_ms = 1000 / animation_fps;

    char const*  text_font              = "Sans" ;
    int const    text_size_px           = 14 ;
    double const text_fade_in_time      = 0.2 ;
    double const text_fade_out_time     = 0.05 ;
    double const text_hold_time         = 5. ; 
    double const text_time              = text_fade_in_time + text_fade_out_time + text_hold_time;
    double const text_full_alpha        = 0.90 ;
    double const initial_delay          = 0.0 ;

}

namespace MPX
{
    inline double
    KoboTitleInfo::cos_smooth (double x)
    {
        return (1.0 - std::cos (x * G_PI)) / 2.0;
    }

    std::string
    KoboTitleInfo::get_text_at_time (double time)
    {
        if( !m_info.empty() )
        {
            unsigned int line = std::fmod( ( time / text_time ), m_info.size() ) ;
            return m_info[line];
        }
        else
        {
            return "";
        }
    }

    double
    KoboTitleInfo::get_text_alpha_at_time (double time)
    {
        {
            double offset = std::fmod( time, text_time ) ;

            if (offset < text_fade_in_time)
            {
                return text_full_alpha * cos_smooth (offset / text_fade_in_time);
            }
            else if (offset < text_fade_in_time + text_hold_time)
            {
                return text_full_alpha;
            }
            else
            {
                return text_full_alpha * cos_smooth (1.0 - (offset - text_fade_in_time - text_hold_time) / text_fade_out_time);
            }
        }
    }

    KoboTitleInfo::KoboTitleInfo ()
    {
        set_app_paintable (true);
        add_events( Gdk::BUTTON_PRESS_MASK ) ;

        set_colormap (Gdk::Screen::get_default()->get_rgba_colormap());

        m_timer.stop ();
        m_timer.reset ();

        set_size_request( -1, 24 ) ;
    }

    void
    KoboTitleInfo::clear ()
    {
        m_info.clear() ;
        m_timer.stop () ;
        m_timer.reset () ;
        m_update_connection.disconnect ();
        queue_draw () ;
    }

    void
    KoboTitleInfo::set_info(
        const std::vector<std::string>& i
    )
    {
        m_info = i ;

        total_animation_time    = m_info.size() * text_time;
        start_time              = initial_delay;
        end_time                = start_time + total_animation_time;

        m_timer.reset () ;

        if( !m_update_connection )
        {
          m_timer.start ();
          m_update_connection = Glib::signal_timeout ().connect (sigc::mem_fun (this, &KoboTitleInfo::update_frame),
                                                                 animation_frame_period_ms);
        }
    }

    bool
    KoboTitleInfo::on_expose_event (GdkEventExpose *event)
    {
        draw_frame ();
        return false;
    }

    void
    KoboTitleInfo::draw_frame ()
    {
        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

        Cairo::RefPtr<Cairo::Context> cairo = get_window ()->create_cairo_context () ;
        const Gtk::Allocation& a = get_allocation() ;

        const ThemeColor& c_base /* :) */ = theme->get_color( THEME_COLOR_BASE ) ; 
        cairo->set_operator(Cairo::OPERATOR_SOURCE) ;
        cairo->set_source_rgba(
              c_base.r
            , c_base.g
            , c_base.b
            , c_base.a
        ) ;
        cairo->paint () ;

        const ThemeColor& c_info = theme->get_color( THEME_COLOR_INFO_AREA ) ; 
        cairo->set_operator( Cairo::OPERATOR_ATOP ) ;
        cairo->set_source_rgba(
              c_info.r 
            , c_info.g
            , c_info.b
            , c_info.a
        ) ;
        RoundedRectangle(
              cairo
            , 1 
            , 1 
            , (a.get_width() - 2)
            , double((a.get_height() - 2))
            , 4. 
        ) ;
        cairo->fill () ;

        double current_time = m_timer.elapsed () ;

        {
            Pango::FontDescription font_desc (text_font) ;
            int text_size_pt = static_cast<int> ((text_size_px * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ;
            font_desc.set_size (text_size_pt * PANGO_SCALE) ;
            font_desc.set_weight (Pango::WEIGHT_BOLD) ;

            std::string text  = get_text_at_time (current_time) ;
            double      alpha = get_text_alpha_at_time (current_time) ;

            Glib::RefPtr<Pango::Layout> layout = Glib::wrap (pango_cairo_create_layout (cairo->cobj ())) ;
            layout->set_font_description (font_desc) ;
            layout->set_text (text) ;

            int width, height;
            layout->get_pixel_size (width, height) ;

            cairo->move_to(
                  (a.get_width () - width) / 2
                , (a.get_height () - height) / 2
            ) ;

            const ThemeColor& c_text = theme->get_color( THEME_COLOR_TEXT ) ; 

            cairo->set_source_rgba(
                  c_text.r
                , c_text.g
                , c_text.b
                , alpha 
            ) ; 
            cairo->set_operator (Cairo::OPERATOR_ATOP) ;

            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
        }
    }

    bool
    KoboTitleInfo::update_frame ()
    {
        queue_draw ();

        return true;
    }

} // namespace MPX
