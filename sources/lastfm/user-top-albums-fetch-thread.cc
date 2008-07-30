#include "mpx/mpx-uri.hh"
#include "mpx/xml/xmltoc++.hh"
#include "mpx/util-ui.hh"
#include <gtkmm.h>
#include <boost/format.hpp>

#include "xsd-user-top-albums.hxx"
#include "user-top-albums-fetch-thread.hh"

using namespace MPX;


struct MPX::UserTopAlbumsFetchThread::ThreadData
{
    UserTopAlbumsFetchThread::SignalAlbum_t       Album; 
    UserTopAlbumsFetchThread::SignalStopped_t     Stopped; 
    MPX::XmlInstance<topalbums>                 * Xml;
    int                                           Stop; 
    topalbums::album_sequence::size_type          Position;

    ThreadData ()
    : Xml(0)
    {}
};

MPX::UserTopAlbumsFetchThread::UserTopAlbumsFetchThread ()
: sigx::glib_threadable()
, RequestLoad(sigc::mem_fun(*this, &UserTopAlbumsFetchThread::on_load))
, RequestStop(sigc::mem_fun(*this, &UserTopAlbumsFetchThread::on_stop))
, SignalAlbum(*this, m_ThreadData, &ThreadData::Album)
, SignalStopped(*this, m_ThreadData, &ThreadData::Stopped)
{
}

MPX::UserTopAlbumsFetchThread::~UserTopAlbumsFetchThread ()
{
}

void
MPX::UserTopAlbumsFetchThread::on_startup ()
{
    m_ThreadData.set(new ThreadData);
}

void
MPX::UserTopAlbumsFetchThread::on_cleanup ()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if(pthreaddata->Xml)
        delete pthreaddata->Xml;
}

bool
MPX::UserTopAlbumsFetchThread::idle_loader ()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if(pthreaddata->Stop)
    {
        return false;
    }

    try{
        TopAlbum a;
        a.Cover     = Util::get_image_from_uri((*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).image().large());
        a.Artist    = (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).artist();
        a.Album     = (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).name();
        a.MBID      = (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).mbid();
        a.Playcount = (*(pthreaddata->Xml->xml().album().begin() + pthreaddata->Position)).playcount();
        pthreaddata->Album.emit(a);
    } catch(...)
    {
        g_message(G_STRLOC ": Error loading image");
    }

    ++ pthreaddata->Position ;

    return (pthreaddata->Xml->xml().album().begin() + pthreaddata->Position) != 
           (pthreaddata->Xml->xml().album().end());
}

void
MPX::UserTopAlbumsFetchThread::on_load (std::string const& artist)
{
    ThreadData * pthreaddata = m_ThreadData.get();

    pthreaddata->Stop = 0;

    try{
        URI u ((boost::format ("http://ws.audioscrobbler.com/1.0/user/%s/topalbums.xml") % artist).str(), true);
        if(pthreaddata->Xml)
            delete pthreaddata->Xml;
        pthreaddata->Xml = new MPX::XmlInstance<topalbums> ((ustring(u)));
        pthreaddata->Position = 0;    
        maincontext()->signal_idle().connect(sigc::mem_fun(*this,&UserTopAlbumsFetchThread::idle_loader));
    } catch( std::runtime_error & cxe)
    {
        g_message(G_STRLOC ": Caught runtime error: %s", cxe.what());
    }
}

void
MPX::UserTopAlbumsFetchThread::on_stop ()
{
    ThreadData * pthreaddata = m_ThreadData.get();
    pthreaddata->Stop = 1;
    pthreaddata->Stopped.emit();
}
