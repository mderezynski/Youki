#include "config.h"

#include <gtkmm.h>
#include <gtk/gtkscrolledwindow.h>
#include "mpx/widgets/rounded-alignment.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/mpx-main.hh"
#include "mpx/i-youki-theme-engine.hh"

namespace MPX
{
    RoundedScrolledWindow::RoundedScrolledWindow(
    )
    {
    }

    RoundedScrolledWindow::~RoundedScrolledWindow()
    {
    }

    void
    RoundedScrolledWindow::add_with_frame(Gtk::Widget&w) 
    {
        RoundedFrame * f = Gtk::manage( new RoundedFrame ) ;
        
    }
}


