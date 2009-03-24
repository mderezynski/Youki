#include <gtkmm.h>
#include <boost/format.hpp>
#include "kobo-position.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/util-graphics.hh"

namespace MPX
{
    KoboPosition::KoboPosition ()

        : m_percent( 0 )
        , m_duration( 0 )
        , m_clicked( false )
        , m_seek_position( 0 )
        , m_seek_factor( 0 )

    {
        add_events(Gdk::EventMask(Gdk::LEAVE_NOTIFY_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK )) ;
    }

    KoboPosition::~KoboPosition () 
    {
    }
    
    void
    KoboPosition::on_size_request( Gtk::Requisition * req )
    {
        req->height = 20 ;  
    }

    void
    KoboPosition::set_position(
          gint64    duration
        , gint64    position
    )
    {
        m_duration = duration ;
        m_position = position ;

        m_percent = double(position) / double(duration) ; 

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
        cairo->set_source_rgba( 0.12, 0.12, 0.12, 1. ) ;
        cairo->rectangle(
              0 
            , 0 
            , a.get_width()
            , 20
        ) ;
        cairo->fill () ;

        RoundedRectangle(
              cairo
            , 4 
            , 4 
            , double((a.get_width() - 8))
            , 12
            , 4.
        ) ;
        cairo->clip () ;

        {
               cairo->set_operator( Cairo::OPERATOR_ATOP ) ;
               cairo->set_source_rgba(
                      c.get_red_p() 
                    , c.get_green_p() 
                    , c.get_blue_p()
                    , .2 
                ) ;
                RoundedRectangle(
                      cairo
                    , 4 
                    , 4 
                    , double((a.get_width() - 8))
                    , 12
                    , 4.
                ) ;
                cairo->fill () ;
        }

        double factor   = 1. ;
        gint64 position = m_clicked ? m_seek_position : m_position ;
        double percent  = double(position) / double(m_duration) ; 

        if( percent >= 0.90 )
        {
            factor = (1. - percent) * 10. ; 
        }

        if( m_percent > 0. )
        {
                cairo->set_operator( Cairo::OPERATOR_ATOP ) ;
                cairo->set_source_rgba(
                      c.get_red_p() 
                    , c.get_green_p() 
                    , c.get_blue_p()
                    , 1. * factor 
                ) ;
                RoundedRectangle(
                      cairo
                    , 4 
                    , 4 
                    , double((a.get_width() - 8)) * double(percent)
                    , 12
                    , 4.
                ) ;
                cairo->fill () ;
        }

        if( m_duration > 0 ) 
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
                (boost::format("%02d:%02d") % ( position / 60 ) % ( position % 60 )).str()
            ) ;
            int width, height;
            layout->get_pixel_size (width, height) ;
            double position_x = 4 + double((a.get_width() - 8)) * double(percent) - width - 2;
            position_x = (position_x < 4) ? 4 : position_x ;
            cairo->move_to(
                  position_x                  
                , (a.get_height () - height) / 2
            ) ;
            cairo->set_source_rgba( 1., 1., 1., 1. ) ;
            cairo->set_operator( Cairo::OPERATOR_ADD ) ;
            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;

            layout->set_text(
                (boost::format("%02d:%02d") % ( m_duration / 60 ) % ( m_duration % 60 )).str()
            ) ;
            layout->get_pixel_size (width, height) ;
            cairo->move_to(
                  4 + double((a.get_width() - 8)) - width - 2
                , (a.get_height () - height) / 2
            ) ;
            cairo->set_source_rgba( 1., 1., 1., 1. * factor ) ;
            cairo->set_operator( Cairo::OPERATOR_ADD ) ;
            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
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
            const Gtk::Allocation& a = get_allocation() ;

            m_clicked = true ;
            m_seek_factor = double(a.get_width()) / double(m_duration) ;
            m_seek_position = double(event->x) / m_seek_factor ;
            m_seek_position = (m_seek_position > m_duration) ? m_duration : m_seek_position ;
            m_seek_position = (m_seek_position < 0) ? 0 : m_seek_position ;
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
            m_seek_position = (m_seek_position > m_duration) ? m_duration : m_seek_position ;
            m_seek_position = (m_seek_position < 0) ? 0 : m_seek_position ;
            queue_draw () ;
        }

        return true ;
    }
}
