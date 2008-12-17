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
        m_services.erase("mpx-service-mbimport");
        m_services.erase("mpx-service-plugins-gui");
        m_services.erase("mpx-service-plugins");
        m_services.erase("mpx-service-player");
        m_services.erase("mpx-service-mlibman");
        m_services.erase("mpx-service-preferences");
        m_services.erase("mpx-service-play");
        m_services.erase("mpx-service-markov");
        m_services.erase("mpx-service-library");
        m_services.erase("mpx-service-taglib");
        m_services.erase("mpx-service-covers");
#ifdef HAVE_HAL
        m_services.erase("mpx-service-hal");
#endif // HAVE_HAL
    }

    void
    Manager::add(Base_p service)
    {
        m_services.insert(std::make_pair(service->get_guid(), service));
        g_message("%s: ADDED SERVICE [%s]", G_STRFUNC, service->get_guid().c_str());
    }
}
}
