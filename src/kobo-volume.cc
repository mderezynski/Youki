#include <gtkmm.h>
#include <boost/format.hpp>
#include <cmath>
#include "kobo-volume.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/util-graphics.hh"

#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-main.hh"

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

        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
        const ThemeColor& c = theme->get_color( THEME_COLOR_BASE ) ;
        Gdk::Color cgdk ;
        cgdk.set_rgb_p( c.r, c.g, c.b ) ; 
        modify_bg( Gtk::STATE_NORMAL, cgdk ) ;
        modify_base( Gtk::STATE_NORMAL, cgdk ) ;
    }

    KoboVolume::~KoboVolume () 
    {
    }
    
    void
    KoboVolume::on_size_request( Gtk::Requisition * req )
    {
        req->width  = 100 + 2*pad ;
        req->height = 16 ;
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

        cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

        if( m_volume )
        {
            GdkRectangle r ;

            r.x         = pad ; 
            r.y         = (a.get_height() - 14) / 2 ; 
            r.width     = fmax( 0, (a.get_width()-2*pad) * double(percent)) ;
            r.height    = 14 ; 

            cairo->save () ;

            Cairo::RefPtr<Cairo::LinearGradient> background_gradient_ptr = Cairo::LinearGradient::create(
                  r.x + r.width / 2
                , r.y  
                , r.x + r.width / 2
                , r.y + r.height
            ) ;
            
            background_gradient_ptr->add_color_stop_rgba(
                  0. 
                , c.get_red_p() 
                , c.get_green_p()
                , c.get_blue_p()
                , 0.85
            ) ;

            background_gradient_ptr->add_color_stop_rgba(
                  .60
                , c.get_red_p() 
                , c.get_green_p()
                , c.get_blue_p()
                , 0.55
            ) ;
            
            background_gradient_ptr->add_color_stop_rgba(
                  1. 
                , c.get_red_p() 
                , c.get_green_p()
                , c.get_blue_p()
                , 0.35
            ) ;

            cairo->set_source( background_gradient_ptr ) ;
            cairo->set_operator( Cairo::OPERATOR_ATOP ) ;

            RoundedRectangle(
                  cairo
                , r.x 
                , r.y
                , r.width 
                , r.height
                , rounding
            ) ;

            cairo->fill (); 
            cairo->restore () ;
        }

        {
            const int text_size_px = 10 ;

            const Gtk::Allocation& a = get_allocation() ;

            Pango::FontDescription font_desc = get_style()->get_font() ; 
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
                , (get_height() - height)/2.
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

    bool
    KoboVolume::on_scroll_event(
        GdkEventScroll* event
    ) 
    {
        if( event->direction == GDK_SCROLL_DOWN )
        {
            m_volume = m_volume - 3 ; 
            m_volume = std::max( m_volume, 0 ) ;
            m_volume = std::min( m_volume, 100 ) ;

            m_SIGNAL_set_volume.emit( m_volume ) ;

            queue_draw () ;
        }
        else if( event->direction == GDK_SCROLL_UP )
        {
            m_volume = m_volume + 3 ; 
            m_volume = std::max( m_volume, 0 ) ;
            m_volume = std::min( m_volume, 100 ) ;

            m_SIGNAL_set_volume.emit( m_volume ) ;

            queue_draw () ;
        }

        return true ;
    }
}
