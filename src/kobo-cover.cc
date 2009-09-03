#include "config.h"
#include <gtkmm.h>
#include "kobo-cover.hh"
#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-main.hh"
#include "mpx/widgets/cairo-extensions.hh"

namespace MPX
{
    KoboCover::KoboCover ()
    {
        m_jewelcase_bot = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "jewelcase_bot.png" ))->scale_simple( 289, 262, Gdk::INTERP_BILINEAR) ;
        m_jewelcase_top = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "jewelcase_top.png" ))->scale_simple( 253, 255, Gdk::INTERP_BILINEAR) ;

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
            m_cover = cover->scale_simple( 94, 94 , Gdk::INTERP_BILINEAR ) ;
            Glib::RefPtr<Gdk::Pixbuf> tiny = cover->scale_simple( 1, 1 , Gdk::INTERP_NEAREST ) ;
            guchar * pixels = gdk_pixbuf_get_pixels( GDK_PIXBUF(tiny->gobj()) ) ;
            m_CurrentColor.set_rgb_p( double(pixels[0])/255., double(pixels[1])/255., double(pixels[2])/255. ) ;
        }
        else
            m_cover = cover ;

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

        Gdk::Cairo::set_source_pixbuf(    
              cairo
            , m_cover
            , (a.get_width() - m_cover->get_width())/2.
            , (a.get_height() - m_cover->get_height())/2.
        ) ;  
        RoundedRectangle(
              cairo
            , (a.get_width() - m_cover->get_width())/2.
            , (a.get_height() - m_cover->get_height())/2.
            , m_cover->get_width()
            , m_cover->get_height()
            , 4.
        ) ;
        cairo->fill_preserve() ;

        const ThemeColor& c = m_Theme->get_color( THEME_COLOR_SELECT ) ;

        cairo->set_operator( Cairo::OPERATOR_ATOP ) ; 

        cairo->set_source_rgba(
              m_CurrentColor.get_red_p()
            , m_CurrentColor.get_green_p()
            , m_CurrentColor.get_blue_p()
            , 0.4
        ) ;

        cairo->set_line_width( 1. ) ;
        cairo->stroke() ;

        return true ;
    }
}
