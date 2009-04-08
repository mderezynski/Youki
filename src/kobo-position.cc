#include <gtkmm.h>
#include <boost/format.hpp>
#include <cmath>
#include "kobo-position.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/util-graphics.hh"

namespace
{
    const double rounding = 2. ; 
}

namespace MPX
{
    KoboPosition::KoboPosition(
        const Gdk::Color& bg
    )

        : m_duration( 0 )
        , m_position( 0 )
        , m_seek_position( 0 )
        , m_seek_factor( 0 )
        , m_clicked( false )
        , m_bg( bg )
    {
        add_events(Gdk::EventMask(Gdk::LEAVE_NOTIFY_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK )) ;
        set_flags(Gtk::CAN_FOCUS) ;
    }

    KoboPosition::~KoboPosition () 
    {
    }
    
    void
    KoboPosition::on_size_request( Gtk::Requisition * req )
    {
        req->height = 16 ;  
    }

    void
    KoboPosition::set_position(
          gint64    duration
        , gint64    position
    )
    {
        m_duration = duration ;
        m_position = position ;

        queue_draw () ;
    }

    bool
    KoboPosition::on_expose_event(
        GdkEventExpose* G_GNUC_UNUSED
    )
    {
        Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context() ;
        const Gdk::Rectangle& a = get_allocation() ;
        const Gdk::Color& c = get_style()->get_base( Gtk::STATE_SELECTED ) ;

        cairo->set_operator( Cairo::OPERATOR_SOURCE ) ;
        cairo->set_source_rgba(
              m_bg.get_red_p() 
            , m_bg.get_green_p()
            , m_bg.get_blue_p()
            , 1.
        ) ;
        cairo->rectangle(
              0 
            , 0 
            , a.get_width()
            , a.get_height()
        ) ;
        cairo->fill () ;

        cairo->set_operator( Cairo::OPERATOR_ATOP ) ;
        cairo->set_source_rgba(
              c.get_red_p() 
            , c.get_green_p() 
            , c.get_blue_p()
            , .2 
        ) ;
        RoundedRectangle(
              cairo
            , 1 
            , 1 
            , a.get_width() - 2
            , 14
            , rounding
        ) ;
        cairo->fill () ;

        double factor   = 1. ;
        gint64 position = m_clicked ? m_seek_position : m_position ;
        double percent  = double(position) / double(m_duration) ; 

        if( percent >= 0.90 )
        {
            factor = (1. - percent) * 10. ; 
        }

        if( percent > 0. )
        {
            GdkRectangle r ;

            r.x         = 1 ; 
            r.y         = 1 ; 
            r.width     = (a.get_width() - 2) * percent ; 
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
                , 0.85 * factor 
            ) ;

            background_gradient_ptr->add_color_stop_rgba(
                  .60
                , c.get_red_p() 
                , c.get_green_p()
                , c.get_blue_p()
                , 0.55 * factor
            ) ;
            
            background_gradient_ptr->add_color_stop_rgba(
                  1. 
                , c.get_red_p() 
                , c.get_green_p()
                , c.get_blue_p()
                , 0.35 * factor
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

        if( m_duration > 0 ) 
        {
            const int text_size_px = 10 ;

            const Gtk::Allocation& a = get_allocation() ;

            Pango::FontDescription font_desc ("Sans") ;
            int text_size_pt = static_cast<int> ((text_size_px * 72) / Util::screen_get_y_resolution (Gdk::Screen::get_default ())) ;
            font_desc.set_size (text_size_pt * PANGO_SCALE) ;
            Glib::RefPtr<Pango::Layout> layout = Glib::wrap (pango_cairo_create_layout (cairo->cobj ())) ;
            layout->set_font_description (font_desc) ;

            layout->set_markup(
                (boost::format("<b>%02d</b>:<b>%02d</b>") % ( position / 60 ) % ( position % 60 )).str()
            ) ;

            int width, height;
            layout->get_pixel_size (width, height) ;

            cairo->move_to(
                  fmax( 3, 3 + double(a.get_width()) * double(percent) - width - 6 ) 
                , 2 
            ) ;
            cairo->set_source_rgba( 1., 1., 1., 1. ) ;
            cairo->set_operator( Cairo::OPERATOR_OVER ) ;
            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;

            if( (m_duration - m_position) > 1 )
            {
                    layout->set_markup(
                        (boost::format("<b>%02d</b>:<b>%02d</b>") % ( m_duration / 60 ) % ( m_duration % 60 )).str()
                    ) ;
                    layout->get_pixel_size (width, height) ;
                    cairo->move_to(
                          3 + double(a.get_width()) - width - 6  
                        , 2
                    ) ;
                    cairo->set_source_rgba( 1., 1., 1., 1. * factor ) ;
                    cairo->set_operator( Cairo::OPERATOR_OVER ) ;
                    pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
            }
        }

        if( has_focus() )
        {
                double h, s, b ;

                Util::color_to_hsb( c, h, s, b ) ;
                b = std::min( 1., b+0.12 ) ;
                s = std::min( 1., s+0.12 ) ;
                Gdk::Color c1 = Util::color_from_hsb( h, s, b ) ;

                cairo->set_operator( Cairo::OPERATOR_ATOP ) ;
                cairo->set_source_rgba(
                      c1.get_red_p() 
                    , c1.get_green_p()
                    , c1.get_blue_p()
                    , 1. 
                ) ;
                RoundedRectangle(
                      cairo
                    , 1 
                    , 1 
                    , a.get_width() - 2
                    , 14
                    , rounding
                ) ;

                std::valarray<double> dashes ( 2 ) ;
                dashes[0] = 1. ;
                dashes[1] = 1. ;

                cairo->set_dash( dashes, 2 ) ;
                cairo->set_line_width( .75 ) ; 
                cairo->stroke () ;
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

            m_clicked = true ;

            m_seek_factor = double(a.get_width()) / double(m_duration) ;

            m_seek_position = double(event->x) / m_seek_factor ;
            m_seek_position = std::min( std::max( gint64(0), m_seek_position ),  m_duration ) ; 

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
        m_SIGNAL_seek_event.emit( m_seek_position ) ;
        m_clicked = false ;
        queue_draw () ;
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

            m_seek_position = double(x_orig) / m_seek_factor ;
            m_seek_position = std::min( std::max( gint64(0), m_seek_position),  m_duration ) ; 

            queue_draw () ;
        }

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
        switch( event->keyval )
        {
            case GDK_Down:
            case GDK_KP_Down:
                m_seek_position = m_position - 5 ; 
                m_seek_position = std::min( std::max( gint64(0), m_seek_position ),  m_duration ) ; 
                m_SIGNAL_seek_event.emit( m_seek_position ) ;
                return true ;

            case GDK_Up:
            case GDK_KP_Up:
                m_seek_position = m_position + 5 ; 
                m_seek_position = std::min( std::max( gint64(0), m_seek_position ),  m_duration ) ; 
                m_SIGNAL_seek_event.emit( m_seek_position ) ;
                return true ;

            default: break;
        }

        return false ;
    }
}
