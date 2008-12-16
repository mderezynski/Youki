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
        m_services["mpx-service-plugins-gui"].reset();
        m_services["mpx-service-plugins"].reset();
        m_services["mpx-service-player"].reset();
        m_services["mpx-service-mlibman"].reset();
        m_services["mpx-service-mbimport"].reset();
        m_services["mpx-service-play"].reset();
        m_services["mpx-service-markov"].reset();
        m_services["mpx-service-library"].reset();
        m_services["mpx-service-taglib"].reset();
        m_services["mpx-service-covers"].reset();

#ifdef HAVE_HAL
        m_services["mpx-service-hal"].reset();
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
