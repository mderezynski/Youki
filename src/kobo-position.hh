#ifndef KOBO_POSITION_HH
#define KOBO_POSITION_HH

#include <gtkmm.h>

namespace MPX
{
    class KoboPosition : public Gtk::DrawingArea
    {
        protected:
        
            double m_percent ;
        
        public:

            KoboPosition () ;
            virtual ~KoboPosition () ;

            void
            set_percent(
                double
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
    } ; 
}

#endif // KOBO_POSITION_HH
