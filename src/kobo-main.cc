#include "config.h"

#include <gtkmm.h>
#include "mpx/mpx-main.hh"
#include "mpx/widgets/cairo-extensions.hh"

#include "mpx/i-youki-theme-engine.hh"

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
        mcs->key_set<int>("mpx", "window-w", m_presize_width ) ;
    }

    MainWindow::MainWindow ()

        : m_drawer_out( false )
        , m_presize_width( 0 )
        , m_drawer_width( 0 )
        , m_bottom_pad( 0 )
        , m_quit_clicked( false )
        , m_maximized( false )
        , m_drawer_width_max( 500 )
        , m_expand_direction( EXPAND_NONE )

    {
        set_title( "YOUKI player" ) ;
        set_decorated( false ) ;
        set_colormap(Glib::wrap(gdk_screen_get_rgba_colormap(gdk_screen_get_default()), true)) ; 

        add_events( Gdk::BUTTON_PRESS_MASK ) ;

        m_title_logo      = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "title-logo.png" )) ;
        m_button_quit     = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "mainwindow-button-quit.png" )) ;
        m_button_maximize = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "mainwindow-button-maximize.png" )) ;
        m_button_minimize = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "mainwindow-button-minimize.png" )) ;

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

        gtk_widget_realize( GTK_WIDGET( gobj() )) ;

        while (gtk_events_pending())
          gtk_main_iteration();

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

                        Gdk::Rectangle r1, r2, r3, r4 ;

                        r1.set_x( event->x ) ;
                        r1.set_y( event->y ) ;
                        r1.set_width( 1 ) ;
                        r1.set_height( 1 ) ;

                        // quit
                        r2.set_x( m_presize_width - 12 - 3 ) ;
                        r2.set_y( 4 ) ; 
                        r2.set_width( 12 ) ; 
                        r2.set_height( 12 ) ; 

                        // maximize
                        r3.set_x( m_presize_width - 27 - 3 ) ; 
                        r3.set_y( 4 ) ; 
                        r3.set_width( 12 ) ; 
                        r3.set_height( 12 ) ; 

                        // minimize
                        r4.set_x( m_presize_width - 42 - 3 ) ; 
                        r4.set_y( 4 ) ; 
                        r4.set_width( 12 ) ; 
                        r4.set_height( 12 ) ; 

                        bool intersect = false ;

                        r2.intersect( r1, intersect ) ;
                        if( intersect )
                        {
                            SignalQuit.emit() ; 
                            return true ;
                        }

                        r3.intersect( r1, intersect ) ;
                        if( intersect )
                        {
                            if( m_maximized )
                                unmaximize() ;
                            else
                                maximize() ;

                            return true ;
                        }

                        r4.intersect( r1, intersect ) ;
                        if( intersect )
                        {
                            iconify() ;
                            return true ;
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
                        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

                        Cairo::RefPtr<Cairo::Context> cr = get_window()->create_cairo_context() ;

                        cr->set_operator( Cairo::OPERATOR_CLEAR ) ;
                        cr->paint () ;

                        //// DRAWER 
                        if( m_expand_direction != EXPAND_NONE || m_drawer_out )
                        {
                                const ThemeColor& c = theme->get_color( THEME_COLOR_DRAWER ) ;

                                cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
                                cr->set_source_rgba(
                                      c.r
                                    , c.g
                                    , c.b
                                    , c.a
                                ) ;
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
                        const ThemeColor& c1 = theme->get_color( THEME_COLOR_BACKGROUND ) ;

                        cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
                        cr->set_source_rgba(
                              c1.r
                            , c1.g
                            , c1.b
                            , c1.a
                        ) ;
                        RoundedRectangle(
                              cr
                            , 0
                            , 0
                            , m_presize_width 
                            , get_allocation().get_height()
                            , rounding
                        ) ;
                        cr->fill_preserve() ;
                        cr->clip() ;

                        //// TITLEBAR 
                        const ThemeColor& t1 = theme->get_color( THEME_COLOR_TITLEBAR_1 ) ;
                        const ThemeColor& t2 = theme->get_color( THEME_COLOR_TITLEBAR_2 ) ;
                        const ThemeColor& t3 = theme->get_color( THEME_COLOR_TITLEBAR_3 ) ;

                        Cairo::RefPtr<Cairo::LinearGradient> background_gradient_ptr = Cairo::LinearGradient::create(
                              m_presize_width / 2. 
                            , 0
                            , m_presize_width / 2. 
                            , 20 
                        ) ;
                        
                        background_gradient_ptr->add_color_stop_rgba(
                              0
                            , t1.r 
                            , t1.g
                            , t1.b
                            , t1.a 
                        ) ;
                        
                        background_gradient_ptr->add_color_stop_rgba(
                              .85                       
                            , t2.r 
                            , t2.g
                            , t2.b
                            , t2.a 
                        ) ;
                        
                        background_gradient_ptr->add_color_stop_rgba(
                              1 
                            , t3.r 
                            , t3.g
                            , t3.b
                            , t3.a 
                        ) ;

                        cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
                        cr->set_source( background_gradient_ptr ) ;
                        RoundedRectangle(
                              cr
                            , 0
                            , 0
                            , m_presize_width 
                            , 20
                            , rounding
                            , CairoCorners::CORNERS( CairoCorners::TOPLEFT | CairoCorners::TOPRIGHT )
                        ) ;
                        cr->fill();

                        //// ICONS
                        cr->set_operator( Cairo::OPERATOR_OVER ) ;

                        GdkRectangle r ;
                        r.x         = 8 ; 
                        r.y         = 5 ; 
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
                        cr->fill() ; 

                        // off

                        r.y         =  4 ; 
                        r.width     = 12 ; 
                        r.height    = 12 ; 

                        r.x = m_presize_width - 12 - 3 ; 

                        Gdk::Cairo::set_source_pixbuf(
                              cr
                            , m_button_quit 
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

                        // maximize

                        r.x = m_presize_width - 27 - 3 ;
                        Gdk::Cairo::set_source_pixbuf(
                              cr
                            , m_button_maximize 
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
                        cr->paint_with_alpha( m_maximized ? .9 : .6 ) ;
                        cr->restore () ;

                        cr->set_operator( Cairo::OPERATOR_OVER ) ;

                        // minimize

                        r.x = m_presize_width - 42 - 3 ;
                        Gdk::Cairo::set_source_pixbuf(
                              cr
                            , m_button_minimize 
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
                        cr->paint_with_alpha( .6 ) ; 
                        cr->restore () ;

                        //// RESIZE GRIP 

                        const ThemeColor& crg = theme->get_color( THEME_COLOR_RESIZE_GRIP ) ;

                        cr->set_source_rgba(
                              crg.r
                            , crg.g
                            , crg.b
                            , crg.a
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

                        cr->set_operator( Cairo::OPERATOR_OVER ) ;

                        //// MAINAREA BORDER 
                        const ThemeColor& brd = theme->get_color( THEME_COLOR_WINDOW_BORDER ) ;

                        RoundedRectangle(
                              cr
                            , 0
                            , 0
                            , m_presize_width 
                            , get_allocation().get_height()
                            , rounding
                        ) ;
                        cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
                        cr->set_source_rgba(
                              brd.r
                            , brd.g
                            , brd.b
                            , brd.a
                        ) ;
                        cr->set_line_width( 1.5 ) ;
                        cr->stroke() ;

                        //// TITLEBAR TOP
                        const ThemeColor& tbt = theme->get_color( THEME_COLOR_TITLEBAR_TOP ) ;

                        cr->save() ;

                        cr->rectangle( 0, 0, m_presize_width, 20 ) ;
                        cr->clip() ;

                        RoundedRectangle(
                              cr
                            , 0
                            , 0
                            , m_presize_width 
                            , 20 + rounding
                            , rounding
                        ) ;
                        cr->set_source_rgba(
                              tbt.r
                            , tbt.g
                            , tbt.b
                            , tbt.a
                        ) ;
                        cr->set_line_width( 1.5 ) ;
                        cr->stroke() ;
                        cr->restore() ;

                        return true ;
                    }
    bool
    MainWindow::on_window_state_event( GdkEventWindowState* event )
                    {
                        m_maximized = ( event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED ) ;
                        queue_draw () ;

                        return false ;
                    }
}
