#include "mpx/mpx-services.hh"
#include <glib/gmessages.h>

namespace MPX
{
namespace Service
{
    Manager::Manager ()
    {
    }

    Manager::~Manager ()
    {
    }

    void
    Manager::add(Base_p service)
    {
        m_services.insert(std::make_pair(service->get_guid(), service));
        g_message("%s: ADDED SERVICE [%s]", G_STRFUNC, service->get_guid().c_str());
    }
}
}
