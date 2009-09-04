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
            m_cover = cover->scale_simple( 50, 50 , Gdk::INTERP_HYPER ) ;
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

        cairo->set_operator( Cairo::OPERATOR_SOURCE ) ; 

        cairo->set_source_rgba(
              m_CurrentColor.get_red_p()
            , m_CurrentColor.get_green_p()
            , m_CurrentColor.get_blue_p()
            , 1. 
        ) ;

        cairo->set_line_width( 1.5 ) ;
        cairo->stroke() ;

        return true ;
    }
}
