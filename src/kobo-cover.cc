#include "config.h"
#include <gtkmm.h>
#include "kobo-cover.hh"
#include "mpx/widgets/cairo-extensions.hh"

namespace MPX
{
    KoboCover::KoboCover ()
    {
        m_jewelcase_bot = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "jewelcase_bot.png" ))->scale_simple( 289, 262, Gdk::INTERP_BILINEAR) ;
        m_jewelcase_top = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "jewelcase_top.png" ))->scale_simple( 253, 255, Gdk::INTERP_BILINEAR) ;
    }
    
    KoboCover::~KoboCover ()
    {
    }

    void
    KoboCover::set(
        Glib::RefPtr<Gdk::Pixbuf> cover
    )
    {
        const Gtk::Allocation& a = get_allocation() ;

        if( cover )
            m_cover = cover->scale_simple( 96, 96 , Gdk::INTERP_BILINEAR ) ;
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

/*
        cairo->set_operator( Cairo::OPERATOR_SOURCE ) ; 
        Gdk::Cairo::set_source_pixbuf(    
              cairo
            , m_jewelcase_bot
            , a.get_width()/2 - 289/2
            , a.get_height() - 262 - 4
        ) ;  
        cairo->rectangle(
              a.get_width()/2 - 289/2
            , a.get_height() - 262 - 4
            , m_jewelcase_bot->get_width()
            , m_jewelcase_bot->get_height()
        ) ;
        cairo->fill () ;

        cairo->set_operator( Cairo::OPERATOR_OVER ) ; 
        Gdk::Cairo::set_source_pixbuf(    
              cairo
            , m_cover
            , a.get_width()/2 - 253/2 + 14.24
            , a.get_height() - 259 - 4
        ) ;  
        cairo->rectangle(
              a.get_width()/2 - 253/2 + 14.24
            , a.get_height() - 259 - 4
            , m_cover->get_width()
            , m_cover->get_height()
        ) ;
        cairo->fill () ;

        Gdk::Cairo::set_source_pixbuf(    
              cairo
            , m_jewelcase_top
            , a.get_width()/2 - 253/2 + 14.24
            , a.get_height() - 259 - 4
        ) ;  
        cairo->rectangle(
              a.get_width()/2 - 253/2 + 14.24
            , a.get_height() - 259 - 4
            , m_jewelcase_top->get_width()
            , m_jewelcase_top->get_height()
        ) ;
        cairo->fill () ;
*/

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
        cairo->fill() ;

/*
        cairo->set_source_rgba( 0., 0., 0., 1. ) ;
        cairo->set_line_width( 0.5 ) ;
        cairo->stroke() ;
*/

        return true ;
    }
}
