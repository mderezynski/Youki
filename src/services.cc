#include "mpx/mpx-services.hh"
#include <glib/gmessages.h>
#include <sigx/sigx.h>

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
                "mpx-service-plugins"
                "mpx-service-player"
                "mpx-service-mbimport"
                "mpx-service-mlibman"
                "mpx-service-preferences"
                "mpx-service-play"
                "mpx-service-markov"
                "mpx-service-artist-images"
                "mpx-service-library"
                "mpx-service-taglib"
                "mpx-service-covers"
#ifdef HAVE_HAL
                "mpx-service-hal"
#endif // HAVE_HAL
        } ;

        for( int n = 0; n < G_N_ELEMENTS(services); ++n )
        {
            Base_p service = m_services[services[n]] ; 

            sigx::glib_threadable * p = dynamic_cast<sigx::glib_threadable*>(service.get());
            if( p )
            {
                p->finish ();
            }

            m_services.erase( services[n] ) ;
        }
    }

    void
    Manager::add(Base_p service)
    {
        sigx::glib_threadable * p = dynamic_cast<sigx::glib_threadable*>(service.get());
        if( p )
        {
            p->run ();
        }

        m_services.insert(std::make_pair(service->get_guid(), service));
        g_message("%s: ADDED SERVICE [%s]", G_STRFUNC, service->get_guid().c_str());
    }
}
}
