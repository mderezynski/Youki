#include <gtkmm.h>
#include <boost/format.hpp>
#include "mpx/sql.hh"
#include "mpx/types.hh"
#include "source-musiclib.hh"
#include "tracks-load-thread.hh"

using namespace MPX;
using namespace MPX::SQL;

struct MPX::TracksLoadThread::ThreadData
{
    Glib::RefPtr<Gtk::TreeStore>                    data_Store;
    Glib::Mutex                                 &   data_Lock;
    PAccess<MPX::Library>                           data_Lib;
    Gtk::TreeIter                                   data_Iter;
    MPX::Source::AlbumColumnsT                      data_AlbumColumns;

    TracksLoadThread::SignalPercentage_t            sig_Percentage; 
    TracksLoadThread::SignalRow_t                   sig_Row;

    ThreadData (Glib::RefPtr<Gtk::TreeStore> const& store, Glib::Mutex & lock, PAccess<MPX::Library> & lib)
    : data_Store(store)
    , data_Lock(lock)
    {
        data_Lib = PAccess<MPX::Library>(*(new MPX::Library(lib.get()))); // FIXME: leak 
        data_Iter = store->children().begin();
    }
};

MPX::TracksLoadThread::TracksLoadThread (Glib::RefPtr<Gtk::TreeStore> const& store, Glib::Mutex & lock, PAccess<MPX::Library> & lib)
: sigx::glib_threadable()
, Request(sigc::mem_fun(*this, &TracksLoadThread::on_request))
, SignalPercentage(*this, m_ThreadData, &ThreadData::sig_Percentage)
, SignalRow(*this, m_ThreadData, &ThreadData::sig_Row)
, Store(store)
, Lock(lock)
, Lib(lib)
{
}

MPX::TracksLoadThread::~TracksLoadThread ()
{
}

void
MPX::TracksLoadThread::on_startup ()
{
    m_ThreadData.set(new ThreadData(Store,Lock,Lib));
}

void
MPX::TracksLoadThread::on_cleanup ()
{
    ThreadData * pthreaddata = m_ThreadData.get();
}

bool
MPX::TracksLoadThread::idle_loader ()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    gint64 album_id = (*(pthreaddata->data_Iter))[pthreaddata->data_AlbumColumns.Id];
    RowV v;
    pthreaddata->data_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track") % album_id).str());

    for(RowV::iterator i = v.begin(); i != v.end(); ++i)
    {
        pthreaddata->sig_Row(pthreaddata->data_Iter, *i); 
    }

    ++ pthreaddata->data_Iter ;

    return (pthreaddata->data_Iter) != 
           (pthreaddata->data_Store->children().end());
 
}

void
MPX::TracksLoadThread::on_request ()
{
    ThreadData * pthreaddata = m_ThreadData.get();

    if (dispatcher()->queued_contexts() > 0)
        return;

    maincontext()->signal_idle().connect(sigc::mem_fun(*this, &TracksLoadThread::idle_loader));
}
