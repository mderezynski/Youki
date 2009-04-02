#include "config.h"

#include <gtkmm.h>
#include "mpx/mpx-main.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "kobo-main.hh"

namespace
{
    const double rounding = 4. ;
}

namespace
{
    void RoundedTriangleRight (Cairo::RefPtr<Cairo::Context> cr, double x, double y, double w, double h,
        double r)
    {
        cr->move_to(x, y);
        cr->line_to(x + w, y - h);
        cr->line_to(x + w, y);
        cr->arc(x + w, y, r, M_PI * 0.5, M_PI );
        cr->line_to(x, y);
    }

    void RoundedTriangleLeft (Cairo::RefPtr<Cairo::Context> cr, double x, double y, double w, double h,
        double r)
    {
        cr->move_to(x, y);
        cr->line_to(x - w, y - h);
        cr->line_to(x - w, y);
        cr->arc(x - w, y, r, 0, M_PI * 0.5);
        cr->line_to(x, y);
    }
}

namespace MPX
{
    MainWindow::~MainWindow ()
    {
        get_position( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-x")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-y")));
        get_size( m_presize_width, Mcs::Key::adaptor<int>(mcs->key("mpx", "window-h")) );
    }

    MainWindow::MainWindow ()

        : m_drawer_out( false )
        , m_presize_width( 0 )
        , m_drawer_width( 0 )
        , m_bottom_pad( 0 )
        , m_quit_clicked( false )
        , m_drawer_width_max( 500 )
        , m_expand_direction( EXPAND_NONE )

                     {
                        set_title( "Youki" ) ;
                        set_decorated( false ) ;
                        set_colormap(Glib::wrap(gdk_screen_get_rgba_colormap(gdk_screen_get_default()), true)) ; 

                        add_events( Gdk::BUTTON_PRESS_MASK ) ;

                        m_title_logo = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "title-logo.png" )) ;
                        m_button_off = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "off.png" )) ;

                        set_geom_hints( false ) ;

                        Glib::signal_timeout().connect(
                              sigc::mem_fun(
                                    *this
                                  , &MainWindow::drawer_extend_timeout
                              )
                            , 20
                        ) ;

                        m_presize_width = get_allocation().get_width() ;

                        a1 = Gtk::manage( new Gtk::Alignment  ) ;
                        a2 = Gtk::manage( new Gtk::Alignment  ) ;

                        //a2->property_yalign() = 0. ;

                        a1->property_top_padding() = 20 ;
                        a1->property_bottom_padding() = 16 ;

                        //a2->property_bottom_padding() = 12 ;

                        a1->set_border_width( 8 ) ;
                        a2->set_border_width( 8 ) ;

                        v = Gtk::manage( new Gtk::HBox ) ;
                        v->pack_start( *a1, 0, 0, 0 ) ;
                        v->pack_start( *a2, 0, 0, 0 ) ;

                        a1->set_size_request( -1, 400 ) ;
                        a2->set_size_request( -1, 0 ) ;

                        add( *v ) ;

                        resize(
                              mcs->key_get<int>("mpx","window-w"),
                              mcs->key_get<int>("mpx","window-h")
                        );

                        move(
                              mcs->key_get<int>("mpx","window-x"),
                              mcs->key_get<int>("mpx","window-y")
                        );


                        while (gtk_events_pending())
                          gtk_main_iteration();
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
    MainWindow::clear_widget_top()
                    {
                        a1->remove() ;
                    }

    void
    MainWindow::clear_widget_drawer()
                    {
                        a2->remove() ;
                    }

    void
    MainWindow::set_geom_hints(
                        bool with_drawer_height
                    )
                    {
                        Gdk::Geometry geom ;

                        geom.min_height = 300 ; 
                        geom.min_width = 524 + ( with_drawer_height ? m_drawer_width_max : 0 ) ;

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
                            m_drawer_width = std::min( m_drawer_width + 20, m_drawer_width_max ) ;

                            a1->set_size_request( m_presize_width, -1 ) ;
                            a2->set_size_request( m_drawer_width, -1 ) ;

                            if( m_drawer_width == m_drawer_width_max )
                            {
                                m_drawer_out = true ;
                            }

                            queue_draw () ;

                            m_expand_direction =
                                  m_drawer_width < m_drawer_width_max
                                ? EXPAND_OUT 
                                : EXPAND_NONE
                            ;
                        }
                        else
                        if( m_expand_direction == EXPAND_IN )
                        {
                            m_drawer_width = std::max( m_drawer_width - 20, 0 ) ;

                            a1->set_size_request( m_presize_width, -1 ) ;
                            a2->set_size_request( m_drawer_width, -1 ) ;

                            if( m_drawer_width == 0 )
                            {
                                a2->hide() ;
                                m_drawer_out = false ;
                                resize( m_presize_width, get_allocation().get_height() ) ;
                                set_geom_hints( false ) ;
                            }

                            queue_draw () ;

                            m_expand_direction =
                                  m_drawer_width > 0
                                ? EXPAND_IN
                                : EXPAND_NONE
                            ;
                        }

                        return true ;
                    }

    bool
    MainWindow::on_button_press_event( GdkEventButton* event )
                    {
                        const Gtk::Allocation& a = get_allocation() ;

                        if( event->window != GTK_WIDGET(gobj())->window )
                        {
                            return false ;
                        }

                        Gdk::Rectangle r1, r2 ;

                        r1.set_x( event->x ) ;
                        r1.set_y( event->y ) ;
                        r1.set_width( 1 ) ;
                        r1.set_height( 1 ) ;

                        r2.set_x( 8 ) ;
                        r2.set_y( (20 - m_button_off->get_height()) / 2 ) ;
                        r2.set_width( m_button_off->get_width() ) ;
                        r2.set_height( m_button_off->get_height() ) ;

                        bool intersect = false ;

                        r2.intersect( r1, intersect ) ;

                        if( intersect )
                        {
                            SignalQuit.emit() ; 
                            return false ;
                        }
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
                        if( (event->x > (m_presize_width - 16)) && (event->x < m_presize_width) && (event->y > (a.get_height() - 16)) )
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
                        if( (event->x < 16) && (event->y > a.get_height()-16) ) 
                        {
                            begin_resize_drag(
                                  Gdk::WINDOW_EDGE_SOUTH_WEST
                                , event->button
                                , event->x_root
                                , event->y_root
                                , event->time
                            ) ;

                            return false ;
                        }
                        else
                        if( (event->y <= a.get_height()) && (event->y > a.get_height()-16) )
                        {
                            if( m_drawer_out )
                            {
                                m_expand_direction = EXPAND_IN ;
                            }
                            else
                            if( m_expand_direction == EXPAND_NONE )
                            {
                                m_presize_width = a.get_width() - m_drawer_width ;
                                m_expand_direction = EXPAND_OUT ;
                                a2->show() ;
                                resize( m_presize_width + m_drawer_width_max, a.get_height() ) ; 
                                set_geom_hints( true ) ;
                            }
                        }

                        return false ;
                    }

    void
    MainWindow::on_size_allocate( Gtk::Allocation & a )
                    {
                        Glib::Mutex::Lock L (m_size_lock) ;

                        if( m_expand_direction != EXPAND_NONE || m_drawer_out )
                            m_presize_width = a.get_width() - m_drawer_width_max ; 
                        else
                            m_presize_width = a.get_width() ; 

                        Gtk::Widget::on_size_allocate( a ) ;

                        a1->set_size_request( m_presize_width, a.get_height() ) ;

                        queue_draw() ;
                        gdk_flush () ;
                    }

    bool
    MainWindow::on_expose_event( GdkEventExpose* event )
                    {
                        Cairo::RefPtr<Cairo::Context> cr = get_window()->create_cairo_context() ;

                        cr->set_operator( Cairo::OPERATOR_CLEAR ) ;
                        cr->paint () ;

                        //// DRAWER 
                        if( m_expand_direction != EXPAND_NONE || m_drawer_out )
                        {
                                cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
                                cr->set_source_rgba( 0.65, 0.65, 0.65, .4 ) ;
                                RoundedRectangle(
                                      cr
                                    , m_presize_width - 16 
                                    , 0 
                                    , m_drawer_width + 16 
                                    , get_allocation().get_height() 
                                    , rounding
                                    , CairoCorners::CORNERS( CairoCorners::TOPRIGHT | CairoCorners::BOTTOMRIGHT )
                                ) ;
                                cr->fill () ;
                        }

                        //// MAINAREA 
                        cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
                        cr->set_source_rgba( 0.10, 0.10, 0.10, 1. ) ;
                        RoundedRectangle(
                              cr
                            , 0
                            , 0
                            , get_allocation().get_width() - ( ( m_expand_direction != EXPAND_NONE || m_drawer_out ) ? m_drawer_width_max : 0 )
                            , get_allocation().get_height()
                            , rounding
                        ) ;
                        cr->fill_preserve() ;
                        cr->set_source_rgba( 0.22, 0.22, 0.22, 1. ) ;
                        cr->set_line_width( 1. ) ;
                        cr->stroke() ;

                        //// TITLEBAR 
                        Cairo::RefPtr<Cairo::LinearGradient> background_gradient_ptr = Cairo::LinearGradient::create(
                              get_allocation().get_width() / 2. 
                            , 0
                            , get_allocation().get_width() / 2.
                            , 20 
                        ) ;
                        
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
                        
                        background_gradient_ptr->add_color_stop_rgba(
                              1 
                            , 1.
                            , 1.
                            , 1.
                            , 0.15 
                        ) ;

                        cr->set_operator( Cairo::OPERATOR_ATOP ) ;
                        cr->set_source( background_gradient_ptr ) ;
                        cr->rectangle(
                              0
                            , 0
                            , get_allocation().get_width()
                            , 20
                        ) ;
                        cr->fill();


                        //// ICONS

                        GdkRectangle r ;

                        r.x         = m_presize_width - m_title_logo->get_width() - 6 ;
                        r.y         = (20 / 2) - (m_title_logo->get_height()/2) ; 
                        r.width     = m_title_logo->get_width() ;
                        r.height    = m_title_logo->get_height() ;

                        Gdk::Cairo::set_source_pixbuf(
                              cr
                            , m_title_logo 
                            , r.x 
                            , r.y 
                        ) ;

                        cr->rectangle(
                              r.x 
                            , r.y 
                            , r.width 
                            , r.height 
                        ) ;

                        cr->fill () ;

                        r.x         = 8 ; 
                        r.y         = (20 - m_button_off->get_height()) / 2. ;
                        r.width     = m_button_off->get_width() ;
                        r.height    = m_button_off->get_height() ;

                        Gdk::Cairo::set_source_pixbuf(
                              cr
                            , m_button_off 
                            , r.x 
                            , r.y 
                        ) ;

                        cr->rectangle(
                              r.x 
                            , r.y 
                            , r.width 
                            , r.height 
                        ) ;

                        cr->save () ;
                        cr->clip () ;
                        cr->paint_with_alpha( m_quit_clicked ? .9 : .6 ) ; 
                        cr->restore () ;

                        //// RESIZE GRIP 
                        cr->set_source_rgba(
                              1.
                            , 1.
                            , 1.
                            , 0.4
                        ) ;

                        RoundedTriangleRight(
                              cr
                            , m_presize_width - 16
                            , get_allocation().get_height()
                            , 16
                            , 16
                            , rounding
                        ) ;

                        RoundedTriangleLeft(
                              cr
                            , 16 
                            , get_allocation().get_height()
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
