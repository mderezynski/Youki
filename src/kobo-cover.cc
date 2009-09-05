#include "config.h"
#include <gtkmm.h>
#include "kobo-cover.hh"
#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-main.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/util-graphics.hh"

namespace MPX
{
    KoboCover::KoboCover ()
    {
        m_Theme = (services->get<IYoukiThemeEngine>("mpx-service-theme")).get() ;
    }
    
    KoboCover::~KoboCover ()
    {
    }

    void
    KoboCover::set(
        Glib::RefPtr<Gdk::Pixbuf> cover
    )
    {
        if( cover )
        {
            m_cover = cover->scale_simple( 94, 94 , Gdk::INTERP_HYPER ) ;
            m_CurrentColor = Util::get_mean_color_for_pixbuf( cover ) ;
        }
        else
        {
            m_cover.reset() ;
        }

        queue_draw () ;
    }
    
    void
    KoboCover::clear ()
    {
        m_cover.reset() ;
        queue_draw () ;
    }

    bool
    KoboCover::on_expose_event(
        GdkEventExpose*
    )
    {
        Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context() ;
        
        cairo->set_operator( Cairo::OPERATOR_SOURCE ) ; 
        cairo->set_source_rgba( 0.1, 0.1, 0.1, 1. ) ;
        cairo->paint () ;     

        if( !m_cover )
        {
            return true ;
        }

        const Gtk::Allocation& a = get_allocation() ;
        cairo->set_operator( Cairo::OPERATOR_SOURCE ) ; 

        GdkRectangle r ;

        r.x = (a.get_width() - m_cover->get_width())/2. ;
        r.y = (a.get_height() - m_cover->get_height())/2. ;
        r.width = m_cover->get_width() ;
        r.height = m_cover->get_height() ;

        Gdk::Cairo::set_source_pixbuf(    
              cairo
            , m_cover
            , r.x 
            , r.y 
        ) ;  
        RoundedRectangle(
              cairo
            , r.x 
            , r.y 
            , r.width 
            , r.height 
            , 3.
        ) ;
        cairo->fill() ;

        Gdk::Color cgdk ;
        cgdk.set_rgb_p( m_CurrentColor.get_red_p(), m_CurrentColor.get_green_p(), m_CurrentColor.get_blue_p() ) ; 

        Cairo::RefPtr<Cairo::LinearGradient> gradient = Cairo::LinearGradient::create(
              r.x + r.width / 2
            , r.y  
            , r.x + r.width / 2
            , r.y + r.height
        ) ;

        double h, s, b ;
    
        double alpha = 0.25 ;
        
        Util::color_to_hsb( cgdk, h, s, b ) ;
        b *= 0.90 ; 
        Gdk::Color c1 = Util::color_from_hsb( h, s, b ) ;

        Util::color_to_hsb( cgdk, h, s, b ) ;
        b *= 0.75 ; 
        Gdk::Color c2 = Util::color_from_hsb( h, s, b ) ;

        Util::color_to_hsb( cgdk, h, s, b ) ;
        b *= 0.45 ; 
        Gdk::Color c3 = Util::color_from_hsb( h, s, b ) ;

        gradient->add_color_stop_rgba(
              1. 
            , c3.get_red_p()
            , c3.get_green_p()
            , c3.get_blue_p()
            , alpha / 3.
        ) ;
        gradient->add_color_stop_rgba(
              .40
            , c2.get_red_p()
            , c2.get_green_p()
            , c2.get_blue_p()
            , alpha / 2.
        ) ;
        gradient->add_color_stop_rgba(
              0.
            , c1.get_red_p()
            , c1.get_green_p()
            , c1.get_blue_p()
            , alpha 
        ) ;
        cairo->set_source( gradient ) ;
        cairo->set_operator( Cairo::OPERATOR_OVER ) ;
        RoundedRectangle(
              cairo
            , r.x 
            , r.y 
            , r.width 
            , r.height 
            , 3. 
        ) ;
        cairo->fill(); 

        ThemeColor c ;

        c.r = m_CurrentColor.get_red_p() ;
        c.g = m_CurrentColor.get_green_p() ;
        c.b = m_CurrentColor.get_blue_p() ;

        Cairo::RefPtr<Cairo::LinearGradient> gradient2 = Cairo::LinearGradient::create(
              r.x + r.width / 2
            , r.y
            , r.x + r.width / 2
            , r.y + r.height
        ) ;
        gradient2->add_color_stop_rgba(
              0. 
            , c.r
            , c.g
            , c.b
            , 0. 
        ) ;
        gradient2->add_color_stop_rgba(
              0.7
            , c.r
            , c.g
            , c.b
            , alpha / 4.
        ) ;
        gradient2->add_color_stop_rgba(
              1.
            , c.r
            , c.g
            , c.b
            , alpha / 2.5 
        ) ;

        cairo->set_source( gradient2 ) ;
        cairo->set_operator( Cairo::OPERATOR_OVER ) ;
        RoundedRectangle(
              cairo
            , r.x 
            , r.y
            , r.width
            , r.height
            , 3.
        ) ;
        cairo->fill() ;

        gradient = Cairo::LinearGradient::create(
              r.x + r.width / 2
            , r.y  
            , r.x + r.width / 2
            , r.y + r.height
        ) ;

        gradient->add_color_stop_rgba(
              1. 
            , c3.get_red_p()
            , c3.get_green_p()
            , c3.get_blue_p()
            , alpha / 2.
        ) ;
        gradient->add_color_stop_rgba(
              .40
            , c2.get_red_p()
            , c2.get_green_p()
            , c2.get_blue_p()
            , alpha / 1.3
        ) ;
        gradient->add_color_stop_rgba(
              0.
            , c1.get_red_p()
            , c1.get_green_p()
            , c1.get_blue_p()
            , alpha 
        ) ;

        cairo->set_source( gradient ) ;
        cairo->set_operator( Cairo::OPERATOR_ATOP ) ; 
        cairo->set_line_width( 2.5 ) ; 
        RoundedRectangle(
              cairo
            , r.x 
            , r.y 
            , r.width 
            , r.height 
            , 2.
        ) ;
        cairo->stroke() ;

        return true ;
    }
}
