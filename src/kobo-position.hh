#ifndef KOBO_POSITION_HH
#define KOBO_POSITION_HH

#include <gtkmm.h>
#include <gst/gst.h>

namespace MPX
{
    class KoboPosition

        : public Gtk::DrawingArea

    {
        protected:
        
            gint64      m_duration ;        
            gint64      m_position ;
            gint64      m_seek_position ;
            double      m_seek_factor ;
            bool        m_clicked ;

            sigc::connection m_update_conn ;

            GstClock* m_clock ;

       protected:
    
            inline double
            cos_smooth(
                  double
            ) ;

            double
            get_position(
            ) ;

            bool
            draw_frame(
            )
            {
                m_position = m_clock ? GST_TIME_AS_SECONDS(gst_clock_get_time(m_clock)) : 0 ;
                queue_draw() ;
                return bool(m_update_conn) ;
            }

        public:

            typedef sigc::signal<void, gint64> SignalSeekEvent ;

        protected:
    
            SignalSeekEvent m_SIGNAL_seek_event ;

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

            void
            start(
            ) ;

            void
            stop(
            ) ;

        protected:

            virtual void
            on_size_allocate(
                   Gtk::Allocation&
            ) ;

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

            virtual bool
            on_focus_in_event(
                  GdkEventFocus*
            ) ;

            virtual bool
            on_key_press_event(
                  GdkEventKey*
            ) ;
    } ; 
}

#endif // KOBO_POSITION_HH
