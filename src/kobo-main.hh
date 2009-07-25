#ifndef KOBO_MAIN_WINDOW_HH
#define KOBO_MAIN_WINDOW_HH

#include <gtkmm.h>
#include <boost/ref.hpp>

namespace MPX
{
        class MainWindow
        : public Gtk::Window
        {
            protected:

                    Glib::RefPtr<Gdk::Pixbuf>   m_title_logo ;

                    Glib::RefPtr<Gdk::Pixbuf>   m_button_quit ;
                    Glib::RefPtr<Gdk::Pixbuf>   m_button_maximize ;
                    Glib::RefPtr<Gdk::Pixbuf>   m_button_minimize ;

                    bool                        m_drawer_out ;
                    int                         m_presize_width ;
                    int                         m_drawer_width ;
                    int                         m_bottom_pad ;
                    bool                        m_quit_clicked ;
                    bool                        m_maximized ;
                    bool                        m_composited ;
                    bool                        m_size_changed ;

                    enum ExpandDirection
                    {
                          EXPAND_IN
                        , EXPAND_OUT
                        , EXPAND_NONE
                    } ; 

                    const int                   m_drawer_width_max ;

                    ExpandDirection             m_expand_direction ;        

                    Glib::Mutex                 m_size_lock ;

                    Gtk::Alignment            * a1
                                            , * a2 ; 

                    Gtk::HBox                 * v ;

                    sigc::signal<void>          SignalQuit ;

            public:

                    MainWindow () ;
                    virtual ~MainWindow () ;

                    sigc::signal<void>&
                    signal_quit()
                    {
                        return SignalQuit ;
                    }

                    void
                    set_widget_top( Gtk::Widget & w ) ;

                    void
                    set_widget_drawer( Gtk::Widget & w ) ;

                    void
                    clear_widget_top() ;

                    void
                    clear_widget_drawer() ;

                    void
                    quit()
                    {
                        m_quit_clicked = true ;
                        queue_draw () ;
                    }

            protected:
            
                    void
                    set_geom_hints(
                        bool with_drawer_height
                    ) ;

                    bool
                    drawer_extend_timeout () ;

            protected:

                    bool
                    on_button_press_event(
                          GdkEventButton*
                    ) ;

                    void
                    on_size_allocate(
                          Gtk::Allocation&
                    ) ;

                    bool
                    on_expose_event(
                          GdkEventExpose*
                    ) ;

                    bool
                    on_window_state_event(
                          GdkEventWindowState*
                    ) ;
        } ;
}

#endif // KOBO_MAIN_WINDOW_HH
