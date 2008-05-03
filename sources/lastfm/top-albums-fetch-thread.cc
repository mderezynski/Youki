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
    TopAlbumsFetchThread::SignalAlbum_t  Album; 
    int m_Stop; 
};

MPX::TopAlbumsFetchThread::TopAlbumsFetchThread ()
: sigx::glib_threadable()
, RequestLoad(sigc::mem_fun(*this, &TopAlbumsFetchThread::on_load))
, RequestStop(sigc::mem_fun(*this, &TopAlbumsFetchThread::on_stop))
, SignalAlbum(*this, m_ThreadData, &ThreadData::Album)
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
}

void
MPX::TopAlbumsFetchThread::on_load (std::string const& artist)
{
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_Stop, 0);

    try{
        URI u ((boost::format ("http://ws.audioscrobbler.com/1.0/artist/%s/topalbums.xml") % artist).str(), true);
        Glib::ustring uri = u;
        MPX::XmlInstance<LastFM::topalbums> topalbums ((ustring(u)));
        LastFM::topalbums & xml = topalbums.xml();
        for(LastFM::topalbums::album_sequence::iterator i = xml.album().begin(); i != xml.album().end(); ++i)
        {
            if(g_atomic_int_get(&pthreaddata->m_Stop))
            {
                return;
            }

            try{
                Glib::RefPtr<Gdk::Pixbuf> image = Util::get_image_from_uri((*i).image().large());
                pthreaddata->Album.emit(image, (*i).name());                
            } catch(...)
            {
                g_message(G_STRLOC ": Error loading image");
            }
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
    g_atomic_int_set(&pthreaddata->m_Stop, 1);
}
