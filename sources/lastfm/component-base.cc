#include "component-base.hh"

MPX::ComponentBase::ComponentBase ()
: sigx::glib_auto_dispatchable()
{
}

MPX::ComponentBase::~ComponentBase ()
{
}

Gtk::Widget&
MPX::ComponentBase::get_UI ()
{
    return *m_UI;
}
