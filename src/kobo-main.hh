#ifndef KOBO_MAIN_WINDOW_HH
#define KOBO_MAIN_WINDOW_HH

#include <gtkmm.h>

namespace MPX
{
        class MainWindow
        : public Gtk::Window
        {
            protected:

                    Glib::RefPtr<Gdk::Pixbuf>   m_title_logo ;
                    Glib::RefPtr<Gdk::Pixbuf>   m_button_off ;

                    bool                        m_drawer_out ;
                    int                         m_presize_height ;
                    int                         m_drawer_height ;
                    int                         m_bottom_pad ;
                    bool                        m_quit_clicked ;

                    bool
                    quit_timeout () ;

                    enum ExpandDirection
                    {
                          EXPAND_IN
                        , EXPAND_OUT
                        , EXPAND_NONE
                    } ; 

                    const int                   m_drawer_height_max ;

                    ExpandDirection             m_expand_direction ;        

                    Glib::Mutex                 m_size_lock ;

                    Gtk::Alignment            * a1
                                            , * a2 ; 

                    Gtk::VBox                 * v ;

            public:

                    MainWindow () ;

                    virtual ~MainWindow () ;

                    void
                    set_widget_top( Gtk::Widget & w ) ;

                    void
                    set_widget_drawer( Gtk::Widget & w ) ;

            protected:
            
                    void
                    set_geom_hints(
                        bool with_drawer_height
                    ) ;

                    bool
                    drawer_extend_timeout () ;

            protected:

                    bool
                    on_button_press_event( GdkEventButton* event ) ;

                    bool
                    on_key_press_event( GdkEventKey* event ) ;

                    void
                    on_size_allocate( Gtk::Allocation & a ) ;

                    bool
                    on_expose_event( GdkEventExpose* event ) ;

                    void
                    initiate_quit() ;
        } ;
}

#endif // KOBO_MAIN_WINDOW_HH
