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

#include "config.h"

#include "youki-togglebutton.hh"

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdkmm/general.h>
#include <gtkmm/icontheme.h>
#include <cairomm/cairomm.h>
#include <cmath>
//#include <boost/shared_ptr.hpp>

#include "mpx/util-graphics.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-main.hh"

namespace
{
    const int xpad = 5 ;
}

namespace MPX
{
    YoukiToggleButton::YoukiToggleButton(
              int                   pixbuf_size
            , const std::string&    s1
            , const std::string&    s2
            , const std::string&    s3
    )

        : m_pixbuf_size( pixbuf_size )

    {
        set_size_request( pixbuf_size + 2*xpad ) ;

        Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();

        m_pixbuf[TOGGLE_BUTTON_STATE_NONE]  = theme->load_icon( s1, m_pixbuf_size ) ;
        m_pixbuf[TOGGLE_BUTTON_STATE_ON]    = theme->load_icon( s2, m_pixbuf_size ) ;
        m_pixbuf[TOGGLE_BUTTON_STATE_OFF]   = theme->load_icon( s3, m_pixbuf_size ) ;
    }

    void
    YoukiToggleButton::set_state(
        ToggleButtonState state
    )
    {
        m_state = state;
        queue_draw() ;
    }

    void
    YoukiToggleButton::set_default_state(
        ToggleButtonState state
    )
    {
        m_default_state = state;
        queue_draw() ;
    }


    ToggleButtonState
    YoukiToggleButton::get_state()
    {
        return m_state;
    }

    bool
    YoukiToggleButton::on_expose_event (GdkEventExpose *event)
    {
        draw_frame ();
        return false;
    }

    void
    YoukiToggleButton::on_clicked()
    {
        if( is_sensitive() )
        {
            m_state = ToggleButtonState( int(m_state+1) % N_TOGGLE_BUTTON_STATES );
            queue_draw() ;
        }
    }

    void
    YoukiToggleButton::draw_frame ()
    {
        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

        Cairo::RefPtr<Cairo::Context> cairo = get_window ()->create_cairo_context () ;

        const Gtk::Allocation& a = get_allocation() ;

        const ThemeColor& c_info = theme->get_color( THEME_COLOR_INFO_AREA ) ;
        const ThemeColor& c_sel  = theme->get_color( THEME_COLOR_SELECT ) ;

        cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

        cairo->set_source_rgba(
              c_info.r
            , c_info.g
            , c_info.b
            , c_info.a
        ) ;
        RoundedRectangle(
              cairo
            , a.get_x() + 1
            , a.get_y() + 1
            , (a.get_width() - 2)
            , double((a.get_height() - 2))
            , 4.
        ) ;

        if( is_sensitive() )
        {
            if( m_state == m_default_state )
            {
                cairo->fill_preserve () ;
                cairo->set_source_rgba(
                      c_sel.r
                    , c_sel.g
                    , c_sel.b
                    , 0.15
                ) ;
            }

            cairo->fill() ;

            GdkRectangle r ;

            int off = GTK_BUTTON(gobj())->GSEAL(depressed) ? 1 : 0 ;

            r.x         = a.get_x() + (a.get_width() - m_pixbuf_size) / 2 + off ;
            r.y         = a.get_y() + (a.get_height() - m_pixbuf_size) / 2 + off ;
            r.width     = m_pixbuf_size ;
            r.height    = m_pixbuf_size ;

            Gdk::Cairo::set_source_pixbuf(
                  cairo
                , m_pixbuf[m_state]
                , r.x
                , r.y
            ) ;

            cairo->rectangle(
                  r.x
                , r.y
                , r.width
                , r.height
            ) ;

            cairo->fill(
            ) ;
        }
        else
        {
            cairo->fill () ;
        }
    }
} // namespace MPX
