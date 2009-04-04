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
        const char* services[] =
        {
              "mpx-service-plugins-gui"
            , "mpx-service-plugins"
            , "mpx-service-controller"
            , "mpx-service-preferences"
            , "mpx-service-play"
            , "mpx-service-library"
            , "mpx-service-covers"
#ifdef HAVE_HAL
            , "mpx-service-hal"
#endif // HAVE_HAL
        } ;

        for( int n = 0; n < G_N_ELEMENTS(services); ++n )
        {
            if(m_services.count(services[n]))
            {
                    Base_p service = m_services[services[n]] ; 

                    if( service )
                    {
                            m_services.erase( services[n] ) ;
                    }
            }
        }
    }

    void
    Manager::add(Base_p service)
    {
        m_services.insert(std::make_pair(service->get_guid(), service));
        g_message("%s: ADDED SERVICE [%s]", G_STRFUNC, service->get_guid().c_str());
    }
}
}
