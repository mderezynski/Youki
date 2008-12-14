#include "mpx/mpx-markov-analyzer-thread.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-types.hh"
#include "mpx/mpx-main.hh"

using boost::get;
using namespace MPX;
using namespace MPX::SQL;

MPX::MarkovAnalyzer::MarkovAnalyzer (MPX::Library & obj_library)
: Service::Base("mpx-service-markov")
{
    m_Library = services->get<Library>("mpx-service-library");
}

MPX::MarkovAnalyzer::~MarkovAnalyzer ()
{
}

void
MPX::MarkovAnalyzer::process_tracks(
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
MPX::MarkovAnalyzer::process_idle()
{
    Glib::Mutex::Lock L (m_queueLock);

    while( m_trackQueue.size() > 1 )
    {
        MPX::Track track1 = m_trackQueue.front();
        m_trackQueue.pop_front();

        MPX::Track track2 = m_trackQueue.front();
        process_tracks (track1, track2);
    }

    return false;
}

void
MPX::MarkovAnalyzer::append (MPX::Track & track)
{
    Glib::Mutex::Lock L (m_queueLock);

    m_trackQueue.push_back(track);

    if(! m_idleConnection )
    {
        m_idleConnection = Glib::signal_idle().connect(
            sigc::mem_fun(
                *this,
                &MarkovAnalyzer::process_idle
        ));
    }
}
