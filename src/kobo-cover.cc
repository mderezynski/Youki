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
            m_cover = cover->scale_simple( 100, 100 , Gdk::INTERP_BILINEAR ) ;
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
        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

        Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context() ;
        
        const ThemeColor& c_base = theme->get_color( THEME_COLOR_BACKGROUND ) ; // all hail to the C-Base!

        cairo->set_source_rgba(
              c_base.r
            , c_base.g
            , c_base.b
            , c_base.a
        ) ;
        cairo->set_operator( Cairo::OPERATOR_SOURCE ) ; 
        cairo->paint () ;     

        if( !m_cover )
        {
            return true ;
        }

        const Gtk::Allocation& a = get_allocation() ;

        GdkRectangle r ;

        cairo->set_operator( Cairo::OPERATOR_OVER ) ; 

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
            , 4. 
        ) ;

        cairo->fill() ;

/*
        r.x = 0 ; 
        r.y = 3 ; 
        r.width = 98 ; 
        r.height = 98 ; 

        const ThemeColor& c_base = theme->get_color( THEME_COLOR_BACKGROUND ) ; // all hail to the C-Base!
        const ThemeColor& c_info = theme->get_color( THEME_COLOR_INFO_AREA ) ; 

        cairo->set_operator( Cairo::OPERATOR_OVER ) ;
        cairo->set_source_rgba(
              c_info.r 
            , c_info.g
            , c_info.b
            , c_info.a 
        ) ;
        RoundedRectangle(
              cairo
            , r.x 
            , r.y
            , r.width 
            , r.height 
            , 4. 
        ) ;
        cairo->fill () ;

        Gdk::Color cgdk ;
        cgdk.set_rgb_p( 1., 1., 1. ) ; 

        Cairo::RefPtr<Cairo::LinearGradient> gradient = Cairo::LinearGradient::create(
              (r.x + r.width) / 2
            , r.y  
            , (r.x + r.width) / 2
            , r.y + r.height
        ) ;

        double h, s, b ;
    
        double alpha = 0.15 ;
        
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
              0
            , c1.get_red_p()
            , c1.get_green_p()
            , c1.get_blue_p()
            , alpha 
        ) ;
        gradient->add_color_stop_rgba(
              .40
            , c2.get_red_p()
            , c2.get_green_p()
            , c2.get_blue_p()
            , alpha / 2.5
        ) ;
        gradient->add_color_stop_rgba(
              1. 
            , c3.get_red_p()
            , c3.get_green_p()
            , c3.get_blue_p()
            , alpha / 3.5
        ) ;
        cairo->set_source( gradient ) ;
        cairo->set_operator( Cairo::OPERATOR_OVER ) ;
        RoundedRectangle(
              cairo
            , r.x 
            , r.y 
            , r.width 
            , r.height 
            , 4. 
        ) ;
        cairo->fill(); 

        ThemeColor c ;

        c.r = cgdk.get_red_p() ;
        c.g = cgdk.get_green_p() ;
        c.b = cgdk.get_blue_p() ;

        gradient = Cairo::LinearGradient::create(
              (r.x + r.width) / 2
            , r.y  
            , (r.x + r.width) / 2
            , r.y + (r.height / 1.7)
        ) ;
        gradient->add_color_stop_rgba(
              0
            , c.r
            , c.g
            , c.b
            , alpha / 2.4 
        ) ;
        gradient->add_color_stop_rgba(
              0.5
            , c.r
            , c.g
            , c.b
            , alpha / 4.3 
        ) ;
        gradient->add_color_stop_rgba(
              1 
            , c.r
            , c.g
            , c.b
            , alpha / 6.5 
        ) ;
        cairo->set_source( gradient ) ;
        cairo->set_operator( Cairo::OPERATOR_OVER ) ;
        RoundedRectangle(
              cairo
            , r.x 
            , r.y
            , r.width
            , r.height / 1.7 
            , 4.
            , CairoCorners::CORNERS( 3 )
        ) ;
        cairo->fill() ;
*/

        return true ;
    }
}
