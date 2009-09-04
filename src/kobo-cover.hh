#ifndef KOBO_COVER_HH
#define KOBO_COVER_HH

#include <gtkmm.h>

namespace MPX
{
    class IYoukiThemeEngine ;

    class KoboCover : public Gtk::DrawingArea
    {
        protected:

            Glib::RefPtr<Gdk::Pixbuf>       m_cover ;
            IYoukiThemeEngine             * m_Theme ;
            Gdk::Color                      m_CurrentColor ;

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
