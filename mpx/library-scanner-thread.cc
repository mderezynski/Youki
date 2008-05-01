#include "mpx/library-scanner-thread.hh"
#include "mpx/library.hh"
#include "mpx/types.hh"
#include "mpx/audio.hh"
#include "metadatareader-taglib.hh"
#include <queue>

MPX::ScanData::ScanData ()
: added(0)
, erroneous(0)
, uptodate(0)
, updated(0)
{
}

struct MPX::LibraryScannerThread::ThreadData
{
    LibraryScannerThread::SignalScanStart_t ScanStart ;
    LibraryScannerThread::SignalScanRun_t   ScanRun ;
    LibraryScannerThread::SignalScanEnd_t   ScanEnd ;
    LibraryScannerThread::SignalReload_t    Reload ;
    LibraryScannerThread::SignalTrack_t     Track ;

    int m_ScanStop;
};

MPX::LibraryScannerThread::LibraryScannerThread (MPX::Library* obj_library)
: sigx::glib_threadable()
, scan(sigc::mem_fun(*this, &LibraryScannerThread::on_scan))
, scan_stop(sigc::mem_fun(*this, &LibraryScannerThread::on_scan_stop))
, signal_scan_start(*this, m_ThreadData, &ThreadData::ScanStart)
, signal_scan_run(*this, m_ThreadData, &ThreadData::ScanRun)
, signal_scan_end(*this, m_ThreadData, &ThreadData::ScanEnd)
, signal_reload(*this, m_ThreadData, &ThreadData::Reload)
, signal_track(*this, m_ThreadData, &ThreadData::Track)
, m_Library(obj_library)
{
    m_Connectable = new ScannerConnectable(signal_scan_start, signal_scan_run, signal_scan_end, signal_reload, signal_track);
}

MPX::LibraryScannerThread::~LibraryScannerThread ()
{
    delete m_Connectable;
}

MPX::LibraryScannerThread::ScannerConnectable&
MPX::LibraryScannerThread::connect ()
{
    return *m_Connectable;
}

void
MPX::LibraryScannerThread::on_startup ()
{
    m_ThreadData.set(new ThreadData);
}

void
MPX::LibraryScannerThread::on_cleanup ()
{
}

void
MPX::LibraryScannerThread::on_scan (ScanData const& scan_data_)
{
    ScanData scan_data = scan_data_;
    ThreadData * pthreaddata = m_ThreadData.get();
    g_atomic_int_set(&pthreaddata->m_ScanStop, 0);

    pthreaddata->ScanStart.emit();

    for(Util::FileList::iterator i = scan_data.collection.begin(); i != scan_data.collection.end(); ++i)
    {
        Track track;
        std::string type;

        try{ 
            if (!Audio::typefind(*i, type))  
              ++(scan_data.erroneous) ;
              continue;
          }  
        catch (Glib::ConvertError & cxe)
          {
              ++(scan_data.erroneous) ;
              continue;
          }   

        track[ATTRIBUTE_TYPE] = type ;
        track[ATTRIBUTE_LOCATION] = *i ;
        track[ATTRIBUTE_LOCATION_NAME] = scan_data.name;

        if( !m_Library->mReaderTagLib->get( *i, track ) )
        {
           ++(scan_data.erroneous) ;
           continue;
        }

        try{
            switch(pthreaddata->Track.emit( track, *i , scan_data.insert_path_sql ))
            {
                case SCAN_RESULT_UPTODATE:
                    ++(scan_data.uptodate) ;
                    break;

                case SCAN_RESULT_OK:
                    ++(scan_data.added) ;
                    break;

                case SCAN_RESULT_ERROR:
                    ++(scan_data.erroneous) ;
                    break;

                case SCAN_RESULT_UPDATE:
                    ++(scan_data.updated) ;
                    break;
            }
        }
        catch( Glib::ConvertError & cxe )
        {
            g_warning("%s: %s", G_STRLOC, cxe.what().c_str() );
        }

        ++(scan_data.position);
        pthreaddata->ScanRun.emit(std::distance(scan_data.collection.begin(), i), scan_data.collection.size());
    }

    pthreaddata->ScanEnd.emit(scan_data.added, scan_data.uptodate, scan_data.updated, scan_data.erroneous, scan_data.collection.size());

    
}

void
MPX::LibraryScannerThread::on_scan_stop ()
{
}
