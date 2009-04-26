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

                virtual void
                on_size_allocate(
                      Gtk::Allocation&
                ) ;

        protected:

                Cairo::RefPtr<Cairo::LinearGradient> m_gradient ;

        public:

                RoundedScrolledWindow(
                ) ;

                virtual
                ~RoundedScrolledWindow() ;
    };
}
#endif // _YOUKI_ROUNDED_ALIGNMENT__HH
