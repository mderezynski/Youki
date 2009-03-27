#include <gtkmm.h>
#include <boost/format.hpp>
#include <cmath>
#include "kobo-volume.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/util-graphics.hh"

namespace
{
    const double rounding = 2. ; 
    const int    pad = 1 ;
}

namespace MPX
{
    KoboVolume::KoboVolume ()

        : m_volume( 0 )
        , m_clicked( false )

    {
        add_events(Gdk::EventMask(Gdk::LEAVE_NOTIFY_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK )) ;
    }

    KoboVolume::~KoboVolume () 
    {
    }
    
    void
    KoboVolume::on_size_request( Gtk::Requisition * req )
    {
        req->width  = 100 + 2*pad ;
        req->height = 12 ;
    }

    void
    KoboVolume::set_volume(
          int volume
    )
    {
        m_volume = volume ;
        queue_draw () ;
    }

    bool
    KoboVolume::on_expose_event(
        GdkEventExpose* G_GNUC_UNUSED
    )
    {
        Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context() ;
        const Gdk::Rectangle& a = get_allocation() ;
        Gdk::Color c ;

        c.set_rgb_p( .7, .7, .7 ) ;


        double percent = double(m_volume) / 100. ; 

        cairo->set_operator( Cairo::OPERATOR_SOURCE ) ;
        // render background 
        {
                cairo->set_source_rgba(
                      .10 
                    , .10 
                    , .10 
                    , 1. 
                ) ;

                cairo->rectangle(
                      0 
                    , 0 
                    , a.get_width()
                    , a.get_height() 
                ) ;

                cairo->fill () ;
        }

        cairo->set_operator( Cairo::OPERATOR_ATOP ) ;
        // render volume bar
        {
                cairo->set_source_rgba(
                      c.get_red_p() 
                    , c.get_green_p() 
                    , c.get_blue_p()
                    , .6
                ) ;

                RoundedRectangle(
                      cairo
                    , pad 
                    , 0 
                    , (a.get_width()-2*pad) * double(percent)
                    , 12 
                    , rounding
                ) ;

                cairo->fill () ;
        }

        // draw volume text
        {
            const int text_size_px = 8 ;

            const Gtk::Allocation& a = get_allocation() ;

            Pango::FontDescription font_desc ("Sans") ;
            int text_size_pt = static_cast<int> ((text_size_px * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ;
            font_desc.set_size (text_size_pt * PANGO_SCALE) ;
            font_desc.set_weight (Pango::WEIGHT_BOLD) ;
            Glib::RefPtr<Pango::Layout> layout = Glib::wrap (pango_cairo_create_layout (cairo->cobj ())) ;
            layout->set_font_description (font_desc) ;

            layout->set_text(
                (boost::format("%d") % m_volume).str()
            ) ;
            int width, height;
            layout->get_pixel_size (width, height) ;

            cairo->move_to(
                  fmax( 2, 2 + double((a.get_width() - pad*2)) * double(percent) - width - (pad+4) ) 
                , (a.get_height() - height) / 2
            ) ;

            cairo->set_source_rgba( 1., 1., 1., 1. ) ;
            cairo->set_operator( Cairo::OPERATOR_SOURCE ) ;
            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
        }

        return true ;
    }

    bool
    KoboVolume::on_leave_notify_event(
        GdkEventCrossing* G_GNUC_UNUSED
    )
    {
        return true ;
    }

    bool
    KoboVolume::on_enter_notify_event(
        GdkEventCrossing*
    )
    {
        return true ;
    }

    bool
    KoboVolume::on_button_press_event(
        GdkEventButton* event
    ) 
    {
        if( event->button == 1 )
        {
            m_clicked = true ;

            m_volume = event->x ; 
            m_volume = std::max( m_volume, 0 ) ;
            m_volume = std::min( m_volume, 100 ) ;

            m_SIGNAL_set_volume.emit( m_volume ) ;

            queue_draw () ;
        }

        return true ;
    }

    bool
    KoboVolume::on_button_release_event(
        GdkEventButton* event
    )
    {
        m_clicked = false ;

        m_volume = event->x ; 
        m_volume = std::max( m_volume, 0 ) ;
        m_volume = std::min( m_volume, 100 ) ;

        m_SIGNAL_set_volume.emit( m_volume ) ;

        queue_draw () ;

        return true ;
    }

    bool
    KoboVolume::on_motion_notify_event(
        GdkEventMotion* event
    )
    {
        if( m_clicked )
        {
            int x_orig, y_orig;
            GdkModifierType state;

            if (event->is_hint)
            {
                gdk_window_get_pointer (event->window, &x_orig, &y_orig, &state);
            }
            else
            {
                x_orig = int (event->x);
                y_orig = int (event->y);
                state = GdkModifierType (event->state);
            }

            m_volume = x_orig ;
            m_volume = std::max( m_volume, 0 ) ;
            m_volume = std::min( m_volume, 100 ) ;

            m_SIGNAL_set_volume.emit( m_volume ) ;

            queue_draw () ;
        }

        return true ;
    }
}
