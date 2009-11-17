#ifndef _YOUKI_ROUNDED_ALIGNMENT__HH
#define _YOUKI_ROUNDED_ALIGNMENT__HH

#include <gtkmm.h>
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/widgets/rounded-frame.hh"

namespace MPX
{
    class RoundedScrolledWindow
    : public Gtk::ScrolledWindow 
    {
        public:

                RoundedScrolledWindow(
                ) ;

                virtual
                ~RoundedScrolledWindow() ;

                void
                add_with_frame(Gtk::Widget&) ;

        private:

                void
                probe_adjustable() ;
    };
}
#endif // _YOUKI_ROUNDED_ALIGNMENT__HH
