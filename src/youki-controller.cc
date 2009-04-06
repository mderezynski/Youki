#include "youki-controller.hh"

#include <glibmm/i18n.h>
#include <boost/format.hpp>

#include <gdk/gdkkeysyms.h>
#include "mpx/mpx-main.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-types.hh"

#include "mpx/mpx-preferences.hh"
#include "mpx/widgets/cairo-extensions.hh"

#include "plugin-manager-gui.hh"

#include "mpx/com/view-album-artist.hh"
#include "mpx/com/view-album.hh"
#include "mpx/com/view-tracks.hh"

#include "mpx/algorithm/youki-markov-predictor.hh"

#include "youki-controller-status-icon.hh"

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

    std::string mpris_attribute_id_str[] =
    {
            "location",
            "title",
            "genre",
            "comment",
            "puid fingerprint",
            "mpx tag hash",
            "mb track id",
            "artist",
            "artist sort name",
            "mb artist id",
            "album",
            "mb album id",
            "mb release date",
            "mb release country",
            "mb release type",
            "asin",
            "album artist",
            "album artist sort name",
            "mb album artist id",
            "mime type",
            "mpx hal volume udi",
            "mpx hal device udi",
            "mpx hal volume relative path",
            "mpx insert path",
            "mpx location name",
    };

    std::string mpris_attribute_id_int[] =
    {
            "tracknumber",
            "time",
            "rating",
            "year",
            "mtime",
            "audio-bitrate",
            "audio-samplerate",
            "mpx play count",
            "mpx play date",
            "mpx insert date",
            "mpx is mb album artist",
            "mpx active",
            "mpx quality",
            "mpx track id",
            "mpx album id",
            "mpx artist id",
            "mpx album artist id",
    };
}

namespace MPX
{
    struct YoukiController::Private
    {
        DataModelFilterArtist_SP_t        FilterModelArtist ;
        DataModelFilterAlbums_SP_t        FilterModelAlbums ;
        DataModelFilterTracks_SP_t        FilterModelTracks ;

        boost::shared_ptr<MarkovTypingPredictor> MarkovPredictor ;

        Private ()
        : MarkovPredictor( new MarkovTypingPredictor )
        {
        }
    } ;

    YoukiController::YoukiController(
        DBus::Connection conn
    )
    : Glib::ObjectBase( "YoukiController" )
    , DBus::ObjectAdaptor( conn, "/info/backtrace/Youki/App" )
    , Service::Base("mpx-service-controller")
    , private_( new Private )
    , m_main_window( 0 )
    {
        m_C_SIG_ID_metadata_updated =
            g_signal_new(
                  "metadata-updated"
                , G_OBJECT_CLASS_TYPE(G_OBJECT_GET_CLASS(gobj()))
                , GSignalFlags(G_SIGNAL_RUN_FIRST)
                , 0
                , NULL
                , NULL
                , g_cclosure_marshal_VOID__VOID
                , G_TYPE_NONE
                , 0
        ) ;

        m_C_SIG_ID_track_cancelled =
            g_signal_new(
                  "track-cancelled"
                , G_OBJECT_CLASS_TYPE(G_OBJECT_GET_CLASS(gobj()))
                , GSignalFlags(G_SIGNAL_RUN_FIRST)
                , 0
                , NULL
                , NULL
                , g_cclosure_marshal_VOID__VOID
                , G_TYPE_NONE
                , 0
        ) ;

        m_mlibman_dbus_proxy = new info::backtrace::Youki::MLibMan_proxy_actual( conn ) ;
        m_mlibman_dbus_proxy->signal_scan_end().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_library_scan_end
        )) ;
        m_mlibman_dbus_proxy->signal_new_album().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_library_new_album
        )) ;

        m_covers    = services->get<Covers>("mpx-service-covers").get() ;
        m_play      = services->get<Play>("mpx-service-play").get() ;
        m_library   = services->get<Library>("mpx-service-library").get() ;

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

        m_NotebookPlugins   = Gtk::manage( new Gtk::Notebook ) ;
        m_NotebookPlugins->property_show_border() = false ;
        m_NotebookPlugins->property_tab_border() = 0 ;
        m_NotebookPlugins->property_tab_pos() = Gtk::POS_BOTTOM ;

        m_Paned1            = Gtk::manage( new Gtk::HPaned ) ;
        m_Paned2            = Gtk::manage( new Gtk::HPaned ) ;

        m_VBox              = Gtk::manage( new Gtk::VBox ) ;

        m_HBox_Entry        = Gtk::manage( new Gtk::HBox ) ;
        m_HBox_Controls     = Gtk::manage( new Gtk::HBox ) ;

        m_Entry             = Gtk::manage( new Gtk::Entry ) ;

        m_completion_timer.stop() ;
        m_completion_timer.reset() ;

        m_Alignment_Entry   = Gtk::manage( new Gtk::Alignment ) ;
        m_Label_Search      = Gtk::manage( new Gtk::Label(_("_Search:"))) ;

        m_ListViewTracks          = Gtk::manage( new ListViewTracks ) ;
        m_ListViewTracks->signal_track_activated().connect(
            sigc::mem_fun(
                      *this
                    , &YoukiController::on_list_view_tr_track_activated
        )) ;

        m_ListViewArtist        = Gtk::manage( new ListViewArtist ) ;
        m_conn1 = m_ListViewArtist->signal_selection_changed().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_aa_selection_changed
        )) ;

        m_ListViewAlbums    = Gtk::manage( new ListViewAlbums ) ;
        m_conn2 = m_ListViewAlbums->signal_selection_changed().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_ab_selection_changed
        )) ;

        m_ScrolledWinArtist     = Gtk::manage( new Gtk::ScrolledWindow ) ;
        m_ScrolledWinAlbums = Gtk::manage( new Gtk::ScrolledWindow ) ;
        m_ScrolledWinTracks = Gtk::manage( new Gtk::ScrolledWindow ) ;

        m_main_window       = new MainWindow ;
        m_main_window->signal_key_press_event().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_main_window_key_press_event
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
        m_disc              = Util::cairo_image_surface_from_pixbuf( Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "disc.png" ))) ;
        m_disc_multiple     = Util::cairo_image_surface_from_pixbuf( Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "disc-multiple.png" ))) ;

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

        m_ScrolledWinArtist->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ; 
        m_ScrolledWinAlbums->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ; 
        m_ScrolledWinTracks->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ; 

        {
                DataModelTracks_SP_t m ( new DataModelTracks ) ;
                private_->FilterModelTracks = DataModelFilterTracks_SP_t (new DataModelFilterTracks( m )) ;

                ColumnP c1 (new Column(_("Title"))) ;
                c1->set_column(0) ;

                ColumnP c2 (new Column(_("Track"))) ;
                c2->set_column(5) ;
                c2->set_alignment( Pango::ALIGN_RIGHT ) ;

                ColumnP c3 (new Column(_("Album"))) ;
                c3->set_column(2) ;

                ColumnP c4 (new Column(_("Artist"))) ;
                c4->set_column(1) ;

                m_ListViewTracks->append_column(c1) ;
                m_ListViewTracks->append_column(c2) ;
                m_ListViewTracks->append_column(c3) ;
                m_ListViewTracks->append_column(c4) ;

                m_ListViewTracks->column_set_fixed(
                      1
                    , true
                    , 60
                ) ;

                m_ListViewTracks->set_model( private_->FilterModelTracks ) ;

                m_ScrolledWinTracks->add( *m_ListViewTracks ) ;
                m_ScrolledWinTracks->show_all() ;

                m_conn3 = m_Entry->signal_changed().connect(
                    sigc::mem_fun(
                          *this
                        , &YoukiController::on_entry_changed__update_completion
                )) ;

                m_conn4 = m_Entry->signal_changed().connect(sigc::bind(
                    sigc::mem_fun(
                          *this
                        , &YoukiController::on_entry_changed__process_filtering)
                        , false
                )) ;
        }

        {
                DataModelArtist_SP_t m (new DataModelArtist) ;
                private_->FilterModelArtist = DataModelFilterArtist_SP_t (new DataModelFilterArtist(m)) ;

                ColumnArtist_SP_t c1 (new ColumnArtist(_("Album Artist"))) ;
                c1->set_column(0) ;

                m_ListViewArtist->append_column(c1) ;
                m_ListViewArtist->set_model( private_->FilterModelArtist ) ;

                m_ScrolledWinArtist->add( *m_ListViewArtist ) ;
                m_ScrolledWinArtist->show_all() ;
        }

        {
                DataModelAlbums_SP_t m ( new DataModelAlbums ) ;
                private_->FilterModelAlbums = DataModelFilterAlbums_SP_t (new DataModelFilterAlbums( m )) ;

                ColumnAlbums_SP_t c1 ( new ColumnAlbums ) ;
                c1->set_column(0) ;

                m_ListViewAlbums->append_column( c1 ) ;
                m_ListViewAlbums->set_model( private_->FilterModelAlbums ) ;

                m_ScrolledWinAlbums->add( *m_ListViewAlbums ) ;
                m_ScrolledWinAlbums->show_all() ;
        }

        m_Entry->signal_key_press_event().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_entry_key_press_event
        )) ;

        m_Entry->signal_activate().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_entry_activated
        )) ;

        m_Paned1->add1( *m_ScrolledWinArtist ) ;
        m_Paned1->add2( *m_ScrolledWinTracks ) ;
        m_Paned2->add1( *m_Paned1 ) ;
        m_Paned2->add2( *m_ScrolledWinAlbums ) ;

        std::vector<Gtk::Widget*> v (2) ;
        v[0] = m_ScrolledWinArtist ;
        v[1] = m_ScrolledWinTracks ;
        m_Paned1->set_focus_chain( v ) ;

        m_HBox_Controls->pack_start( *m_main_position, true, true, 0 ) ;
        m_HBox_Controls->pack_start( *m_main_volume, false, false, 0 ) ;
        m_HBox_Controls->set_spacing( 4 ) ;

        m_main_window->set_widget_top( *m_VBox ) ;
        m_main_window->set_widget_drawer( *m_NotebookPlugins ) ; 

        m_VBox->pack_start( *m_HBox_Entry, false, false, 0 ) ;
        m_VBox->pack_start( *m_Paned2, true, true, 0 ) ;
        m_VBox->pack_start( *m_main_titleinfo, false, false, 0 ) ;
        m_VBox->pack_start( *m_HBox_Controls, false, false, 0 ) ;
        m_VBox->pack_start( *m_main_infoarea, false, false, 0 ) ;

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
        m_ListViewArtist->clear_selection() ;
        m_ListViewAlbums->clear_selection() ;

        m_play->signal_spectrum().connect(
            sigc::mem_fun(
                  *m_main_infoarea
                , &InfoArea::update_spectrum
        )) ;

        m_Paned2->set_position( mcs->key_get<int>("main-window","paned2") ) ;
        while (gtk_events_pending()) gtk_main_iteration() ;

        m_Paned1->set_position( mcs->key_get<int>("main-window","paned1") ) ;
        while (gtk_events_pending()) gtk_main_iteration() ;
    }

    YoukiController::~YoukiController ()
    {
        m_play->request_status( PLAYSTATUS_STOPPED ) ; 

        mcs->key_set<int>("main-window","paned1", m_Paned1->get_position() ) ;
        mcs->key_set<int>("main-window","paned2", m_Paned2->get_position() ) ;

        delete m_control_status_icon ;
        delete m_main_window ;
        m_mlibman_dbus_proxy->Exit () ;
        delete m_mlibman_dbus_proxy ;

        delete private_ ;

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
        using boost::get ;

        DataModelTracks_SP_t m1 ( new DataModelTracks ) ;
        DataModelFilterTracks_SP_t model_tracks = DataModelFilterTracks_SP_t (new DataModelFilterTracks( m1 )) ;

        DataModelArtist_SP_t m2 (new DataModelArtist) ;
        DataModelFilterArtist_SP_t model_album_artists = DataModelFilterArtist_SP_t (new DataModelFilterArtist( m2 )) ;

        DataModelAlbums_SP_t m3 ( new DataModelAlbums ) ;
        DataModelFilterAlbums_SP_t model_albums = DataModelFilterAlbums_SP_t (new DataModelFilterAlbums( m3 )) ;

        // Tracks 

        SQL::RowV v ;
        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM track_view ORDER BY ifnull(album_artist_sortname,album_artist), mb_release_date, album, track_view.track")).str()) ; 
        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
        {
                SQL::Row & r = *i;
                try{
                    model_tracks->append_track_quiet(r, (*(services->get<Library>("mpx-service-library")->sqlToTrack(r, true, false ).get()))) ;
                } catch( Library::FileQualificationError )
                {
                }
        }

        // Album Artists

        model_album_artists->append_artist_quiet(
              _("All Artists") 
            , -1
        ) ;

        v.clear () ; 
        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT * FROM album_artist ORDER BY album_artist")).str()) ; 

        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
        {
                SQL::Row & r = *i;
                model_album_artists->append_artist_quiet(
                      r.count("album_artist_sortname") ? boost::get<std::string>(r["album_artist_sortname"]) : boost::get<std::string>(r["album_artist"])
                    , boost::get<gint64>(r["id"])
                ) ;
        }

        // Albums

        model_albums->append_album_quiet(
              m_disc_multiple 
            , -1
            , -1
            , ""
            , _("All Albums")
        ) ;

        v.clear () ; 
        services->get<Library>("mpx-service-library")->getSQL(v, (boost::format("SELECT album, album.mb_album_id, album.id, album_artist.id AS album_artist_id, album_artist, album_artist_sortname FROM album JOIN album_artist ON album.album_artist_j = album_artist.id ORDER BY ifnull(album_artist_sortname,album_artist), mb_release_date, album")).str()) ; 

        for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
        {
                SQL::Row & r = *i;

                const std::string& mbid = get<std::string>(r["mb_album_id"]) ;

                Glib::RefPtr<Gdk::Pixbuf> cover_pb ;
                Cairo::RefPtr<Cairo::ImageSurface> cover_is ;

                services->get<Covers>("mpx-service-covers")->fetch(
                      mbid
                    , cover_pb
                ) ;
               
                if( cover_pb ) 
                {
                    cover_is = Util::cairo_image_surface_from_pixbuf(
                        cover_pb->scale_simple( 64, 64, Gdk::INTERP_BILINEAR )
                    ) ;
                }
                else
                {
                    cover_is = m_disc ; 
                }
                
                model_albums->append_album_quiet(
                      cover_is
                    , get<gint64>(r["id"])
                    , get<gint64>(r["album_artist_id"])
                    , get<std::string>(r["album"])
                    , r.count("album_artist_sortname") ? get<std::string>(r["album_artist_sortname"]) : get<std::string>(r["album_artist"])
                ) ;
        }

        boost::optional<gint64> id_albums = m_ListViewAlbums->get_selected() ;
        boost::optional<gint64> id_artist = m_ListViewArtist->get_selected() ;

        private_->FilterModelArtist = model_album_artists ;
        private_->FilterModelAlbums = model_albums ;
        private_->FilterModelTracks = model_tracks ;

        m_ListViewArtist->set_model( private_->FilterModelArtist ) ;
        m_ListViewAlbums->set_model( private_->FilterModelAlbums ) ;
        m_ListViewTracks->set_model( private_->FilterModelTracks ) ; 

        boost::optional<MPX::Track> t = m_track_current ;
        if( t )
        {
            m_ListViewTracks->set_active_track( boost::get<gint64>(t.get()[ATTRIBUTE_MPX_TRACK_ID].get()) ) ;
        }

        on_entry_changed__process_filtering() ;

        m_ListViewArtist->select_id( id_artist ) ;
        m_ListViewAlbums->select_id( id_albums ) ;
    }

    void
    YoukiController::on_library_scan_end(
    )
    {
        reload_library () ;
    }

    void
    YoukiController::on_library_new_album(
          gint64                id
        , const std::string&    s1
        , const std::string&    s2
        , const std::string&    s3
        , const std::string&    s4
        , const std::string&    s5
    )
    {
        RequestQualifier rq ; 

        rq.mbid     =   s1 ;
        rq.asin     =   s2 ;
        rq.uri      =   s3 ;
        rq.artist   =   s4 ;
        rq.album    =   s5 ;

        m_covers->cache( rq, true ) ;
    }

////////////////

    void
    YoukiController::play_track(
        MPX::Track& t
    )
    {
        try{
                boost::shared_ptr<Library> library = services->get<Library>("mpx-service-library") ;
       
                m_track_previous    = m_track_current ;
                m_track_current     = t ; 

                m_play->switch_stream( library->trackGetLocation( t ) ) ;

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
        if( position < 0 )
            return ;

        gint64 duration = m_play->property_duration().get_value() ;
    
        if( duration < 0 )
            return ;

        if( m_seek_position && guint64(position) >= m_seek_position.get() ) 
        {
            m_seek_position.reset() ; 
        }

        m_main_position->set_position( duration, position ) ;
        m_control_status_icon->set_position( duration, position ) ;
    }

    void
    YoukiController::on_play_eos ()
    {
        if( m_next_track_queue_id )
        {
            SQL::RowV v ;
            m_library->getSQL(v, (boost::format("SELECT * FROM track_view WHERE id = '%lld'") % m_next_track_queue_id.get() ).str()) ; 
            Track_sp p = m_library->sqlToTrack( v[0], true, false ) ;
            m_next_track_queue_id.reset() ;
            play_track( *(p.get()) ) ;
        }
        else
        {
                boost::optional<gint64> pos = m_ListViewTracks->get_local_active_track () ;

                if( pos )
                {
                    gint64 pos_cpy = pos.get() + 1 ;

                    if( pos_cpy < private_->FilterModelTracks->size() )
                    {
                        play_track( boost::get<4>(private_->FilterModelTracks->row(pos_cpy)) ) ;
                        return ;
                    }
                }
                else
                if( private_->FilterModelTracks->size() )
                {
                    play_track( boost::get<4>(private_->FilterModelTracks->row( 0 )) ) ;
                    return ;
                }

                m_play->request_status( PLAYSTATUS_STOPPED ) ; 

                if( m_track_previous )
                {
                        m_library->trackPlayed(
                              boost::get<gint64>(m_track_previous.get()[ATTRIBUTE_MPX_TRACK_ID].get())
                            , boost::get<gint64>(m_track_previous.get()[ATTRIBUTE_MPX_ALBUM_ID].get())
                            , time(NULL)
                        ) ;

                        m_track_previous.reset() ;
                }
        }
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
                break ;

            case PLAYSTATUS_STOPPED:

                m_track_current.reset() ;

                m_main_titleinfo->clear() ;
                m_main_window->queue_draw () ;    
                m_ListViewTracks->clear_active_track() ;
                m_main_cover->clear() ;
                m_control_status_icon->set_image( Glib::RefPtr<Gdk::Pixbuf>(0) ) ;
                m_main_position->set_position( 0, 0 ) ;
                m_seek_position.reset() ; 
                m_playqueue.clear() ;
                break ;

            case PLAYSTATUS_WAITING:

                m_track_current.reset() ;

                m_main_titleinfo->clear() ;
                m_main_window->queue_draw () ;    
                m_seek_position.reset() ; 
                break ;

            case PLAYSTATUS_PAUSED:
                break ;

            default: break ;
        }
    }

    void
    YoukiController::on_play_stream_switched()
    {
        boost::optional<MPX::Track> t = m_track_current ;

        g_return_if_fail( bool(t) ) ;

        m_ListViewTracks->set_active_track( boost::get<gint64>(t.get()[ATTRIBUTE_MPX_TRACK_ID].get()) ) ;
        m_control_status_icon->set_metadata( t.get() ) ;

        if( t.get().has( ATTRIBUTE_MB_ALBUM_ID ) )
        {
                const std::string& mbid = boost::get<std::string>(t.get()[ATTRIBUTE_MB_ALBUM_ID].get()) ;

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
                if( t.get().has( n ) )
                    info.push_back( boost::get<std::string>(t.get()[id[n]].get()) ) ;
        }

        m_main_titleinfo->set_info( info ) ;

        g_signal_emit(
              G_OBJECT(gobj())
            , m_C_SIG_ID_metadata_updated
            , 0
        ) ;

        if( m_track_previous )
        {
                m_library->trackPlayed(
                      boost::get<gint64>(m_track_previous.get()[ATTRIBUTE_MPX_TRACK_ID].get())
                    , boost::get<gint64>(m_track_previous.get()[ATTRIBUTE_MPX_ALBUM_ID].get())
                    , time(NULL)
                ) ;

                private_->MarkovPredictor->process_string(
                      boost::get<std::string>(m_track_previous.get()[ATTRIBUTE_ALBUM_ARTIST].get())
                ) ;
        
                m_track_previous.reset() ;
        }

        m_playqueue.push_back( boost::get<gint64>(t.get()[ATTRIBUTE_MPX_TRACK_ID].get()) ) ;
    }

    void
    YoukiController::on_list_view_tr_track_activated(
          MPX::Track    t 
        , bool          play
    ) 
    {
        g_signal_emit(
              G_OBJECT(gobj())
            , m_C_SIG_ID_track_cancelled
            , 0
        ) ;

        if( play )
        {
            play_track( t ) ;
        }
        else
        {
            m_next_track_queue_id = boost::get<gint64>(t[ATTRIBUTE_MPX_TRACK_ID].get()) ; 
        }
    }

    void
    YoukiController::on_list_view_aa_selection_changed(
    ) 
    {
        private_->FilterModelTracks->clear_synthetic_constraints_quiet() ;

        //// SET UP CONSTRAINTS

        boost::optional<gint64> id_artist = m_ListViewArtist->get_selected() ;

        boost::optional<std::set<gint64> > constraint ; 

        if( id_artist ) 
        {
            AQE::Constraint_t c ;
            c.TargetAttr = ATTRIBUTE_MPX_ALBUM_ARTIST_ID ;
            c.TargetValue = id_artist.get() ;
            c.MatchType = AQE::MT_EQUAL ;
            private_->FilterModelTracks->add_synthetic_constraint_quiet( c ) ;

            constraint = std::set<gint64>() ; 
            constraint.get().insert( id_artist.get() ) ;
        }

        private_->FilterModelAlbums->set_constraint_artist( constraint ) ;
        private_->FilterModelAlbums->regen_mapping() ;
        private_->FilterModelTracks->regen_mapping() ;
    }

    void
    YoukiController::on_list_view_ab_selection_changed(
    ) 
    {
        private_->FilterModelTracks->clear_synthetic_constraints_quiet() ;

        boost::optional<gint64> id_artist = m_ListViewArtist->get_selected() ;
        boost::optional<gint64> id_albums = m_ListViewAlbums->get_selected() ;

        if( id_albums ) 
        {
            AQE::Constraint_t c ;
            c.TargetAttr = ATTRIBUTE_MPX_ALBUM_ID ;
            c.TargetValue = id_albums.get() ;
            c.MatchType = AQE::MT_EQUAL ;
            private_->FilterModelTracks->add_synthetic_constraint_quiet( c ) ;
        }

        if( id_artist ) 
        {
            AQE::Constraint_t c ;
            c.TargetAttr = ATTRIBUTE_MPX_ALBUM_ARTIST_ID ;
            c.TargetValue = id_artist.get() ;
            c.MatchType = AQE::MT_EQUAL ;
            private_->FilterModelTracks->add_synthetic_constraint_quiet( c ) ;
        }

        private_->FilterModelTracks->regen_mapping() ;
    }

    void
    YoukiController::on_entry_activated(
    )
    {
        if( m_prediction_last.length() )
        {
            m_conn3.block() ;
            m_conn4.block() ;
            m_Entry_Text += m_prediction_last ;
            m_Entry->select_region( -1, -1 ) ;
            m_conn3.unblock() ;
            m_conn4.unblock() ;
        }

        on_entry_changed__process_filtering() ;
    }

    bool
    YoukiController::on_entry_key_press_event(
          GdkEventKey* event
    ) 
    {
        switch( event->keyval )
        {
            case GDK_KP_Enter:
            case GDK_ISO_Enter:
            case GDK_3270_Enter:
            case GDK_Return:
            {
                if( event->state & GDK_CONTROL_MASK )  
                {
                    if( private_->FilterModelTracks->size() )
                    {
                        play_track( boost::get<4>(private_->FilterModelTracks->row( m_ListViewTracks->get_upper_row()  )) ) ;
                    }
                }
                return true ;
            }

            default: break ;
        }

        return false ;
    }

    bool
    YoukiController::completion_timer_check(
    ) 
    {
        if( m_completion_timer.elapsed() > 0.25 ) 
        {
            m_completion_timer.stop() ;
            m_completion_timer.reset() ;

            m_conn3.block() ;
            m_conn4.block() ;

            m_Entry->set_text( m_Entry_Text + m_prediction ) ; 
            m_Entry->select_region( m_Entry_Text.length(), -1 ) ;

            m_prediction_last = m_prediction ;
            m_prediction.clear() ;

            m_conn3.unblock() ;
            m_conn4.unblock() ;

            on_entry_changed__process_filtering() ;

            return false ;
        }
        if( m_completion_timer.elapsed() < 0 ) 
            return false ;

        return true ;
    }

    void
    YoukiController::on_entry_changed__update_completion()
    {
        m_completion_timer.stop() ;
        m_completion_timer.reset() ;

        Glib::ustring text = m_Entry->get_text() ;

        if( text.size() > 0 )
        {
            if( text == m_Entry_Text + m_prediction_last ) 
            {
                return ;
            }

            Glib::ustring new_prediction = private_->MarkovPredictor->predict( text ) ;

            if( text == m_Entry_Text && new_prediction == m_prediction_last ) 
            {
                m_prediction.clear() ;
                m_prediction_last.clear() ;
                return ;
            }

            m_Entry_Text = text ; 
            m_prediction = new_prediction ; 

            if( !m_prediction.empty() && !m_conn_completion ) 
            {
                m_conn_completion = Glib::signal_timeout().connect(
                    sigc::mem_fun(
                          *this
                        , &YoukiController::completion_timer_check
                    )
                    , 50
                ) ;
            }

            m_completion_timer.start() ;
        }
        else
        {
            m_Entry_Text.clear() ;
            m_prediction.clear() ;
            m_prediction_last.clear() ;
        }
    }

    void
    YoukiController::on_entry_changed__process_filtering(
        bool forced
    )
    {
        private_->FilterModelTracks->clear_synthetic_constraints_quiet() ;

        private_->FilterModelTracks->set_filter( m_Entry_Text ) ;

        private_->FilterModelArtist->set_constraint_artist( private_->FilterModelTracks->m_constraint_artist ) ;
        private_->FilterModelAlbums->set_constraint_albums( private_->FilterModelTracks->m_constraint_albums ) ;
        private_->FilterModelAlbums->set_constraint_artist( private_->FilterModelTracks->m_constraint_artist ) ;

        private_->FilterModelArtist->regen_mapping() ;
        private_->FilterModelAlbums->regen_mapping() ;
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

    void
    YoukiController::on_info_area_clicked ()
    {
        API_pause_toggle() ;
    }

    bool
    YoukiController::on_main_window_key_press_event(
        GdkEventKey* event
    ) 
    {
        switch( event->keyval )
        {
            case GDK_Escape:
                m_main_window->hide() ;
                return true ;

            case GDK_F1:
                m_mlibman_dbus_proxy->ShowWindow () ;
                return true ;

            case GDK_F2:
                services->get<Preferences>("mpx-service-preferences")->present () ;
                return true ;

            case GDK_F3:
                services->get<PluginManagerGUI>("mpx-service-plugins-gui")->present () ;
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
    YoukiController::Present ()
    {
        if( m_main_window )
        {
            m_main_window->present() ;
        }
    }

    void
    YoukiController::Startup ()
    {
    }

    void
    YoukiController::Quit ()
    {
    }

    std::map<std::string, DBus::Variant>
    YoukiController::GetMetadata ()
    {
        boost::optional<MPX::Track> t = m_track_current ;

        if( !t )
        {
            return std::map<std::string, DBus::Variant>() ;
        }

        std::map<std::string, DBus::Variant> m ;

        for(int n = ATTRIBUTE_LOCATION; n < N_ATTRIBUTES_STRING; ++n)
        {
                if(t.get()[n].is_initialized())
                {
                        DBus::Variant val ;
                        DBus::MessageIter iter = val.writer() ;
                        std::string v = boost::get<std::string>(t.get()[n].get()) ; 
                        iter << v ;
                        m[mpris_attribute_id_str[n-ATTRIBUTE_LOCATION]] = val ;
                }
        }

        for(int n = ATTRIBUTE_TRACK; n < ATTRIBUTE_MPX_ALBUM_ARTIST_ID; ++n)
        {
                if(t.get()[n].is_initialized())
                {
                        DBus::Variant val ;
                        DBus::MessageIter iter = val.writer() ;
                        int64_t v = boost::get<gint64>(t.get()[n].get()) ;
                        iter << v ;
                        m[mpris_attribute_id_int[n-ATTRIBUTE_TRACK]] = val ;
                }
        }

        return m ;
    }

    void
    YoukiController::Next ()
    {
        API_next() ;
    }

    void
    YoukiController::Prev ()
    {
        API_prev() ;
    }

    void
    YoukiController::Pause ()
    {
        API_pause_toggle() ;
    }

    void
    YoukiController::queue_next_track(
          gint64 id
    )
    {
        m_next_track_queue_id = id ;
    }

    PlayStatus
    YoukiController::get_status(
    )
    {
        return PlayStatus( m_play->property_status().get_value() ) ;
    }

    MPX::Track&
    YoukiController::get_metadata(
    )
    {
        if( m_track_current )
            return m_track_current.get() ;
        else
            throw std::runtime_error("No current track!") ;
    }

    void
    YoukiController::API_pause_toggle(
    )
    {
        if( m_track_current )
        {
                PlayStatus s = PlayStatus(m_play->property_status().get_value()) ;

                if( s == PLAYSTATUS_PAUSED )
                    m_play->request_status( PLAYSTATUS_PLAYING ) ;
                else
                if( s == PLAYSTATUS_PLAYING )
                    m_play->request_status( PLAYSTATUS_PAUSED ) ;
        }
    }

    void
    YoukiController::API_next(
    )
    {
        if( m_track_current )
        {
            on_play_eos() ;
        }
    }

    void
    YoukiController::API_prev(
    )
    {
        if( m_track_current )
        {
                if( m_playqueue.size() > 1 )
                {
                    m_playqueue.pop_back() ;

                    SQL::RowV v ;
                    m_library->getSQL(v, (boost::format("SELECT * FROM track_view WHERE id = '%lld'") % m_playqueue.back() ).str()) ; 
                    Track_sp p = m_library->sqlToTrack( v[0], true, false ) ;
                    play_track( *(p.get()) ) ;
                }
        }
    }
}
