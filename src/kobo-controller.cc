#include "kobo-controller.hh"
#include <glibmm/i18n.h>
#include "mpx/mpx-main.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-types.hh"

namespace MPX
{
    KoboController::KoboController ()
    {
        m_VBox        = Gtk::manage( new Gtk::VBox ) ;
        m_ScrolledWin = Gtk::manage( new Gtk::ScrolledWindow ) ;
        m_Entry       = Gtk::manage( new Gtk::Entry ) ;

        Gdk::Color c ;
        c.set_rgb_p( 0.12, 0.12, 0.12 ) ;
        m_Entry->modify_bg( Gtk::STATE_NORMAL, c ) ;
        m_Entry->modify_base( Gtk::STATE_NORMAL, c ) ;
        c.set_rgb_p( 1., 1., 1. ) ; 
        m_Entry->modify_text( Gtk::STATE_NORMAL, c ) ;
        m_Entry->modify_fg( Gtk::STATE_NORMAL, c ) ;
        m_Entry->property_has_frame() = false ; 

        m_ScrolledWin->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ; 

        m_ListView = new ListView;

        DataModelP m (new DataModel);

        SQL::RowV v;
        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view ORDER BY album_artist, album, track_view.track")).str()); 
        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
        {
                SQL::Row & r = *i;
                try{
                    m->append_track(r, (*(services->get<Library>("mpx-service-library")->sqlToTrack(r, true, false).get())));
                } catch( Library::FileQualificationError )
                {
                }
        }

        m_FilterModel = DataModelFilterP (new DataModelFilter(m));

        m_Entry->signal_changed().connect(
                sigc::bind(
                        sigc::mem_fun(
                              *this
                            , &KoboController::on_entry_changed
                        )
                      , m_FilterModel
                      , m_Entry
        ));

        ColumnP c1 (new Column(_("Title")));
        c1->set_column(0);

        ColumnP c2 (new Column(_("Artist")));
        c2->set_column(1);

        ColumnP c3 (new Column(_("Album")));
        c3->set_column(2);

        m_ListView->append_column(c1);
        m_ListView->append_column(c2);
        m_ListView->append_column(c3);

        m_ListView->set_model(m_FilterModel);

        m_ScrolledWin->add(*m_ListView);
        m_ScrolledWin->show_all();

        m_main_window = new MainWindow ;
        m_main_cover = new KoboCover ;
        m_main_position = new KoboPosition ;

        m_main_window->set_widget_top( *m_VBox ) ;
        m_main_window->set_widget_drawer( *m_main_cover ) ; 

        m_VBox->pack_start( *m_Entry, false, false, 0 ) ;
        m_VBox->pack_start( *m_ScrolledWin, true, true, 0 ) ;
        m_VBox->pack_start( *m_main_position, false, false, 0 ) ;

        m_ListView->signal_track_activated().connect(
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

        m_main_window->show_all() ;
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
                m_ListView->set_active_track( id ) ;
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
        boost::optional<gint64> pos = m_ListView->get_local_active_track () ;

        if( pos )
        {
            gint64 pos_cpy = pos.get() + 1 ;

            if( pos_cpy < m_FilterModel->size() )
            {
                m_current_track = boost::get<4>(m_FilterModel->row(pos_cpy)) ;
                play_track( m_current_track.get() ) ; 
                return ;
            }
        }
        else
        {
            if( m_FilterModel->size() )
            {
                m_current_track = boost::get<4>(m_FilterModel->row( 0 )) ;
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
                m_ListView->clear_active_track() ;
                m_main_cover->clear() ;
                m_current_track.reset() ;
                m_main_position->set_percent( 0. ) ;
                break ;

            case PLAYSTATUS_WAITING:
                m_ListView->clear_active_track() ;
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
