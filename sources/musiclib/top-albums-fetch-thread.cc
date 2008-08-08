#include "mpx/mpx-uri.hh"
#include "mpx/xml/xmltoc++.hh"
#include "mpx/util-ui.hh"

#include "xsd-tag-topalbums.hxx"
#include "top-albums-fetch-thread.hh"

#include <gdkmm/pixbuf.h>
#include <glibmm/main.h>
#include <boost/format.hpp>

using namespace MPX;


struct MPX::TopAlbumsFetchThread::ThreadData
{
    MPX::XmlInstance<tag>                         * Xml; 
    tag::album_sequence::size_type                  Position;
    int                                             Stop;
    sigc::connection                                IdleConnection;

    TopAlbumsFetchThread::SignalAlbum_t             Album; 
    TopAlbumsFetchThread::SignalStopped_t           Stopped; 
    TopAlbumsFetchThread::SignalProgress_t          Progress; 

    ThreadData () : Xml(0) {} 
};

MPX::TopAlbumsFetchThread::TopAlbumsFetchThread ()
: sigx::glib_threadable()

, RequestLoad(
    sigc::mem_fun(
        *this,
        &TopAlbumsFetchThread::on_load
  ))

, RequestStop(
    sigc::mem_fun(
        *this,
        &TopAlbumsFetchThread::on_stop
  ))

, SignalAlbum(
        *this,
        m_ThreadData,
        &ThreadData::Album
  )

, SignalStopped(
        *this,
        m_ThreadData,
        &ThreadData::Stopped
  )

, SignalProgress(
        *this,
        m_ThreadData,
        &ThreadData::Progress
  )

{
}

MPX::TopAlbumsFetchThread::~TopAlbumsFetchThread ()
{
}

void
MPX::TopAlbumsFetchThread::on_startup ()
{
    m_ThreadData.set(new ThreadData);
}

void
MPX::TopAlbumsFetchThread::on_cleanup ()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if(pthreaddata->Xml)
        delete pthreaddata->Xml;
}

bool
MPX::TopAlbumsFetchThread::idle_loader ()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    try{

        if(g_atomic_int_get(&(pthreaddata->Stop)))
        {
            return false;
        }

        pthreaddata->Progress.emit( gint64(pthreaddata->Position + 1), gint64(pthreaddata->Xml->xml().album().size()) );
    
        Glib::RefPtr<Gdk::Pixbuf> image =
            Util::get_image_from_uri(
                (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).coverart().large()
            );

        pthreaddata->Album.emit(
            image,  
            (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).artist().name(),
            (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).name()
        );                

    } catch(...)
    {
        g_message(G_STRLOC ": Error loading image");
    }

    ++ pthreaddata->Position ;

    bool done = (pthreaddata->Xml->xml().album().begin() + pthreaddata->Position) == 
                (pthreaddata->Xml->xml().album().end());

    return !done;
}

void
MPX::TopAlbumsFetchThread::on_load (std::string const& value)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if (dispatcher()->queued_contexts() > 0)
        return;

    g_atomic_int_set(&(pthreaddata->Stop), 0);

    try{
        URI u ((boost::format ("http://ws.audioscrobbler.com/1.0/tag/%s/topalbums.xml") % value).str(), true);

        if(pthreaddata->Xml)
        {
            delete pthreaddata->Xml;
        }

        pthreaddata->Xml = new MPX::XmlInstance<tag>(Glib::ustring(u));
        pthreaddata->Position = 0; 

        if(pthreaddata->Xml->xml().album().size())
        {
            pthreaddata->IdleConnection = maincontext()->signal_idle().connect(sigc::mem_fun(*this, &TopAlbumsFetchThread::idle_loader));
        }

    } catch( std::runtime_error & cxe)
    {
        g_message(G_STRLOC ": Caught runtime error: %s", cxe.what());
    }
}

void
MPX::TopAlbumsFetchThread::on_stop ()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    g_atomic_int_set(&(pthreaddata->Stop), 1);
    pthreaddata->IdleConnection.disconnect();
    pthreaddata->Stopped.emit();
}
