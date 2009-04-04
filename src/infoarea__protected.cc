#include "config.h"
#include "infoarea.hh"

namespace MPX 
{
        bool
        InfoArea::on_button_press_event(
            GdkEventButton* G_GNUC_UNUSED 
        )
        {
            m_signal.emit() ;
            return true ;
        }

        bool
        InfoArea::on_button_release_event(
            GdkEventButton* G_GNUC_UNUSED 
        )
        {
            return true ;
        }

        bool
        InfoArea::on_expose_event(
            GdkEventExpose* G_GNUC_UNUSED 
        )
        {
            Cairo::RefPtr<Cairo::Context> cairo = get_window ()->create_cairo_context ();

            draw_background( cairo ) ;
            draw_spectrum( cairo ) ;

            return true;
        }
}
