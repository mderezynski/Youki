#include "config.h"

#include <gtkmm.h>
#include "mpx/widgets/cairo-extensions.hh"
#include "kobo-main.hh"

namespace
{
    const double rounding = 8. ;
}

namespace
{
    void RoundedTriangle (Cairo::RefPtr<Cairo::Context> cr, double x, double y, double w, double h,
        double r)
    {
        cr->move_to(x, y);
        cr->line_to(x + w, y - h);
        cr->line_to(x + w, y);
        cr->arc(x + w - r, y - r, r, 0, M_PI * 0.5);
        cr->line_to(x, y);
    }
}

namespace MPX
{
    MainWindow::MainWindow ()
    : m_drawer_out( false )
    , m_presize_height( 0 )
    , m_drawer_height( 0 )
    , m_drawer_height_max( 300 )
    , m_expand_direction( EXPAND_NONE )
                    {
                        set_title( "Youki" ) ;
                        set_decorated( false ) ;
                        set_colormap(Glib::wrap(gdk_screen_get_rgba_colormap(gdk_screen_get_default()), true)) ; 

                        add_events( Gdk::BUTTON_PRESS_MASK ) ;
                        m_title_logo = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "title-logo.png" )) ;

                        set_geom_hints( false ) ;

                        Glib::signal_timeout().connect(
                              sigc::mem_fun(
                                    *this
                                  , &MainWindow::drawer_extend_timeout
                              )
                            , 20
                        ) ;

                        m_presize_height = get_allocation().get_height() ;

                        a1 = Gtk::manage( new Gtk::Alignment  ) ;
                        a2 = Gtk::manage( new Gtk::Alignment  ) ;

                        a2->property_yalign() = 0. ;

                        a1->property_top_padding() = 20 ;
                        a1->property_bottom_padding() = 16 ;
                        a2->property_bottom_padding() = 12 ;

                        a1->set_border_width( 8 ) ;
                        a2->set_border_width( 8 ) ;

                        v = Gtk::manage( new Gtk::VBox ) ;
                        v->pack_start( *a1, 0, 0, 0 ) ;
                        v->pack_start( *a2, 0, 0, 0 ) ;

                        a1->set_size_request( -1, 400 ) ;
                        a2->set_size_request( -1, 0 ) ;

                        add( *v ) ;
                    }

    void
    MainWindow::set_widget_top( Gtk::Widget & w )
                    {
                        a1->add( w ) ;
                    }

    void
    MainWindow::set_widget_drawer( Gtk::Widget & w )
                    {
                        a2->add( w ) ;
                    }

    void
    MainWindow::set_geom_hints(
                        bool with_drawer_height
                    )
                    {
                        Gdk::Geometry geom ;

                        geom.min_height = 300 + ( with_drawer_height ? m_drawer_height_max : 0 ) ;
                        geom.min_width = 524 ;

                        set_geometry_hints(
                              *this
                            , geom
                            , Gdk::HINT_MIN_SIZE
                        ) ;
                    }

    bool
    MainWindow::drawer_extend_timeout () 
                    {
                        Glib::Mutex::Lock L (m_size_lock) ;

                        if( m_expand_direction == EXPAND_OUT )
                        {
                            m_drawer_height = std::min( m_drawer_height + 20, m_drawer_height_max ) ;

                            a1->set_size_request( -1, m_presize_height ) ;
                            a2->set_size_request( -1, m_drawer_height ) ;

                            if( m_drawer_height == m_drawer_height_max )
                            {
                                m_drawer_out = true ;
                            }

                            queue_draw () ;

                            m_expand_direction =
                                  m_drawer_height < m_drawer_height_max
                                ? EXPAND_OUT 
                                : EXPAND_NONE
                            ;
                        }
                        else
                        if( m_expand_direction == EXPAND_IN )
                        {
                            m_drawer_height = std::max( m_drawer_height - 20, 0 ) ;

                            a1->set_size_request( -1, m_presize_height ) ;
                            a2->set_size_request( -1, m_drawer_height ) ;

                            if( m_drawer_height == 0 )
                            {
                                m_drawer_out = false ;

                                resize( get_allocation().get_width(), m_presize_height ) ;
                                set_geom_hints( false ) ;
                            }

                            queue_draw () ;

                            m_expand_direction =
                                  m_drawer_height > 0
                                ? EXPAND_IN
                                : EXPAND_NONE
                            ;
                        }

                        return true ;
                    }

    bool
    MainWindow::on_button_press_event( GdkEventButton* event )
                    {
                        if( event->y < 20 )
                        {
                            begin_move_drag(
                                  event->button
                                , event->x_root
                                , event->y_root
                                , event->time
                            ) ;
                            return false ;
                        }
                        else
                        if( (event->y > (m_presize_height - 16)) && (event->y < m_presize_height) && (event->x > (get_allocation().get_width() - 16)) )
                        {
                            begin_resize_drag(
                                  Gdk::WINDOW_EDGE_SOUTH_EAST
                                , event->button
                                , event->x_root
                                , event->y_root
                                , event->time
                            ) ;

                            return false ;
                        }
                        else
                        if( (event->y <= m_presize_height) && (event->y > m_presize_height-16) )
                        {
                            if( m_drawer_out )
                            {
                                m_expand_direction = EXPAND_IN ;
                            }
                            else
                            if( m_expand_direction == EXPAND_NONE )
                            {
                                m_presize_height = get_allocation().get_height() - m_drawer_height ;

                                resize( get_allocation().get_width(), m_presize_height + m_drawer_height_max ) ; 

                                set_geom_hints( true ) ;

                                m_expand_direction = EXPAND_OUT ;
                            }
                        }

                        return false ;
                    }

    void
    MainWindow::on_size_allocate( Gtk::Allocation & a )
                    {
                        Glib::Mutex::Lock L (m_size_lock) ;

                        if( m_expand_direction != EXPAND_NONE || m_drawer_out )
                            m_presize_height = a.get_height() - m_drawer_height_max ; 
                        else
                            m_presize_height = a.get_height() ; 

                        Gtk::Widget::on_size_allocate( a ) ;

                        a1->set_size_request( a.get_width(), m_presize_height ) ;

                        queue_draw() ;
                        gdk_flush () ;
                    }

    bool
    MainWindow::on_expose_event( GdkEventExpose* event )
                    {
                        Cairo::RefPtr<Cairo::Context> cr = get_window()->create_cairo_context() ;

                        cr->set_operator( Cairo::OPERATOR_CLEAR ) ;
                        cr->paint () ;

                        if( m_expand_direction != EXPAND_NONE || m_drawer_out )
                        {
                                cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
                                cr->set_source_rgba( 0.65, 0.65, 0.65, .4 ) ;
                            
                                RoundedRectangle(
                                      cr
                                    , 0 
                                    , m_presize_height - 16
                                    , get_allocation().get_width()
                                    , m_drawer_height + 16 
                                    , rounding
                                    , CairoCorners::CORNERS( CairoCorners::BOTTOMLEFT | CairoCorners::BOTTOMRIGHT )
                                ) ;

                                cr->fill () ;
                        }

                        cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
                        cr->set_source_rgba( 0.12, 0.12, 0.12, 1. ) ;
                    
                        RoundedRectangle(
                              cr
                            , 0
                            , 0
                            , get_allocation().get_width()
                            , get_allocation().get_height() - ( ( m_expand_direction != EXPAND_NONE || m_drawer_out ) ? m_drawer_height_max : 0 )
                            , rounding
                        ) ;
                        cr->fill_preserve() ;
                        cr->set_source_rgba( 0.22, 0.22, 0.22, 1. ) ;
                        cr->set_line_width( 1. ) ;
                        cr->stroke() ;

                        // Create the linear gradient diagonal
                        Cairo::RefPtr<Cairo::LinearGradient> background_gradient_ptr = Cairo::LinearGradient::create(
                              get_allocation().get_width() / 2. 
                            , 0
                            , get_allocation().get_width() / 2.
                            , 20 
                        ) ;
                        
                        // Set grandient colors
                        background_gradient_ptr->add_color_stop_rgba(
                              0
                            , 1.
                            , 1.
                            , 1.
                            , 0.6 
                        ) ;
                        
                        background_gradient_ptr->add_color_stop_rgba(
                              .9                       
                            , 1.
                            , 1.
                            , 1.
                            , 0.2 
                        ) ;
                        
                        // Set grandient colors
                        background_gradient_ptr->add_color_stop_rgba(
                              1 
                            , 1.
                            , 1.
                            , 1.
                            , 0.15 
                        ) ;

                        // Draw a rectangle and fill with gradient
                        cr->set_operator( Cairo::OPERATOR_ATOP ) ;
                        cr->set_source( background_gradient_ptr ) ;
                        cr->rectangle(
                              0
                            , 0
                            , get_allocation().get_width()
                            , 20
                        ) ;
                        cr->fill();


                        Gdk::Cairo::set_source_pixbuf(
                              cr
                            , m_title_logo 
                            , 12
                            , (20 / 2) - (m_title_logo->get_height()/2) 
                        ) ;

                        cr->rectangle(
                              12
                            , (20 / 2) - (m_title_logo->get_height()/2) 
                            , m_title_logo->get_width()
                            , m_title_logo->get_height()
                        ) ;

                        cr->fill () ;

                        cr->set_source_rgba(
                              1.
                            , 1.
                            , 1.
                            , 0.4
                        ) ;

                        RoundedTriangle(
                              cr
                            , get_allocation().get_width() - 16
                            , m_presize_height
                            , 16
                            , 16
                            , rounding
                        ) ;

                        cr->fill () ;

                        if( get_child() )
                        {
                            propagate_expose( *get_child(), event ) ;
                        }

                        return false ;
                    }
}
