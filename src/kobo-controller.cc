#include "kobo-controller.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include <glibmm/i18n.h>
#include "mpx/mpx-main.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-types.hh"

namespace
{
    bool
    on_alignment_expose(
          GdkEventExpose* event
        , Gtk::Alignment* widget
    )
    {
            Cairo::RefPtr<Cairo::Context> cairo = widget->get_window()->create_cairo_context() ;
            cairo->set_source_rgba( 0.65, 0.65, 0.65, 1. ) ;
            cairo->set_line_width( 0.85 ) ;
            cairo->set_operator( Cairo::OPERATOR_SOURCE ) ; 
           
            MPX::RoundedRectangle( 
                  cairo
                , widget->get_allocation().get_x() + 1
                , widget->get_allocation().get_y() + 1
                , widget->get_allocation().get_width() - 2
                , widget->get_allocation().get_height() - 2
                , 4.
            ) ;

            cairo->stroke () ;

            return true ;
    }
}

namespace MPX
{
    KoboController::KoboController ()
    : m_seek_position( -1 )
    {
        m_VBox              = Gtk::manage( new Gtk::VBox ) ;
        m_ScrolledWin       = Gtk::manage( new Gtk::ScrolledWindow ) ;
        m_Entry             = Gtk::manage( new Gtk::Entry ) ;
        m_Alignment_Entry   = Gtk::manage( new Gtk::Alignment ) ;
        m_HBox_Entry        = Gtk::manage( new Gtk::HBox ) ;
        m_Label_Search      = Gtk::manage( new Gtk::Label(_("_Search:"))) ;

        m_Label_Search->set_mnemonic_widget( *m_Entry ) ;
        m_Label_Search->set_use_underline() ;

        m_VBox->property_spacing() = 2 ; 
        m_HBox_Entry->property_spacing() = 4 ; 
        m_HBox_Entry->set_border_width( 2 ) ;

        m_HBox_Entry->pack_start( *m_Label_Search, false, false, 0 ) ;
        m_HBox_Entry->pack_start( *m_Alignment_Entry, true, true, 0 ) ;

        m_Alignment_Entry->add( *m_Entry ) ;
        m_Alignment_Entry->property_top_padding() = 2 ;
        m_Alignment_Entry->property_bottom_padding() = 2 ;
        m_Alignment_Entry->property_left_padding() = 2 ;
        m_Alignment_Entry->property_right_padding() = 2 ;

        m_Alignment_Entry->signal_expose_event().connect(
            sigc::bind(
                      &on_alignment_expose
                    , m_Alignment_Entry
        )) ;    

        Gdk::Color c ;
        c.set_rgb_p( 0.12, 0.12, 0.12 ) ;
        m_Entry->modify_bg( Gtk::STATE_NORMAL, c ) ;
        m_Entry->modify_base( Gtk::STATE_NORMAL, c ) ;
        m_Entry->modify_bg( Gtk::STATE_ACTIVE, c ) ;
        m_Entry->modify_base( Gtk::STATE_ACTIVE, c ) ;
        m_Entry->modify_bg( Gtk::STATE_PRELIGHT, c ) ;
        m_Entry->modify_base( Gtk::STATE_PRELIGHT, c ) ;
        c.set_rgb_p( 1., 1., 1. ) ; 
        m_Entry->modify_text( Gtk::STATE_NORMAL, c ) ;
        m_Entry->modify_fg( Gtk::STATE_NORMAL, c ) ;
        m_Entry->property_has_frame() = false ; 

        m_Label_Search->modify_bg( Gtk::STATE_NORMAL, c ) ;
        m_Label_Search->modify_base( Gtk::STATE_NORMAL, c ) ;
        m_Label_Search->modify_bg( Gtk::STATE_ACTIVE, c ) ;
        m_Label_Search->modify_base( Gtk::STATE_ACTIVE, c ) ;
        m_Label_Search->modify_bg( Gtk::STATE_PRELIGHT, c ) ;
        m_Label_Search->modify_base( Gtk::STATE_PRELIGHT, c ) ;
        c.set_rgb_p( 1., 1., 1. ) ; 
        m_Label_Search->modify_text( Gtk::STATE_NORMAL, c ) ;
        m_Label_Search->modify_fg( Gtk::STATE_NORMAL, c ) ;

        m_ScrolledWin->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ; 

        m_ListView = new ListView ;

        DataModelP m (new DataModel) ;

        SQL::RowV v ;
        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view ORDER BY album_artist, album, track_view.track")).str()) ; 
        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
        {
                SQL::Row & r = *i;
                try{
                    m->append_track(r, (*(services->get<Library>("mpx-service-library")->sqlToTrack(r, true, false).get()))) ;
                } catch( Library::FileQualificationError )
                {
                }
        }

        m_FilterModel = DataModelFilterP (new DataModelFilter(m)) ;

        m_Entry->signal_changed().connect(
                sigc::bind(
                        sigc::mem_fun(
                              *this
                            , &KoboController::on_entry_changed
                        )
                      , m_FilterModel
                      , m_Entry
        )) ;

        ColumnP c1 (new Column(_("Title"))) ;
        c1->set_column(0) ;

        ColumnP c2 (new Column(_("Track"))) ;
        c2->set_column(5) ;
        c2->set_alignment( Pango::ALIGN_RIGHT ) ;

        ColumnP c3 (new Column(_("Album"))) ;
        c3->set_column(2) ;

        ColumnP c4 (new Column(_("Artist"))) ;
        c4->set_column(1) ;

        m_ListView->append_column(c1) ;
        m_ListView->append_column(c2) ;
        m_ListView->append_column(c3) ;
        m_ListView->append_column(c4) ;

        m_ListView->column_set_fixed(
              1
            , true
            , 60
        ) ;

        m_ListView->set_model(m_FilterModel ) ;

        m_ScrolledWin->add(*m_ListView) ;
        m_ScrolledWin->show_all() ;

        m_main_window = new MainWindow ;
        m_main_cover = new KoboCover ;
        m_main_position = new KoboPosition ;
        m_main_titleinfo = new KoboTitleInfo ;

        m_main_window->set_widget_top( *m_VBox ) ;
        m_main_window->set_widget_drawer( *m_main_cover ) ; 

        m_VBox->pack_start( *m_HBox_Entry, false, false, 0 ) ;
        m_VBox->pack_start( *m_ScrolledWin, true, true, 0 ) ;
        m_VBox->pack_start( *m_main_position, false, false, 0 ) ;
        m_VBox->pack_start( *m_main_titleinfo, false, false, 0 ) ;

        m_ListView->signal_track_activated().connect(
            sigc::mem_fun(
                      *this
                    , &KoboController::on_track_activated
        )) ;

        m_main_position->signal_seek_event().connect(
            sigc::mem_fun(
                  *this
                , &KoboController::on_seek
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
        if( m_seek_position != -1 )
        {
            if( position >= m_seek_position )
            {
                gint64 duration = m_play->property_duration().get_value() ;
                m_main_position->set_position( duration, position ) ;
                m_seek_position = -1 ;
            }
        }
        else
        if( m_seek_position == -1 )
        {
            gint64 duration = m_play->property_duration().get_value() ;
            m_main_position->set_position( duration, position ) ;
        }

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
                m_current_track.reset() ;
                m_main_titleinfo->clear() ;
                m_main_window->queue_draw () ;    
                m_seek_position = -1 ;
                m_ListView->clear_active_track() ;
                m_main_cover->clear() ;
                m_main_position->set_position( 0, 0 ) ;
                break ;

            case PLAYSTATUS_WAITING:
                m_current_track.reset() ;
                m_main_titleinfo->clear() ;
                m_main_window->queue_draw () ;    
                m_seek_position = -1 ;
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

        std::vector<std::string> info ;

        int id[] = { ATTRIBUTE_ARTIST, ATTRIBUTE_TITLE } ;

        for( int n = 0; n < 3 ; ++n ) 
        {
                if( m_current_track.get().has( n ) )
                    info.push_back( boost::get<std::string>(m_current_track.get()[id[n]].get()) ) ;
        }

        m_main_titleinfo->set_info( info ) ;
    }

    void
    KoboController::on_track_activated(
          MPX::Track    t 
        , bool          play
    ) 
    {
        play_track( t ) ;
    }

    void
    KoboController::on_seek(
          gint64        position
    )
    {
        m_seek_position = position ;
        m_play->seek( position ) ;
    }
}
