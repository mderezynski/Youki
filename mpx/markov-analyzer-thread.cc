#include "mpx/mpx-markov-analyzer-thread.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"

using boost::get;
using namespace MPX;
using namespace MPX::SQL;

struct MPX::MarkovAnalyzerThread::ThreadData
{
    TrackQueue_t                m_trackQueue;
};

MPX::MarkovAnalyzerThread::MarkovAnalyzerThread (MPX::Library & obj_library)
: sigx::glib_threadable()
, append(sigc::mem_fun(*this, &MarkovAnalyzerThread::on_append))
, m_Library(new MPX::Library(obj_library))
{
}

MPX::MarkovAnalyzerThread::~MarkovAnalyzerThread ()
{
}

void
MPX::MarkovAnalyzerThread::on_startup ()
{
    m_ThreadData.set(new ThreadData);
}

void
MPX::MarkovAnalyzerThread::on_cleanup ()
{
}

void
MPX::MarkovAnalyzerThread::process_tracks(
    MPX::Track & track1,
    MPX::Track & track2
)
{
    using boost::get;

    if( !(track1.has(ATTRIBUTE_MPX_TRACK_ID) && track2.has(ATTRIBUTE_MPX_TRACK_ID) ))
    {
        return;
    }

    m_Library->markovUpdate(
        get<gint64>(track1[ATTRIBUTE_MPX_TRACK_ID].get()),
        get<gint64>(track2[ATTRIBUTE_MPX_TRACK_ID].get())
    );
}

bool
MPX::MarkovAnalyzerThread::process_idle()
{
    ThreadData * pthreaddata = m_ThreadData.get();
    while( pthreaddata->m_trackQueue.size() > 1 )
    {
        MPX::Track track1 = pthreaddata->m_trackQueue.front();
        pthreaddata->m_trackQueue.pop_front();

        MPX::Track track2 = pthreaddata->m_trackQueue.front();

        process_tracks (track1, track2);
    }

    return false;
}

void
MPX::MarkovAnalyzerThread::on_append (MPX::Track & track)
{
    ThreadData * pthreaddata = m_ThreadData.get();
    pthreaddata->m_trackQueue.push_back(track);

    if(! m_idleConnection )
    {
        m_idleConnection = maincontext()->signal_idle().connect(
            sigc::mem_fun(
                *this,
                &MarkovAnalyzerThread::process_idle
        ));
    }
}
