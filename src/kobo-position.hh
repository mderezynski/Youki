#ifndef KOBO_POSITION_HH
#define KOBO_POSITION_HH

#include <gtkmm.h>

namespace MPX
{
    class KoboPosition : public Gtk::DrawingArea
    {
        protected:
        
            gint64  m_duration ;        
            gint64  m_position ;
            gint64  m_seek_position ;
            bool    m_clicked ;
            double  m_seek_factor ;
        
            sigc::connection m_scrollback_conn ;
    
        public:

            typedef sigc::signal<void, gint64>      SignalSeekEvent ;

        protected:
    
            SignalSeekEvent    m_SIGNAL_seek_event ;

        public:

            SignalSeekEvent&
            signal_seek_event()
            {
                return m_SIGNAL_seek_event ;
            }

            KoboPosition () ;
            virtual ~KoboPosition () ;

            void
            set_position(
                  gint64
                , gint64
            ) ;

        protected:

            virtual void
            on_size_request(
                Gtk::Requisition*
            ) ;

            virtual bool
            on_expose_event(
                GdkEventExpose*
            ) ;

            virtual bool
            on_leave_notify_event(
                GdkEventCrossing*
            ) ;

            virtual bool
            on_enter_notify_event(
                GdkEventCrossing*
            ) ;

            virtual bool
            on_button_press_event(
                GdkEventButton*
            ) ;

            virtual bool
            on_button_release_event(
                GdkEventButton*
            ) ;

            virtual bool
            on_motion_notify_event(
                GdkEventMotion*
            ) ;
    } ; 
}

#endif // KOBO_POSITION_HH
