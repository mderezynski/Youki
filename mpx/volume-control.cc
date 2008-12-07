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
#include "mpx/mpx-main.hh"
#include "volume-control.hh"
#include <gtkmm.h>
#include <gtk/gtk.h>

namespace
{
    inline int
    clamp_value( int a, int x, int b )
    {
        if( x < a )
            return a;

        if( x > b )
            return b;

        return x;
    }

    const double HIDE_TIMEOUT = .5;
}

namespace MPX
{
        VolumeControl::VolumeControl(
            const Glib::RefPtr<Gnome::Glade::Xml>& xml
        ) 
        {
            xml->get_widget( "volume-button", m_volume_toggle ) ;
            xml->get_widget( "volume-range",  m_volume_range ) ;
            xml->get_widget( "volume-window", m_volume_window ) ;
            xml->get_widget( "volume-image",  m_volume_image ) ;

            m_volume_pixbufs[0] = m_volume_toggle->render_icon( 
                Gtk::StockID( "audio-volume-muted" ),
                Gtk::ICON_SIZE_BUTTON
            );

            m_volume_pixbufs[1] = m_volume_toggle->render_icon( 
                Gtk::StockID( "audio-volume-low" ),
                Gtk::ICON_SIZE_BUTTON
            );

            m_volume_pixbufs[2] = m_volume_toggle->render_icon( 
                Gtk::StockID( "audio-volume-medium" ),
                Gtk::ICON_SIZE_BUTTON
            );

            m_volume_pixbufs[3] = m_volume_toggle->render_icon( 
                Gtk::StockID( "audio-volume-high" ),
                Gtk::ICON_SIZE_BUTTON
            );

            gtk_widget_realize( GTK_WIDGET( m_volume_window->gobj() ));


            m_volume_window->signal_button_press_event().connect(
                    sigc::mem_fun(
                        *this,
                        &VolumeControl::volume_window_button_press_event_cb
            ));


            m_volume_toggle->signal_clicked().connect(
                    sigc::mem_fun(
                        *this,
                        &VolumeControl::volume_toggle_clicked_cb
            ));
                    
            m_volume_toggle->signal_key_press_event().connect(
                    sigc::mem_fun(
                        *this,
                        &VolumeControl::volume_toggle_key_press_event_cb
            ));


            m_volume_range->signal_button_release_event().connect(
                    sigc::mem_fun(
                        *this,
                        &VolumeControl::volume_range_button_release_cb
            ));

            m_volume_range->signal_value_changed().connect(
                    sigc::mem_fun(
                        *this,
                        &VolumeControl::volume_range_value_changed_cb
            ));

            m_volume_range->signal_focus_out_event().connect(
                    sigc::mem_fun(
                        *this,
                        &VolumeControl::volume_range_focus_out_event_cb
            ));
        }

        void
        VolumeControl::set_volume( int volume )
        {
            m_volume_range->set_value( volume );
        }

        bool
        VolumeControl::volume_range_button_release_cb(
            GdkEventButton* G_GNUC_UNUSED
        )
        {
            m_volume_window->hide_all();
            return false;
        }

        void
        VolumeControl::volume_range_value_changed_cb()
        {
            ValueChanged( m_volume_range->get_value() );
            volume_hide_timeout_readd();
            adapt_image();
        }

        void
        VolumeControl::volume_show ()
        {
            int     x_ro,
                    y_ro;

            int     x_widget,
                    y_widget;

            int     x_size,
                    y_size;

            int     pos_x,
                    pos_y;

            int     offset;

            x_size = GTK_WIDGET(m_volume_toggle->gobj())->allocation.width;
            y_size = GTK_WIDGET(m_volume_toggle->gobj())->allocation.height;

            gdk_window_get_root_origin(
                GTK_WIDGET(m_volume_toggle->gobj())->window,
                &x_ro,
                &y_ro
            );

            gdk_window_get_position(
                GTK_WIDGET(m_volume_toggle->gobj())->window,
                &x_widget,
                &y_widget
            );

            if( x_size > GTK_WIDGET(m_volume_window->gobj())->allocation.width )
                offset = (x_size - GTK_WIDGET(m_volume_window->gobj())->allocation.width) / 2;
            else
                offset = -((GTK_WIDGET(m_volume_window->gobj())->allocation.width - x_size) / 2);

            pos_x = x_widget + GTK_WIDGET(m_volume_toggle->gobj())->allocation.x + offset;
            pos_y = y_widget + GTK_WIDGET(m_volume_toggle->gobj())->allocation.y - (GTK_WIDGET(m_volume_window->gobj())->allocation.height + 4 /* grace height */ );

            m_volume_window->move( pos_x, pos_y );
            m_volume_window->show_all();
            m_volume_range->grab_focus();

            volume_hide_timeout_readd();
        }

        bool
        VolumeControl::volume_toggle_clicked_cb_idle()
        {
            return false;
        }

        void
        VolumeControl::volume_hide_real ()
        {
            m_volume_window->hide_all();
        }

        bool
        VolumeControl::volume_hide ()
        {
            if( m_keypress_timer.elapsed() > HIDE_TIMEOUT ) 
            {
                m_hide_conn.disconnect();
                volume_hide_real ();
            }

            return true;
        }

        bool
        VolumeControl::volume_window_button_press_event_cb(
            GdkEventButton* G_GNUC_UNUSED
        )
        {
            volume_hide_real ();
            return true;
        }

        void
        VolumeControl::volume_hide_timeout_readd()
        {
            m_keypress_timer.reset();

            if( !m_hide_conn )
            {
                m_hide_conn = Glib::signal_timeout().connect(
                        sigc::mem_fun(
                            *this,
                            &VolumeControl::volume_hide
                        ),
                        50
                    );
            }
        }

        void
        VolumeControl::volume_toggle_clicked_cb()
        {
            if( m_volume_window->is_visible() )
            {
                m_volume_window->hide_all();
            }
            else
            {
                volume_show ();
            }
        }

        bool
        VolumeControl::volume_range_focus_out_event_cb(
            GdkEventFocus* G_GNUC_UNUSED
        )
        {
            m_volume_window->hide_all();
            return false;
        }

        bool
        VolumeControl::volume_toggle_key_press_event_cb(
            GdkEventKey* event
        )
        {
            int offset;
                    

            if(
                (event->keyval == GDK_Page_Up)
                            ||
                (event->keyval == GDK_Page_Down)
                            ||
                (event->keyval == GDK_Down)
                            ||
                (event->keyval == GDK_Up)
            )
            {
                switch (event->keyval)
                {
                        case GDK_Page_Up:
                            offset = +10;
                            break;

                        case GDK_Page_Down:
                            offset = -10;
                            break;

                        case GDK_Up:
                            offset = +1;
                            break;

                        case GDK_Down:
                            offset = -1;
                            break;

                        default:
                            g_assert_not_reached ();
                }

                volume_show ();
                m_volume_range->set_value( clamp_value( 0, mcs->key_get<int>("mpx","volume") + offset, 100 ) );

                return true;
            }

            return false;
        }

        void
        VolumeControl::adapt_image ()
        {
            if( m_volume_range->get_value() == 0 )
                m_volume_image->set( m_volume_pixbufs[0] );
            else
                m_volume_image->set( m_volume_pixbufs[clamp_value( 0, (m_volume_range->get_value()/34), 2 )+1]);
        }

} // MPX
