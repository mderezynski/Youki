#include "kobo-controller.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-types.hh"

namespace MPX
{
    KoboController::KoboController ()
    {
        m_main_window = new MainWindow ;

        m_main_track_view = new AllTracksView  ;
        m_main_window->set_widget_top( *m_main_track_view->get_widget() ) ;

        m_main_cover = new KoboCover ;
        m_main_position = new KoboPosition ;

        m_main_window->set_widget_drawer( *m_main_cover ) ; 

        m_vbox_bottom = dynamic_cast<Gtk::VBox*>( m_main_track_view->get_widget() ) ;
        m_vbox_bottom->pack_start( *m_main_position, false, false, 0 ) ;

        m_main_window->show_all() ;

        m_main_track_view->get_alltracks()->signal_track_activated().connect(
            sigc::mem_fun(
                      *this
                    , &KoboController::on_track_activated
        )) ;

        m_play = services->get<Play>("mpx-service-play").get() ;

        m_play->signal_eos().connect(
            sigc::mem_fun(
                  *this
                , &KoboController::on_eos
        )) ;

        m_play->signal_position().connect(
            sigc::mem_fun(
                  *this
                , &KoboController::on_position
        )) ;

        m_play->signal_playstatus().connect(
            sigc::mem_fun(
                  *this
                , &KoboController::on_playstatus
        )) ;

        m_play->signal_stream_switched().connect(
            sigc::mem_fun(
                  *this
                , &KoboController::on_stream_switched
        )) ;
    }

    KoboController::~KoboController ()
    {
    }

    Gtk::Window*
    KoboController::get_widget ()
    {
        return m_main_window ;
    }

////////////////

    void
    KoboController::play_track(
        MPX::Track& t
    )
    {
        try{
                boost::shared_ptr<Library> library = services->get<Library>("mpx-service-library") ;

                std::string location = library->trackGetLocation( t ) ;
                gint64 id = boost::get<gint64>(t[ATTRIBUTE_MPX_TRACK_ID].get()) ;

                m_current_track = t ; 

                m_play->switch_stream( location ) ;
                m_main_track_view->get_alltracks()->set_active_track( id ) ;
        } catch( Library::FileQualificationError & cxe )
        {
            g_message("%s: Error: What: %s", G_STRLOC, cxe.what());
        }
    }

    void
    KoboController::on_position(
          gint64 position
    )
    {
        gint64 duration = m_play->property_duration().get_value() ;

        double percent = double(position) / double(duration) ;

        m_main_position->set_percent( percent ) ;
    }

    void
    KoboController::on_eos ()
    {
        boost::optional<gint64> pos = m_main_track_view->get_alltracks()->get_local_active_track () ;

        if( pos )
        {
            gint64 pos_cpy = pos.get() + 1 ;

            if( pos_cpy < m_main_track_view->get_model()->size() )
            {
                m_current_track = boost::get<4>(m_main_track_view->get_model()->row(pos_cpy)) ;
                play_track( m_current_track.get() ) ; 
                return ;
            }
        }
        else
        {
            if( m_main_track_view->get_model()->size() )
            {
                m_current_track = boost::get<4>(m_main_track_view->get_model()->row( 0 )) ;
                play_track( m_current_track.get() ) ; 
                return ;
            }
        }

        m_play->request_status( PLAYSTATUS_STOPPED ) ; 
    }

    void
    KoboController::on_playstatus(
          PlayStatus status
    )
    {
        switch( status )
        {
            case PLAYSTATUS_STOPPED:
                m_main_track_view->get_alltracks()->clear_active_track() ;
                m_main_cover->clear() ;
                m_current_track.reset() ;
                break ;

            case PLAYSTATUS_WAITING:
                m_main_track_view->get_alltracks()->clear_active_track() ;
                m_main_cover->clear() ;
                m_current_track.reset() ;
                m_main_position->set_percent( 0. ) ;
                break ;

            default: break ;
        }
    }

    void
    KoboController::on_stream_switched()
    {
        g_return_if_fail( bool(m_current_track) ) ;

        if( m_current_track.get().has( ATTRIBUTE_MB_ALBUM_ID ) )
        {
                const std::string& mbid = boost::get<std::string>(m_current_track.get()[ATTRIBUTE_MB_ALBUM_ID].get()) ;

                boost::shared_ptr<Covers> covers = services->get<Covers>("mpx-service-covers") ;
                Glib::RefPtr<Gdk::Pixbuf> cover ;

                if( covers->fetch(
                      mbid
                    , cover
                ))
                {
                    m_main_cover->set(
                        cover->scale_simple(
                              160
                            , 160
                            , Gdk::INTERP_BILINEAR
                    )) ;
                }
        }
        else
        {
                m_main_cover->clear() ;
        }
    }

    void
    KoboController::on_track_activated(
          MPX::Track    t 
        , bool          play
    ) 
    {
        play_track( t ) ;
    }
}
