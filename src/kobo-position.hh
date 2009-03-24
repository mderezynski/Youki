#ifndef KOBO_POSITION_HH
#define KOBO_POSITION_HH

#include <gtkmm.h>

namespace MPX
{
    class KoboPosition : public Gtk::DrawingArea
    {
        protected:
        
            double m_percent ;
            gint64 m_duration ;        
            gint64 m_position ;
            bool   m_inside ;
    
        public:

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
    } ; 
}

#endif // KOBO_POSITION_HH
