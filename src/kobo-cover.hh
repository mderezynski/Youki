#ifndef KOBO_COVER_HH
#define KOBO_COVER_HH

#include <gtkmm.h>

namespace MPX
{
    class KoboCover : public Gtk::DrawingArea
    {
        protected:

            Glib::RefPtr<Gdk::Pixbuf>       m_jewelcase_bot ;
            Glib::RefPtr<Gdk::Pixbuf>       m_jewelcase_top ;
            Glib::RefPtr<Gdk::Pixbuf>       m_cover ;

        public:

            KoboCover () ;
            virtual ~KoboCover () ;

            void
            set(
                Glib::RefPtr<Gdk::Pixbuf>
            ) ;

            void
            clear() ;

        protected:

            bool
            on_expose_event(
                GdkEventExpose*
            ) ;
    } ;
}

#endif // KOBO_COVER_HH
