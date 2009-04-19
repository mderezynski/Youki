#ifndef _YOUKI_ROUNDED_ALIGNMENT__HH
#define _YOUKI_ROUNDED_ALIGNMENT__HH

#include <gtkmm.h>
#include "mpx/widgets/cairo-extensions.hh"

namespace MPX
{
    class RoundedScrolledWindow
    : public Gtk::ScrolledWindow 
    {
        protected:

                virtual bool
                on_expose_event(
                      GdkEventExpose*
                ) ;

        public:

                RoundedScrolledWindow(
                ) ;

                virtual
                ~RoundedScrolledWindow() ;
    };
}
#endif // _YOUKI_ROUNDED_ALIGNMENT__HH
