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
    const int pad = 1 ;

    Gdk::Color
    get_color_at_pos(
          const Gdk::Color&     c1
        , const Gdk::Color&     c2
        , const double          ratio
    )
    {
        Gdk::Color c ;

        double r = ( c1.get_red_p() * ( 1 - ratio ) + c2.get_red_p() * ratio ) ;
        double g = ( c1.get_green_p() * ( 1 - ratio ) + c2.get_green_p() * ratio ) ;
        double b = ( c1.get_blue_p() * ( 1 - ratio ) + c2.get_blue_p() * ratio ) ; 

        c.set_rgb_p( r, g, b ) ;

        return c ;
    }
}

namespace MPX
{
    KoboVolume::KoboVolume ()

        : m_volume( 0 )
        , m_clicked( false )

    {
        add_events(Gdk::EventMask(Gdk::LEAVE_NOTIFY_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK )) ;
        set_flags(Gtk::CAN_FOCUS) ;

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
        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

        const Gdk::Rectangle& a = get_allocation() ;

        const ThemeColor& c_base /* :) */ = theme->get_color( THEME_COLOR_BACKGROUND ) ; 
        const ThemeColor& c = theme->get_color( THEME_COLOR_SELECT ) ;
        const ThemeColor& ct = theme->get_color( THEME_COLOR_TEXT_SELECTED ) ;

        cairo->set_operator(Cairo::OPERATOR_SOURCE) ;
        cairo->set_source_rgba(
              c_base.r
            , c_base.g
            , c_base.b
            , c_base.a
        ) ;
        cairo->paint () ;

        double percent = double(m_volume) / 100. ; 
        cairo->set_operator( Cairo::OPERATOR_OVER ) ;

        if( m_volume )
        {
            GdkRectangle r ;

            r.x         = pad ; 
            r.y         = (a.get_height() - 14) / 2 ; 
            r.width     = fmax( 0, (a.get_width()-2*pad) * double(percent)) ;
            r.height    = 14 ; 

            cairo->save () ;

            Gdk::Color c1 ;
            c1.set_rgb_p( .2, .2, .2 ) ;

            Gdk::Color c2 ;
            c2.set_rgb_p( c.r, c.g, c.b ) ; 

            Gdk::Color c_gradient = get_color_at_pos( c1, c2,  percent ) ;

            cairo->set_source_rgba(
                  c_gradient.get_red_p()
                , c_gradient.get_green_p()
                , c_gradient.get_blue_p()
                , .6
            ) ;

            cairo->set_operator(
                  Cairo::OPERATOR_OVER
            ) ;

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

            cairo->set_source_rgba(
                  ct.r
                , ct.g
                , ct.b
                , ct.a
            ) ;
            cairo->set_operator( Cairo::OPERATOR_SOURCE ) ;
            pango_cairo_show_layout (cairo->cobj (), layout->gobj ()) ;
        }

        GdkRectangle r ;

        r.x = 0 ;
        r.y = 0 ;
        r.width = a.get_width() ;
        r.height = a.get_height() ;

        if( has_focus() )
        {
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
            grab_focus() ;
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

    void
    KoboVolume::vol_down()
    {
        m_volume = ((m_volume+4)/5)*5 - 5 ; 
        m_volume = std::max( m_volume, 0 ) ;
        m_volume = std::min( m_volume, 100 ) ;
        m_SIGNAL_set_volume.emit( m_volume ) ;
        queue_draw () ;
    }

    void
    KoboVolume::vol_up()
    {
        m_volume = (m_volume/5)*5 + 5 ; 
        m_volume = std::max( m_volume, 0 ) ;
        m_volume = std::min( m_volume, 100 ) ;
        m_SIGNAL_set_volume.emit( m_volume ) ;
        queue_draw () ;
    }

    bool
    KoboVolume::on_scroll_event(
        GdkEventScroll* event
    ) 
    {
        if( event->direction == GDK_SCROLL_DOWN )
        {
            vol_down () ;
        }
        else
        if( event->direction == GDK_SCROLL_UP )
        {
            vol_up () ;
        }

        return true ;
    }

    bool
    KoboVolume::on_key_press_event(
        GdkEventKey* event
    )
    {
        switch( event->keyval )
        {
            case GDK_Left:
            case GDK_KP_Left:
            case GDK_Down:
            case GDK_KP_Down:

                vol_down () ;

                return true ;

            case GDK_Up:
            case GDK_KP_Up:
            case GDK_Right:
            case GDK_KP_Right:

                vol_up () ;

                return true ;

            default: break;
        }

        return false ;
    }

}
