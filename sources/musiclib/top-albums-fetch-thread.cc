#include "mpx/uri.hh"
#include "mpx/xmltoc++.hh"
#include "mpx/util-ui.hh"
#include <gtkmm.h>
#include <boost/format.hpp>

#include "xsd-top-albums.hxx"
#include "top-albums-fetch-thread.hh"

using namespace MPX;


struct MPX::TopAlbumsFetchThread::ThreadData
{
    std::string                                     Artist;
    MPX::XmlInstance<LastFM::topalbums>           * Xml; 
    LastFM::topalbums::album_sequence::size_type    Position;

    int Stop;

    TopAlbumsFetchThread::SignalAlbum_t             Album; 
    TopAlbumsFetchThread::SignalStopped_t           Stopped; 

    ThreadData ()    
    : Xml(0)
    {
    }
};

MPX::TopAlbumsFetchThread::TopAlbumsFetchThread ()
: sigx::glib_threadable()
, RequestLoad(sigc::mem_fun(*this, &TopAlbumsFetchThread::on_load))
, RequestStop(sigc::mem_fun(*this, &TopAlbumsFetchThread::on_stop))
, SignalAlbum(*this, m_ThreadData, &ThreadData::Album)
, SignalStopped(*this, m_ThreadData, &ThreadData::Stopped)
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
        if (pthreaddata->Stop)    
        {
            return false;
        }

        Glib::RefPtr<Gdk::Pixbuf> image =
            Util::get_image_from_uri(
                (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).image().large()
            );

        pthreaddata->Album.emit(
            image, (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).name()
        );                

    } catch(...)
    {
        g_message(G_STRLOC ": Error loading image");
    }

    ++ pthreaddata->Position ;

    return (pthreaddata->Xml->xml().album().begin() + pthreaddata->Position) != 
           (pthreaddata->Xml->xml().album().end()) || (pthreaddata->Position == 10);
 
}

void
MPX::TopAlbumsFetchThread::on_load (std::string const& artist)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if (dispatcher()->queued_contexts() > 0)
        return;

    pthreaddata->Stop = 0;

    try{
        URI u ((boost::format ("http://ws.audioscrobbler.com/1.0/artist/%s/topalbums.xml") % artist).str(), true);
        Glib::ustring uri = u;
        if(pthreaddata->Xml)
            delete pthreaddata->Xml;
        pthreaddata->Xml = new MPX::XmlInstance<LastFM::topalbums>((ustring(u)));
        pthreaddata->Position = 0; 
        if(pthreaddata->Xml->xml().album().size())
        {
            maincontext()->signal_idle().connect(sigc::mem_fun(*this, &TopAlbumsFetchThread::idle_loader));
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

    pthreaddata->Stop = 1;
    pthreaddata->Stopped.emit();
}
