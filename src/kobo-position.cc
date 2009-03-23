#include <gtkmm.h>
#include "kobo-position.hh"

namespace MPX
{
    KoboPosition::KoboPosition ()
    : m_percent( 0 )
    {
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
    KoboPosition::set_percent(
        double p
    )
    {
        m_percent = p ;
        queue_draw () ;
    }

    bool
    KoboPosition::on_expose_event(
        GdkEventExpose* G_GNUC_UNUSED
    )
    {
        Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context() ;

        const Gdk::Rectangle& a = get_allocation() ;

        cairo->rectangle(
              0
            , 0
            , get_allocation().get_width()
            , get_allocation().get_height()
        ) ;
        cairo->clip () ;

        cairo->set_operator( Cairo::OPERATOR_SOURCE ) ;
        cairo->set_source_rgba( 0.12, 0.12, 0.12, 1. ) ;
        cairo->rectangle(
              0 
            , 0 
            , a.get_width()
            , 20
        ) ;
        cairo->fill () ;

        cairo->set_operator( Cairo::OPERATOR_ATOP ) ;
        cairo->set_source_rgba( 1., 1., 1., 1. ) ;
        cairo->set_line_width( 0.5 ) ;

        cairo->rectangle(
              2 
            , 5 
            , a.get_width() - 4
            , 10
        ) ;

        cairo->stroke () ;
   
        cairo->rectangle(
              4 
            , 7 
            , double((a.get_width() - 8)) * double(m_percent)
            , 6
        ) ;

        cairo->fill () ;

        return true ;
    }
}
