#include "config.h"

#include <gtkmm.h>
#include <gtk/gtkscrolledwindow.h>
#include "mpx/widgets/rounded-alignment.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/mpx-main.hh"
#include "mpx/i-youki-theme-engine.hh"

namespace MPX
{
    RoundedScrolledWindow::RoundedScrolledWindow(
    )
    {
        set_border_width( 4 ) ;
    }

    RoundedScrolledWindow::~RoundedScrolledWindow()
    {
    }

    void
    RoundedScrolledWindow::on_size_allocate(
          Gtk::Allocation&  alloc
    ) 
    {
        Gtk::ScrolledWindow::on_size_allocate( alloc ) ;

        const Gdk::Rectangle& a = get_allocation();

        m_gradient =
             Cairo::LinearGradient::create(
                  a.get_width() / 2. 
                , 0
                , a.get_width() / 2. 
                , a.get_height() 
        ) ;
        
        m_gradient->add_color_stop_rgba(
              0
            , 0.4 
            , 0.4 
            , 0.4 
            , .8 
        ) ;
        
        m_gradient->add_color_stop_rgba(
              .5                       
            , 0.43 
            , 0.43 
            , 0.43 
            , .8 
        ) ;
        
        m_gradient->add_color_stop_rgba(
              .85                     
            , 0.44 
            , 0.44 
            , 0.44 
            , .8 
        ) ;

        m_gradient->add_color_stop_rgba(
              1              
            , .85 
            , .85 
            , .85 
            , .8 
        ) ;
    } 

    bool
    RoundedScrolledWindow::on_expose_event(
            GdkEventExpose* event
    )
    {
        Cairo::RefPtr<Cairo::Context> cairo = get_window()->create_cairo_context();

        const Gdk::Rectangle& a = get_allocation();

        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

        const ThemeColor& c_bg   = theme->get_color( THEME_COLOR_BACKGROUND ) ;
        const ThemeColor& c_base = theme->get_color( THEME_COLOR_BASE ) ;

        GtkWidget * h = gobj()->GSEAL(hscrollbar) ;
        std::size_t w = a.get_width() ; 

        if( h )
        {
            w -= h->GSEAL(allocation).width ;
        }

        cairo->set_operator(
            Cairo::OPERATOR_SOURCE
        ) ;

        cairo->set_source_rgba(
              c_bg.r
            , c_bg.g
            , c_bg.b
            , c_bg.a
        ) ;
        cairo->rectangle(
              a.get_x()
            , a.get_y()
            , w 
            , a.get_height()
        ) ;
        cairo->fill() ;

        cairo->set_operator(
            Cairo::OPERATOR_ATOP
        ) ;

        cairo->set_source_rgba(
              c_base.r
            , c_base.g
            , c_base.b
            , c_base.a
        ) ;
        RoundedRectangle(
              cairo
            , 1 + a.get_x()
            , 1 + a.get_y()
            , w - 2
            , a.get_height() - 2
            , 4.
        ) ;
        cairo->fill() ;

        Gtk::ScrolledWindow::on_expose_event( event ) ;

/*
        cairo->set_operator( Cairo::OPERATOR_OVER ) ;
        cairo->set_source( m_gradient ) ;
        cairo->set_line_width( 2 ) ;
        RoundedRectangle(
              cairo
            , 1 + a.get_x()
            , 1 + a.get_y()
            , w - 2
            , a.get_height() - 2
            , 4.
        ) ;
        cairo->stroke();
*/

        return true;
    }
}


