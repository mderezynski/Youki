#include <gtkmm.h>
#include <boost/format.hpp>
#include <cmath>

#include "mpx/mpx-main.hh"
#include "mpx/algorithm/limiter.hh"
#include "mpx/algorithm/interval.hh"
#include "mpx/widgets/cairo-extensions.hh"

#include "mpx/i-youki-play.hh"
#include "mpx/i-youki-theme-engine.hh"

#include "mpx/util-graphics.hh"

#include "kobo-position.hh"

namespace
{
    int const animation_fps = 2 ;
    int const animation_frame_period_ms = 1000 / animation_fps ;
}

namespace MPX
{
    inline double
    KoboPosition::cos_smooth (double x)
    {
        return (2.0 - std::cos (x * G_PI)) / 2.0;
    }

    double
    KoboPosition::get_position ()
    {
        return 0. ;
    }

    KoboPosition::KoboPosition(
    )

        : m_duration( 0 )
        , m_seek_position( 0 )
        , m_seek_factor( 0 )
        , m_clicked( false )
        , m_clock( 0 )

    {
        add_events(Gdk::EventMask(Gdk::LEAVE_NOTIFY_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK )) ;
        set_flags(Gtk::CAN_FOCUS) ;
    }

    KoboPosition::~KoboPosition () 
    {
    }

    void
    KoboPosition::stop()
    {
/*
        m_update_conn.disconnect() ;
        m_clock = 0 ;
        queue_draw() ;
*/
    }

    void
    KoboPosition::start()
    {
/*
        boost::shared_ptr<IPlay> p = services->get<IPlay>("mpx-service-play") ;

        m_clock = p->get_clock() ;

        if( !m_update_conn )
        {
            m_update_conn = Glib::signal_timeout().connect(
                sigc::mem_fun(
                      *this
                    , &KoboPosition::draw_frame 
            ), animation_frame_period_ms) ;
        }
*/
    }

    void
    KoboPosition::on_size_allocate( Gtk::Allocation& alloc )
    {
        Gtk::DrawingArea::on_size_allocate( alloc ) ;
    }

    void
    KoboPosition::on_size_request( Gtk::Requisition * req )
    {
        req->height = 17 ;  
    }

    void
    KoboPosition::set_position(
          gint64    duration
        , gint64    position
    )
    {
        m_position = position ;
        m_duration = duration ;
        queue_draw () ;
    }

    bool
    KoboPosition::on_expose_event(
        GdkEventExpose* G_GNUC_UNUSED
    )
    {
        Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context() ;
        const Gdk::Rectangle& a = get_allocation() ;

        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

        const ThemeColor& c = theme->get_color( THEME_COLOR_SELECT ) ;

        Gdk::Color cgdk ;
        double h, s, b ;

        cgdk.set_rgb_p( c.r, c.g, c.b ) ;
        Util::color_to_hsb( cgdk, h, s, b ) ;

        Gdk::Color c_base_gdk = Util::color_from_hsb( h, s, b ) ;

        // BAR BACKGROUND

        Cairo::RefPtr<Cairo::LinearGradient> volume_bar_back_gradient = Cairo::LinearGradient::create(
              a.get_width() / 2 
            , 1 
            , a.get_width() / 2 
            , 15
        ) ;

        volume_bar_back_gradient->add_color_stop_rgba(
              0. 
            , c_base_gdk.get_red_p()
            , c_base_gdk.get_green_p()
            , c_base_gdk.get_blue_p()
            , 0.2 
        ) ;

        volume_bar_back_gradient->add_color_stop_rgba(
              .2
            , c_base_gdk.get_red_p()
            , c_base_gdk.get_green_p()
            , c_base_gdk.get_blue_p()
            , 0.195 
        ) ;

        volume_bar_back_gradient->add_color_stop_rgba(
              .4
            , c_base_gdk.get_red_p()
            , c_base_gdk.get_green_p()
            , c_base_gdk.get_blue_p()
            , 0.185
        ) ;

        volume_bar_back_gradient->add_color_stop_rgba(
              .6
            , c_base_gdk.get_red_p()
            , c_base_gdk.get_green_p()
            , c_base_gdk.get_blue_p()
            , 0.185
        ) ;
        
        volume_bar_back_gradient->add_color_stop_rgba(
              .9
            , c_base_gdk.get_red_p()
            , c_base_gdk.get_green_p()
            , c_base_gdk.get_blue_p()
            , 0.195
        ) ;

        volume_bar_back_gradient->add_color_stop_rgba(
              1. 
            , c_base_gdk.get_red_p()
            , c_base_gdk.get_green_p()
            , c_base_gdk.get_blue_p()
            , 0.2 
        ) ;

        cairo->set_source( volume_bar_back_gradient ) ;
        cairo->set_operator( Cairo::OPERATOR_OVER ) ;

        RoundedRectangle(
              cairo
            , 1 
            , 1 
            , a.get_width() - 2
            , 15
            , 2.
        ) ;
        cairo->fill () ;

        // BAR

        double factor       = 1. ;
        gint64 position     = m_clicked ? m_seek_position : m_position ;
        double percent      = double(position) / double(m_duration) ; 

        if( percent >= 0.90 )
        {
            factor = (1. - percent) * 10. ; 
        }

        if( percent > 0. )
        {
            GdkRectangle r ;

            r.x         = 1 ; 
            r.y         = 1 ; 
            r.width     = ((a.get_width() - 2) * percent) /*+ posadd*/ ;
            r.height    = 15 ; 

            cairo->save () ;

            Cairo::RefPtr<Cairo::LinearGradient> volume_bar_gradient = Cairo::LinearGradient::create(
                  r.x + r.width / 2
                , r.y  
                , r.x + r.width / 2
                , r.y + r.height
            ) ;

            double h, s, b ;
            
            Util::color_to_hsb( cgdk, h, s, b ) ;
            b *= 0.85 ; 
            Gdk::Color c1 = Util::color_from_hsb( h, s, b ) ;

            Util::color_to_hsb( cgdk, h, s, b ) ;
            b *= 0.55 ; 
            Gdk::Color c2 = Util::color_from_hsb( h, s, b ) ;

            Util::color_to_hsb( cgdk, h, s, b ) ;
            b *= 0.35 ; 
            Gdk::Color c3 = Util::color_from_hsb( h, s, b ) ;

            volume_bar_gradient->add_color_stop_rgba(
                  0. 
                , c1.get_red_p()
                , c1.get_green_p()
                , c1.get_blue_p()
                , factor 
            ) ;

            volume_bar_gradient->add_color_stop_rgba(
                  .60
                , c2.get_red_p()
                , c2.get_green_p()
                , c2.get_blue_p()
                , factor
            ) ;
            
            volume_bar_gradient->add_color_stop_rgba(
                  1. 
                , c3.get_red_p()
                , c3.get_green_p()
                , c3.get_blue_p()
                , factor
            ) ;

            cairo->set_source( volume_bar_gradient ) ;
            cairo->set_operator( Cairo::OPERATOR_OVER ) ;

            RoundedRectangle(
                  cairo
                , r.x 
                , r.y
                , r.width 
                , r.height
                , 2.
            ) ;

            cairo->fill (); 
            cairo->restore () ;
        }

        if( m_duration > 0 ) 
        {
            ThemeColor ct = theme->get_color( THEME_COLOR_TEXT_SELECTED ) ;
            ct.a = 0.92 ;

            /// POSITION

            const int text_size_px = 10 ;
            const Gtk::Allocation& a = get_allocation() ;

            Pango::FontDescription font_desc = get_style()->get_font() ;
            int text_size_pt = static_cast<int> ((text_size_px * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ;
            font_desc.set_size (text_size_pt * PANGO_SCALE) ;
            Glib::RefPtr<Pango::Layout> layout = Glib::wrap (pango_cairo_create_layout (cairo->cobj ())) ;
            layout->set_font_description (font_desc) ;

            layout->set_markup(
                (boost::format("<b>%02d</b>:<b>%02d</b>") % ( position / 60 ) % ( position % 60 )).str()
            ) ;

            Pango::Rectangle rl, ri ;
            GdkRectangle r ;

            layout->get_extents( rl, ri ) ; 

            r.width = rl.get_width() / PANGO_SCALE ;
            r.height = rl.get_height() / PANGO_SCALE ;
            r.x = fmax( 3, 3 + double(a.get_width()) * double(percent) - r.width - 7 ) ; 
            r.y = (a.get_height() - r.height) / 2 ; 

            cairo->move_to(
                  r.x                  
                , r.y 
            ) ;

            cairo->set_source_rgba(
                  ct.r
                , ct.g
                , ct.b
                , ct.a
            ) ;

            cairo->set_operator( Cairo::OPERATOR_SOURCE ) ;
            pango_cairo_show_layout( cairo->cobj (), layout->gobj () ) ;

            if( ( m_duration - m_position ) > 1 )
            {
                    /// DURATION

                    layout->set_markup(
                        (boost::format("<b>%02d</b>:<b>%02d</b>") % ( m_duration / 60 ) % ( m_duration % 60 )).str()
                    ) ;

                    layout->get_extents( rl, ri ) ; 

                    r.width = rl.get_width() / PANGO_SCALE ;
                    r.height = rl.get_height() / PANGO_SCALE ;
                    r.x = 3 + double(a.get_width()) - r.width - 7 ;
                    r.y = (a.get_height() - r.height) / 2 ; 

                    cairo->move_to(
                          r.x 
                        , r.y
                    ) ;

                    cairo->set_source_rgba(
                          ct.r
                        , ct.g
                        , ct.b
                        , ct.a * factor
                    ) ;

                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;
                    pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
            }
        }

        if( has_focus() )
        {
            GdkRectangle r ; 

            r.x = 0 ;
            r.y = 0 ;

            r.width = a.get_width() ;
            r.height = a.get_height() ;

            theme->draw_focus(
                  cairo
                , r 
                , is_sensitive()
                , 2.
            ) ;
        }

        return true ;
    }

    bool
    KoboPosition::on_leave_notify_event(
        GdkEventCrossing* G_GNUC_UNUSED
    )
    {
        return true ;
    }

    bool
    KoboPosition::on_enter_notify_event(
        GdkEventCrossing*
    )
    {
        return true ;
    }

    bool
    KoboPosition::on_button_press_event(
        GdkEventButton* event
    ) 
    {
        if( event->button == 1 )
        {
            grab_focus() ;
        
            const Gtk::Allocation& a = get_allocation() ;

            Interval<std::size_t> i (
                  Interval<std::size_t>::IN_IN
                , 0 
                , a.get_width()
            ) ;

            if( i.in( event->x ) ) 
            {
                m_clicked       = true ;
                m_seek_factor   = double(a.get_width()-2) / double(m_duration) ;

                Limiter<gint64> p (
                      Limiter<gint64>::ABS_ABS
                    , 0
                    , m_duration 
                    , event->x / m_seek_factor
                ) ;

                m_seek_position = p ; 

                queue_draw () ;
            }
        }

        return true ;
    }

    bool
    KoboPosition::on_motion_notify_event(
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

            const Gtk::Allocation& a = get_allocation() ;

            Interval<std::size_t> i (
                  Interval<std::size_t>::IN_IN
                , 0 
                , a.get_width()
            ) ;

            if( i.in( x_orig ))
            {
                Limiter<gint64> p (
                      Limiter<gint64>::ABS_ABS
                    , 0
                    , m_duration
                    , x_orig / m_seek_factor
                ) ;

                m_seek_position = p ; 
            }

            queue_draw () ;
        }

        return true ;
    }

    bool
    KoboPosition::on_button_release_event(
        GdkEventButton* G_GNUC_UNUSED
    )
    {
        m_position = m_seek_position ;
        m_clicked  = false ;

        m_SIGNAL_seek_event.emit( m_seek_position ) ;

        queue_draw () ;

        return true ;
    }

    bool
    KoboPosition::on_focus_in_event(
        GdkEventFocus* event
    )
    {
        queue_draw() ;
        return false ;
    }

    bool
    KoboPosition::on_key_press_event(
        GdkEventKey* event
    )
    {
        Limiter<gint64> p ;

        switch( event->keyval )
        {
            case GDK_Down:
            case GDK_KP_Down:

                p = Limiter<gint64> (
                      Limiter<gint64>::ABS_ABS
                    , 0
                    , m_duration
                    , m_position - 5 
                ) ;

                m_seek_position = p ; 
                m_SIGNAL_seek_event.emit( m_seek_position ) ;

                return true ;

            case GDK_Up:
            case GDK_KP_Up:

                p = Limiter<gint64> (
                      Limiter<gint64>::ABS_ABS
                    , 0
                    , m_duration
                    , m_position + 5 
                ) ;

                m_seek_position = p ; 
                m_SIGNAL_seek_event.emit( m_seek_position ) ;

                return true ;

            default: break;
        }

        return false ;
    }
}
