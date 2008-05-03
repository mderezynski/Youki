#include "mpx/uri.hh"
#include "mpx/xmltoc++.hh"
#include "mpx/util-ui.hh"
#include <gtkmm.h>
#include <boost/format.hpp>

#include "xsd-user-top-albums.hxx"
#include "user-top-albums-fetch-thread.hh"

using namespace MPX;


struct MPX::UserTopAlbumsFetchThread::ThreadData
{
    UserTopAlbumsFetchThread::SignalAlbum_t  Album; 
    int m_Stop; 
};

MPX::UserTopAlbumsFetchThread::UserTopAlbumsFetchThread ()
: sigx::glib_threadable()
, RequestLoad(sigc::mem_fun(*this, &UserTopAlbumsFetchThread::on_load))
, RequestStop(sigc::mem_fun(*this, &UserTopAlbumsFetchThread::on_stop))
, SignalAlbum(*this, m_ThreadData, &ThreadData::Album)
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
}

void
MPX::UserTopAlbumsFetchThread::on_load (std::string const& artist)
{
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_Stop, 0);

    try{
        URI u ((boost::format ("http://ws.audioscrobbler.com/1.0/user/%s/topalbums.xml") % artist).str(), true);
        Glib::ustring uri = u;
        MPX::XmlInstance<topalbums> topalbums ((ustring(u)));
        for(topalbums::album_sequence::iterator i = topalbums.xml().album().begin(); i != topalbums.xml().album().end(); ++i)
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
MPX::UserTopAlbumsFetchThread::on_stop ()
{
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_Stop, 1);
}
