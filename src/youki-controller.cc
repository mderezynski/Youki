#include "youki-controller.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include <glibmm/i18n.h>
#include <gdk/gdkkeysyms.h>
#include "mpx/mpx-main.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-types.hh"
#include "mpx/mpx-preferences.hh"

namespace
{
    bool
    on_alignment_expose(
          GdkEventExpose* event
        , Gtk::Alignment* widget
    )
    {
            Cairo::RefPtr<Cairo::Context> cairo = widget->get_window()->create_cairo_context() ;

            cairo->set_operator( Cairo::OPERATOR_ATOP ) ; 

            cairo->set_source_rgba(
                  .65
                , .65
                , .65
                , 0.4 
            ) ;

            cairo->set_line_width( 1. ) ;
           
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
    YoukiController::YoukiController(
        DBus::Connection conn
    )
    : DBus::ObjectAdaptor( conn, "/info/backtrace/Youki/App" )
    , m_main_window( 0 )
    , m_seek_position( -1 )
    {
        m_mlibman_dbus_proxy = new info::backtrace::Youki::MLibMan_proxy_actual( conn ) ;
        m_mlibman_dbus_proxy->signal_scan_end().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_library_scan_end
        )) ;

        m_play = services->get<Play>("mpx-service-play").get() ;
        m_play->signal_eos().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_eos
        )) ;

        m_play->signal_position().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_position
        )) ;

        m_play->signal_playstatus().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_playstatus
        )) ;

        m_play->signal_stream_switched().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_stream_switched
        )) ;

        m_play->signal_request_window_id().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_request_window_id
        )) ;

        m_play->signal_video_geom().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_video_geom
        )) ;

        //m_VideoWidget       = Gtk::manage( new VideoWidget( m_play ) ) ;
        m_Paned             = Gtk::manage( new Gtk::HPaned ) ;
        m_VBox              = Gtk::manage( new Gtk::VBox ) ;
        m_HBox_Entry        = Gtk::manage( new Gtk::HBox ) ;
        m_HBox_Controls     = Gtk::manage( new Gtk::HBox ) ;

        m_Entry             = Gtk::manage( new Gtk::Entry ) ;
        m_Entry->signal_activate().connect(
                sigc::bind(
                        sigc::mem_fun(
                              *this
                            , &YoukiController::on_entry_changed
                        )
                      , m_FilterModel
                      , m_Entry
        )) ;

        m_Alignment_Entry   = Gtk::manage( new Gtk::Alignment ) ;
        m_Label_Search      = Gtk::manage( new Gtk::Label(_("_Search:"))) ;

        m_ListView          = Gtk::manage( new ListView ) ;
        m_ListView->signal_track_activated().connect(
            sigc::mem_fun(
                      *this
                    , &YoukiController::on_list_view_tr_track_activated
        )) ;

        m_ListViewAA        = Gtk::manage( new ListViewAA ) ;
        m_ListViewAA->signal_selection_changed().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_aa_selection_changed
        )) ;

        m_ScrolledWin       = Gtk::manage( new Gtk::ScrolledWindow ) ;
        m_ScrolledWinAA     = Gtk::manage( new Gtk::ScrolledWindow ) ;

        m_main_window       = Gtk::manage( new MainWindow ) ;
        m_main_window->signal_key_press_event().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_main_window_key_press
        )) ;

        m_main_window->signal_quit().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::initiate_quit
        )) ;

        m_main_cover        = Gtk::manage( new KoboCover ) ;

        Gdk::Color background ;
        background.set_rgb_p( 0.1, 0.1, 0.1 ) ;

        m_main_position     = Gtk::manage( new KoboPosition( background ) ) ;
        m_main_position->signal_seek_event().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_position_seek
        )) ;

        m_main_titleinfo    = Gtk::manage( new KoboTitleInfo ) ;

        m_main_infoarea     = Gtk::manage( new InfoArea ) ;
        m_main_infoarea->signal_clicked().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_info_area_clicked
        )) ;

        m_main_volume       = Gtk::manage( new KoboVolume ) ;
        m_main_volume->set_volume(
            m_play->property_volume().get_value()
        ) ;
        m_main_volume->signal_set_volume().connect(
            sigc::mem_fun(
                  *this 
                , &YoukiController::on_volume_set_volume
        )) ;

        m_icon              = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "youki.png" )) ;

        m_main_window->set_icon( m_icon ) ;

        m_Label_Search->set_mnemonic_widget( *m_Entry ) ;
        m_Label_Search->set_use_underline() ;

        m_VBox->property_spacing() = 4 ; 
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

        m_ScrolledWin->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS ) ; 
        m_ScrolledWinAA->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS ) ; 

        {
                DataModelP m (new DataModel) ;
                m_FilterModel = DataModelFilterP (new DataModelFilter(m)) ;

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

                m_ListView->set_model( m_FilterModel ) ;

                m_ScrolledWin->add(*m_ListView) ;
                m_ScrolledWin->show_all() ;

                m_Entry->signal_changed().connect(
                        sigc::bind(
                                sigc::mem_fun(
                                      *this
                                    , &YoukiController::on_entry_changed
                                )
                              , m_FilterModel
                              , m_Entry
                )) ;
        }

        {
                DataModelAAP m (new DataModelAA) ;
                m_FilterModelAA = DataModelFilterAAP (new DataModelFilterAA(m)) ;

                ColumnAAP c1 (new ColumnAA(_("Album Artist"))) ;
                c1->set_column(0) ;

                m_ListViewAA->append_column(c1) ;
                m_ListViewAA->set_model( m_FilterModelAA ) ;

                m_ScrolledWinAA->add(*m_ListViewAA) ;
                m_ScrolledWinAA->show_all() ;
        }

        m_Paned->add1( *m_ScrolledWinAA ) ;
        m_Paned->add2( *m_ScrolledWin ) ;

        m_HBox_Controls->pack_start( *m_main_position, true, true, 0 ) ;
        m_HBox_Controls->pack_start( *m_main_volume, false, false, 0 ) ;
        m_HBox_Controls->set_spacing( 2 ) ;

        m_main_window->set_widget_top( *m_VBox ) ;
        m_main_window->set_widget_drawer( *m_main_cover ) ; 

        m_VBox->pack_start( *m_HBox_Entry, false, false, 0 ) ;
        m_VBox->pack_start( *m_Paned, true, true, 0 ) ;
        m_VBox->pack_start( *m_main_titleinfo, false, false, 0 ) ;
        m_VBox->pack_start( *m_HBox_Controls, false, false, 0 ) ;
        m_VBox->pack_start( *m_main_infoarea, false, false, 0 ) ;

#if 0
        m_main_window->set_widget_drawer( *m_VideoWidget ) ; 
        gtk_widget_realize(GTK_WIDGET(m_VideoWidget->gobj())) ;

        m_main_window->show_all() ;
        while (gtk_events_pending())
            gtk_main_iteration() ;

        m_play->set_window_id( m_VideoWidget->get_video_xid() ) ;
#endif

        m_control_status_icon = new YoukiControllerStatusIcon ;
        m_control_status_icon->signal_clicked().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_status_icon_clicked
        )) ;
        m_control_status_icon->signal_scroll_up().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_status_icon_scroll_up
        )) ;
        m_control_status_icon->signal_scroll_down().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_status_icon_scroll_down
        )) ;

        reload_library () ;

        m_play->signal_spectrum().connect(
            sigc::mem_fun(
                  *m_main_infoarea
                , &InfoArea::update_spectrum
        )) ;

        m_main_window->show_all() ;

        StartupComplete () ;
    }

    YoukiController::~YoukiController ()
    {
        delete m_control_status_icon ;
        delete m_main_window ;
        m_mlibman_dbus_proxy->Exit () ;
        delete m_mlibman_dbus_proxy ;

        ShutdownComplete () ;
    }

    Gtk::Window*
    YoukiController::get_widget ()
    {
        return m_main_window ;
    }

////////////////

    void
    YoukiController::initiate_quit ()
    {
        m_main_window->quit () ;

        Glib::signal_timeout().connect(
              sigc::mem_fun(
                    *this
                  , &YoukiController::quit_timeout
              )
            , 500
        ) ;
    }

    bool
    YoukiController::quit_timeout ()
    {
        Gtk::Main::quit () ;
        return false ;
    }

    void
    YoukiController::reload_library ()
    {
        m_FilterModel->clear () ;
        m_FilterModelAA->clear () ;

        // Tracks 

        SQL::RowV v ;
        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view ORDER BY album_artist, date, album, track_view.track")).str()) ; 
        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
        {
                SQL::Row & r = *i;
                try{
                    m_FilterModel->append_track_quiet(r, (*(services->get<Library>("mpx-service-library")->sqlToTrack(r, true, false).get()))) ;
                } catch( Library::FileQualificationError )
                {
                }
        }

        m_FilterModel->regen_mapping () ;

        // Album Artists

        m_FilterModelAA->append_artist_quiet(
              _("All Artists") 
            , -1
        ) ;

        v.clear () ; 
        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM album_artist ORDER BY album_artist")).str()) ; 

        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
        {
                SQL::Row & r = *i;
                m_FilterModelAA->append_artist_quiet(
                      boost::get<std::string>(r["album_artist"])
                    , boost::get<gint64>(r["id"])
                ) ;
        }

        m_FilterModelAA->regen_mapping () ;
    }

    void
    YoukiController::on_library_scan_end(
    )
    {
        reload_library () ;
    }

////////////////

    void
    YoukiController::play_track(
        MPX::Track& t
    )
    {
        try{
                boost::shared_ptr<Library> library = services->get<Library>("mpx-service-library") ;

                m_current_track = t ; 
                m_play->switch_stream( library->trackGetLocation( t ) ) ;
                m_ListView->set_active_track( boost::get<gint64>(t[ATTRIBUTE_MPX_TRACK_ID].get()) ) ;
                m_control_status_icon->set_metadata( t ) ;

        } catch( Library::FileQualificationError & cxe )
        {
            g_message("%s: Error: What: %s", G_STRLOC, cxe.what());
        }
    }

    void
    YoukiController::on_play_position(
          gint64 position
    )
    {
        gint64 duration ;

        if( m_seek_position != -1 )
        {
            if( position >= m_seek_position )
            {
                duration = m_play->property_duration().get_value() ;
                m_seek_position = -1 ;
            }
        }
        else
        if( m_seek_position == -1 )
        {
            duration = m_play->property_duration().get_value() ;
        }

        m_main_position->set_position( duration, position ) ;
        m_control_status_icon->set_position( duration, position ) ;
    }

    void
    YoukiController::on_play_eos ()
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
        if( m_FilterModel->size() )
        {
            m_current_track = boost::get<4>(m_FilterModel->row( 0 )) ;
            play_track( m_current_track.get() ) ; 
            return ;
        }

        m_play->request_status( PLAYSTATUS_STOPPED ) ; 
    }

    void
    YoukiController::on_play_playstatus(
          PlayStatus status
    )
    {
        m_control_status_icon->set_playstatus( status ) ;

        switch( status )
        {
            case PLAYSTATUS_PLAYING:
                m_VideoWidget->property_playing() = true ;
                m_VideoWidget->queue_draw() ;
                break ;

            case PLAYSTATUS_STOPPED:
                m_VideoWidget->property_playing() = false ;
                m_VideoWidget->queue_draw() ;
                m_current_track.reset() ;
                m_main_titleinfo->clear() ;
                m_main_window->queue_draw () ;    
                m_ListView->clear_active_track() ;
                m_main_cover->clear() ;
                m_control_status_icon->set_image( Glib::RefPtr<Gdk::Pixbuf>(0) ) ;
                m_main_position->set_position( 0, 0 ) ;
                m_seek_position = -1 ;
                break ;

            case PLAYSTATUS_WAITING:
                m_VideoWidget->property_playing() = false ;
                m_VideoWidget->queue_draw() ;
                m_current_track.reset() ;
                m_main_titleinfo->clear() ;
                m_main_window->queue_draw () ;    
                m_seek_position = -1 ;
                break ;

            case PLAYSTATUS_PAUSED:
                m_VideoWidget->property_playing() = false ;
                m_VideoWidget->queue_draw() ;
                break ;

            default: break ;
        }
    }

    void
    YoukiController::on_play_stream_switched()
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

                    m_control_status_icon->set_image( cover ) ;
                }
                else
                {
                    m_main_cover->clear() ;

                    m_control_status_icon->set_image( Glib::RefPtr<Gdk::Pixbuf>(0) ) ;
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
    YoukiController::on_list_view_tr_track_activated(
          MPX::Track    t 
        , bool          play
    ) 
    {
        play_track( t ) ;
    }

    void
    YoukiController::on_list_view_aa_selection_changed(
    ) 
    {
        gint64 id = m_ListViewAA->get_selected() ;

        m_FilterModel->clear_synthetic_constraints_quiet () ;

        if( id == -1 )
        {
            m_FilterModel->regen_mapping() ;
            return ;
        }

        AQE::Constraint_t c ;

        c.TargetAttr = ATTRIBUTE_MPX_ALBUM_ARTIST_ID ;
        c.TargetValue = id ;
        c.MatchType = AQE::MT_EQUAL ;

        m_FilterModel->add_synthetic_constraint( c ) ;

        m_Entry->set_text("") ;
    }

    void
    YoukiController::on_position_seek(
          gint64        position
    )
    {
        m_seek_position = position ;
        m_play->seek( position ) ;
    }

    void
    YoukiController::on_volume_set_volume(
          int           volume
    )
    {
        mcs->key_set<int>(
              "mpx"
            ,"volume"
            , volume
        ) ;

        m_play->property_volume().set_value( volume ) ;
    }

    ::Window
    YoukiController::on_play_request_window_id(
    )
    {
        return m_VideoWidget->get_video_xid() ;
    }

    void
    YoukiController::on_play_video_geom(
          int               width
        , int               height
        , const GValue*     par
    )
    {
        m_VideoWidget->property_geometry() = Geometry( width, height ) ;

        if( par )
        {
            m_VideoWidget->set_par( par ) ;
        }

        m_VideoWidget->queue_draw() ;
    }

    void
    YoukiController::on_info_area_clicked ()
    {
        PlayStatus s = PlayStatus(m_play->property_status().get_value()) ;

        if( s == PLAYSTATUS_PAUSED )
            m_play->request_status( PLAYSTATUS_PLAYING ) ;
        else
        if( s == PLAYSTATUS_PLAYING )
            m_play->request_status( PLAYSTATUS_PAUSED ) ;
    }

    bool
    YoukiController::on_main_window_key_press(
        GdkEventKey* event
    ) 
    {
        switch( event->keyval )
        {
            case GDK_F1:
                m_mlibman_dbus_proxy->ShowWindow () ;
                return true ;

            case GDK_F2:
                services->get<Preferences>("mpx-service-preferences")->present () ;
                return true ;

            case GDK_q:
            case GDK_Q:
                if( event->state & GDK_CONTROL_MASK )
                {
                    initiate_quit() ;
                    return true ;
                }
        }

        return false ;
    }

    void
    YoukiController::on_status_icon_clicked(
    )
    {
        if( m_main_window->is_visible() )
        {
            m_main_window->get_position( m_main_window_x, m_main_window_y ) ;
            m_main_window->hide () ;
        }
        else
        {
            m_main_window->move( m_main_window_x, m_main_window_y ) ;
            m_main_window->show () ;
            m_main_window->raise () ;
        }
    }

    void
    YoukiController::on_status_icon_scroll_up(
    )
    {
        int volume = m_play->property_volume().get_value() ; 

        volume = std::max( 0, volume - 5 ) ;

        mcs->key_set<int>(
              "mpx"
            ,"volume"
            , volume
        ) ;

        m_play->property_volume().set_value( volume ) ;

        m_main_volume->set_volume(
            volume 
        ) ;
    }

    void
    YoukiController::on_status_icon_scroll_down(
    )
    {
        int volume = m_play->property_volume().get_value() ; 

        volume = std::min( 100, volume + 5 ) ;

        mcs->key_set<int>(
              "mpx"
            ,"volume"
            , volume
        ) ;

        m_play->property_volume().set_value( volume ) ;

        m_main_volume->set_volume(
            volume 
        ) ;
    }

    //// DBUS

    void
    YoukiController::Startup ()
    {
        if( m_main_window )
        {
            m_main_window->present() ;
        }
    }

    void
    YoukiController::Quit ()
    {
    }

    std::map<std::string, DBus::Variant>
    YoukiController::GetMetadata ()
    {
    }

    void
    YoukiController::Next ()
    {
    }

    void
    YoukiController::Prev ()
    {
    }

    void
    YoukiController::Pause ()
    {
    }
}
