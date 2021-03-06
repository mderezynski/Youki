#include "config.h"
//
#include "youki-controller.hh"
#include "youki-controller-status-icon.hh"

#include <glibmm/i18n.h>
#include <gdk/gdkkeysyms.h>
#include <boost/format.hpp>

#include "mpx/mpx-main.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-string.hh"

#include "mpx/com/view-album-artist.hh"
#include "mpx/com/view-album.hh"
#include "mpx/com/view-tracks.hh"

#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/widgets/percentual-distribution-hbox.hh"
#include "mpx/widgets/rounded-frame.hh"

#include "mpx/i-youki-theme-engine.hh"

#include "library.hh"
#include "plugin-manager-gui.hh"
#include "play.hh"
#include "preferences.hh"

namespace
{
    std::string
    mpris_attribute_id_str[] =
    {
           "location"
         , "title"
         , "genre"
         , "comment"
         , "label"
         , "puid fingerprint"
         , "mpx tag hash"
         , "mb track id"
         , "artist"
         , "artist sort name"
         , "mb artist id"
         , "album"
         , "mb album id"
         , "mb release date"
         , "mb release country"
         , "mb release type"
         , "asin"
         , "album artist"
         , "album artist sort name"
         , "mb album artist id"
         , "mime type"
         , "mpx hal volume udi"
         , "mpx hal device udi"
         , "mpx hal volume relative path"
         , "mpx insert path"
         , "mpx location name"
    };

    std::string
    mpris_attribute_id_int[] =
    {
            "tracknumber"
          , "time"
          , "rating"
          , "year"
          , "mtime"
          , "audio-bitrate"
          , "audio-samplerate"
          , "mpx play count"
          , "mpx play date"
          , "mpx insert date"
          , "mpx is mb album artist"
          , "mpx active"
          , "mpx quality"
          , "mpx device id"
          , "mpx track id"
          , "mpx album id"
          , "mpx artist id"
          , "mpx album artist id"
    };

    bool
    CompareAlbumArtists(
          const MPX::SQL::Row&   r1
        , const MPX::SQL::Row&   r2
    )
    {
        return Glib::ustring(::MPX::Util::row_get_album_artist_name( r1 )).casefold() < Glib::ustring(::MPX::Util::row_get_album_artist_name( r2 )).casefold() ;
    }

    void
    handle_sql_error( MPX::SQL::SqlGenericError & cxe )
    {
        Gtk::Dialog dialog ("SQL Error", true, false ) ;
        Gtk::Label label ;
        label.set_text( cxe.what() ) ;
        dialog.get_vbox()->pack_start( label, true, true ) ;
        dialog.add_button("OK", Gtk::RESPONSE_DELETE_EVENT) ;
        dialog.show_all() ; 
        dialog.run() ;
        std::abort() ;
    }
}

namespace MPX
{
    struct YoukiController::Private
    { 
        View::Artist::DataModelFilter_sp_t  FilterModelArtist ;
        View::Albums::DataModelFilter_sp_t  FilterModelAlbums ;
        View::Tracks::DataModelFilter_sp_t  FilterModelTracks ;

    } ;

    YoukiController::YoukiController(
        DBus::Connection conn
    )
    : Glib::ObjectBase( "YoukiController" )
    , DBus::ObjectAdaptor( conn, "/info/backtrace/Youki/App" )
    , Service::Base("mpx-service-controller")
    , private_( new Private )
    , m_main_window( 0 )
    , m_follow_track( false )
    {
        m_C_SIG_ID_track_new =
            g_signal_new(
                  "track-new"
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

        m_C_SIG_ID_track_out =
            g_signal_new(
                  "track-out"
                , G_OBJECT_CLASS_TYPE(G_OBJECT_GET_CLASS(gobj()))
                , GSignalFlags(G_SIGNAL_RUN_FIRST)
                , 0
                , NULL
                , NULL
                , g_cclosure_marshal_VOID__VOID
                , G_TYPE_NONE
                , 0
        ) ;

        m_covers    = services->get<Covers>("mpx-service-covers").get() ;
        m_play      = services->get<Play>("mpx-service-play").get() ;
        m_library   = services->get<Library>("mpx-service-library").get() ;

        m_mlibman_dbus_proxy = services->get<info::backtrace::Youki::MLibMan_proxy_actual>("mpx-service-mlibman").get() ; 

        m_mlibman_dbus_proxy->signal_scan_start().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_library_scan_start
        )) ;

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

        m_mlibman_dbus_proxy->signal_new_artist().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_library_new_artist
        )) ;

        m_mlibman_dbus_proxy->signal_new_track().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_library_new_track
        )) ;

        m_mlibman_dbus_proxy->signal_entity_deleted().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_library_entity_deleted
        )) ;

        m_mlibman_dbus_proxy->signal_entity_updated().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_library_entity_updated
        )) ;


        m_library->signal_album_updated().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_local_library_album_updated
        )) ;

        m_covers->signal_got_cover().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_covers_got_cover
        )) ;

        m_play->signal_eos().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_eos
        )) ;

        m_play->signal_seek().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_seek
        )) ;

        m_play->signal_position().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_position
        )) ;

        m_play->property_status().signal_changed().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_playstatus_changed
        )) ;

        m_play->signal_stream_switched().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_stream_switched
        )) ;

        m_play->signal_metadata().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_play_metadata
        )) ;


        m_NotebookPlugins   = Gtk::manage( new Gtk::Notebook ) ;
        m_NotebookPlugins->property_show_border() = false ;
        m_NotebookPlugins->property_tab_border() = 0 ;
        m_NotebookPlugins->property_tab_pos() = Gtk::POS_BOTTOM ;

        m_HBox_Main         = Gtk::manage( new PercentualDistributionHBox ) ;
        m_VBox              = Gtk::manage( new Gtk::VBox ) ;
        m_HBox_Entry        = Gtk::manage( new Gtk::HBox ) ;
        m_HBox_Info         = Gtk::manage( new Gtk::HBox ) ;
        m_HBox_Controls     = Gtk::manage( new Gtk::HBox ) ;
        m_HBox_Bottom       = Gtk::manage( new Gtk::HBox ) ;
        m_VBox_Bottom       = Gtk::manage( new Gtk::VBox ) ;
       
        m_HBox_Main->set_spacing( 8 ) ; 
        m_VBox->set_spacing( 6 ) ;
        m_VBox_Bottom->set_spacing( 2 ) ;
        m_HBox_Bottom->set_spacing( 4 ) ;
        m_HBox_Controls->set_spacing( 2 ) ;
        m_HBox_Info->set_spacing( 2 ) ; 

        m_Entry             = Gtk::manage( new Gtk::Entry ) ;
        m_Entry->set_icon_from_pixbuf(
              Gtk::IconTheme::get_default()->load_icon( "mpx-stock-entry-clear", 16 ) 
            , Gtk::ENTRY_ICON_PRIMARY
        ) ; 
        m_Entry->signal_icon_press().connect(
              sigc::hide( sigc::hide(
                sigc::mem_fun(
                      *this
                    , &YoukiController::on_entry_clear_clicked
        )))) ;

        m_Alignment_Entry   = Gtk::manage( new Gtk::Alignment ) ;
        m_Label_Search      = Gtk::manage( new Gtk::Label(_("_Search:"))) ;

        m_ListViewTracks        = Gtk::manage( new View::Tracks::Class ) ;
        m_ListViewArtist        = Gtk::manage( new View::Artist::Class ) ;
        m_ListViewAlbums        = Gtk::manage( new View::Albums::Class ) ;

        m_ScrolledWinArtist = Gtk::manage( new Gtk::ScrolledWindow ) ;
        m_ScrolledWinAlbums = Gtk::manage( new Gtk::ScrolledWindow ) ;
        m_ScrolledWinTracks = Gtk::manage( new Gtk::ScrolledWindow ) ;

        m_ScrolledWinArtist->set_shadow_type( Gtk::SHADOW_NONE ) ;
        m_ScrolledWinAlbums->set_shadow_type( Gtk::SHADOW_NONE ) ;
        m_ScrolledWinTracks->set_shadow_type( Gtk::SHADOW_NONE ) ;

        m_main_window       = new MainWindow ;
        m_main_window->signal_key_press_event().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_main_window_key_press_event_pre
        ), false ) ;
        m_main_window->signal_key_press_event().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_main_window_key_press_event_after
        ), true ) ;

        m_main_window->signal_quit().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::initiate_quit
        )) ;

        Gdk::Color background ;
        background.set_rgb_p( 0.1, 0.1, 0.1 ) ;

        m_main_position     = Gtk::manage( new KoboPosition ) ;
        m_main_position->signal_seek_event().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_position_seek
        )) ;

        m_main_spectrum_titleinfo = new YoukiSpectrumTitleinfo ;
        m_main_spectrum_titleinfo->signal_clicked().connect(
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

        m_icon = Gdk::Pixbuf::create_from_file( Glib::build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "youki.png" )) ;
        m_main_window->set_icon( m_icon ) ;

        m_Label_Search->set_mnemonic_widget( *m_Entry ) ;
        m_Label_Search->set_use_underline() ;

        m_VBox->property_spacing() = 4 ; 

/*
        m_Button_STOP = new Gtk::Button ;
        Gtk::Image * img = Gtk::manage( new Gtk::Image( Gtk::Stock::MEDIA_STOP, Gtk::ICON_SIZE_BUTTON )) ;
        m_Button_STOP->add( *img ) ;

        m_Button_PREV = new Gtk::Button ;
        img = Gtk::manage( new Gtk::Image( Gtk::Stock::MEDIA_PREVIOUS, Gtk::ICON_SIZE_BUTTON )) ;
        m_Button_PREV->add( *img ) ;

        m_Button_NEXT = new Gtk::Button ;
        img = Gtk::manage( new Gtk::Image( Gtk::Stock::MEDIA_NEXT, Gtk::ICON_SIZE_BUTTON )) ;
        m_Button_NEXT->add( *img ) ;
   
        m_Button_PREV->set_relief( Gtk::RELIEF_NONE ) ; 
        m_Button_STOP->set_relief( Gtk::RELIEF_NONE ) ; 
        m_Button_NEXT->set_relief( Gtk::RELIEF_NONE ) ; 

        Gtk::VSeparator * sep = Gtk::manage( new Gtk::VSeparator ) ;
*/

        m_HBox_Entry->property_spacing() = 4 ; 
        m_HBox_Entry->set_border_width( 2 ) ;
/*
        m_HBox_Entry->pack_start( *m_Button_PREV, false, true, 0 );
        m_HBox_Entry->pack_start( *m_Button_STOP, false, true, 0 );
        m_HBox_Entry->pack_start( *m_Button_NEXT, false, true, 0 );
        m_HBox_Entry->pack_start( *sep, false, false, 0 ) ;
*/
        m_HBox_Entry->pack_start( *m_Label_Search, false, false, 0 ) ;
        m_HBox_Entry->pack_start( *m_Alignment_Entry, true, true, 0 ) ;

        m_Alignment_Entry->add( *m_Entry ) ;
        m_Alignment_Entry->property_top_padding() = 2 ;
        m_Alignment_Entry->property_bottom_padding() = 2 ;
        m_Alignment_Entry->property_left_padding() = 2 ;
        m_Alignment_Entry->property_right_padding() = 2 ;

        on_style_changed() ;

        m_main_window->signal_style_changed().connect(
            sigc::hide(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_style_changed
        ))) ;

        m_ScrolledWinArtist->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ; 
        m_ScrolledWinAlbums->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ; 
        m_ScrolledWinTracks->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ; 

        {
                View::Tracks::DataModel_sp_t m ( new View::Tracks::DataModel ) ;
                private_->FilterModelTracks = View::Tracks::DataModelFilter_sp_t (new View::Tracks::DataModelFilter( m )) ;

                // Tracks 

                using boost::get ;

                SQL::RowV v ;
                m_library->getSQL(v, (boost::format("SELECT * FROM track_view ORDER BY ifnull(album_artist_sortname,album_artist), mb_release_date, album, track_view.track")).str()) ; 
                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                {
                        SQL::Row & r = *i;
                        try{
                            private_->FilterModelTracks->append_track(r, m_library->sqlToTrack(r, true, false ) ) ;
                        } catch( Library::FileQualificationError )
                        {
                        }
                }

                m_ListViewTracks->set_model( private_->FilterModelTracks ) ; 

                View::Tracks::Column_sp_t c1 (new View::Tracks::Column(_("Track"))) ;
                c1->set_column(5) ;
                c1->set_alignment( Pango::ALIGN_RIGHT ) ;

                View::Tracks::Column_sp_t c2 (new View::Tracks::Column(_("Title"))) ;
                c2->set_column(0) ;

                View::Tracks::Column_sp_t c3 (new View::Tracks::Column(_("Time"))) ;
                c3->set_column(9) ;
                c3->set_alignment( Pango::ALIGN_RIGHT ) ;

                View::Tracks::Column_sp_t c4 (new View::Tracks::Column(_("Artist"))) ;
                c4->set_column(1) ;

                View::Tracks::Column_sp_t c5 (new View::Tracks::Column(_("Album"))) ;
                c5->set_column(2) ;

                m_ListViewTracks->append_column(c1) ;
                m_ListViewTracks->append_column(c2) ;
                m_ListViewTracks->append_column(c3) ;
                m_ListViewTracks->append_column(c4) ;
                m_ListViewTracks->append_column(c5) ;

                m_ListViewTracks->column_set_fixed(
                      0
                    , true
                    , 60
                ) ;

                m_ListViewTracks->column_set_fixed(
                      2
                    , true
                    , 60
                ) ;

                RoundedFrame * frame = Gtk::manage( new RoundedFrame ) ;
                frame->add( *m_ListViewTracks ) ;

                m_ScrolledWinTracks->add( *frame ) ;
                m_ScrolledWinTracks->show_all() ;

                m_conn4 = m_Entry->signal_changed().connect(
                    sigc::mem_fun(
                          *this
                        , &YoukiController::on_entry_changed
                )) ;
        }

        {
                View::Artist::DataModel_sp_t m (new View::Artist::DataModel) ;
                private_->FilterModelArtist = View::Artist::DataModelFilter_sp_t (new View::Artist::DataModelFilter( m )) ;
                View::Artist::Column_sp_t c1 (new View::Artist::Column(_("Album Artist"))) ;
                c1->set_column(0) ;
                m_ListViewArtist->append_column(c1) ;

                private_->FilterModelArtist->append_artist("",-1);

                SQL::RowV v ;
                m_library->getSQL(v, (boost::format("SELECT * FROM album_artist")).str()) ; 
                std::stable_sort( v.begin(), v.end(), CompareAlbumArtists ) ;
                for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                {
                    SQL::Row & r = *i;

                    private_->FilterModelArtist->append_artist(
                          Util::row_get_album_artist_name( r )
                        , boost::get<gint64>(r["id"])
                    ) ;
                }

                m_ListViewArtist->set_model( private_->FilterModelArtist ) ;

                RoundedFrame * frame = Gtk::manage( new RoundedFrame ) ;
                frame->add( *m_ListViewArtist ) ;

                m_ScrolledWinArtist->add( *frame ) ;
                m_ScrolledWinArtist->show_all() ;
        }

        {
                View::Albums::DataModel_sp_t m ( new View::Albums::DataModel ) ;
                private_->FilterModelAlbums = View::Albums::DataModelFilter_sp_t (new View::Albums::DataModelFilter( m )) ;

                View::Albums::Column_sp_t c1 ( new View::Albums::Column ) ;
                c1->set_column(0) ;

                m_ListViewAlbums->append_column( c1 ) ;



                // our "All Albums" entry: FIXME: Don't do this but manage it inside the model
                MPX::View::Albums::Album_sp dummy_album ( new MPX::View::Albums::Album ) ;
                dummy_album->album_id = -1 ;
                private_->FilterModelAlbums->append_album( dummy_album ) ;


                SQL::RowV v ;
                try{
                    m_library->getSQL( v, "SELECT album.id AS id, album_artist.mb_album_artist_id AS mbid_artist FROM album JOIN album_artist "
                                          "ON album.album_artist_j = album_artist.id ORDER BY "
                                          "ifnull(album_artist_sortname,album_artist), mb_release_date, album"
                    ) ; 
                } catch (MPX::SQL::SqlGenericError & cxe )
                {
                    handle_sql_error( cxe ) ;
                }

                for( SQL::RowV::iterator i = v.begin(); i != v.end(); ++i )
                {
                    gint64 id = get<gint64>((*i)["id"]); 
                    try{
                        private_->FilterModelAlbums->append_album( get_album_from_id( id )) ;
                    } catch( std::logic_error )
                    {
                        g_message("Ooops") ;
                    }
                }

                m_ListViewAlbums->set_model( private_->FilterModelAlbums ) ;

                RoundedFrame * frame = Gtk::manage( new RoundedFrame ) ;
                frame->add( *m_ListViewAlbums ) ;

                m_ScrolledWinAlbums->add( *frame ) ;
                m_ScrolledWinAlbums->show_all() ;
        }

        SQL::RowV v ;

        m_library->getSQL(v, (boost::format("SELECT max(id) AS id FROM album_artist")).str()) ; 
        gint64 max_artist = boost::get<gint64>(v[0]["id"]) ;

        v.clear();

        m_library->getSQL(v, (boost::format("SELECT max(id) AS id FROM album")).str()) ; 
        gint64 max_albums = boost::get<gint64>(v[0]["id"]) ;

        private_->FilterModelTracks->set_sizes( max_artist, max_albums ) ;

        private_->FilterModelArtist->regen_mapping() ;
        private_->FilterModelAlbums->regen_mapping() ;
        private_->FilterModelTracks->regen_mapping() ;

        m_ListViewTracks->signal_track_activated().connect(
            sigc::mem_fun(
                      *this
                    , &YoukiController::on_list_view_tr_track_activated
        )) ;
        m_ListViewTracks->signal_find_propagate().connect(
            sigc::mem_fun(
                      *this
                    , &YoukiController::on_list_view_tr_find_propagate
        )) ;

        m_ListViewTracks->signal_only_this_album_mbid().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_ab_select_album
        )) ;

        m_ListViewTracks->signal_only_this_artist_mbid().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_ab_select_artist
        )) ;

        m_conn1 = m_ListViewArtist->signal_selection_changed().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_aa_selection_changed
        )) ;

        m_ListViewArtist->signal_find_accepted().connect(
            sigc::mem_fun(
                  *m_ListViewAlbums
                , &Gtk::Widget::grab_focus
        )) ;

        m_ListViewAlbums->signal_find_accepted().connect(
            sigc::mem_fun(
                  *m_ListViewTracks
                , &Gtk::Widget::grab_focus
        )) ;

        m_conn2 = m_ListViewAlbums->signal_selection_changed().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_ab_selection_changed
        )) ;

        m_ListViewAlbums->signal_only_this_album_mbid().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_ab_select_album
        )) ;

        m_ListViewAlbums->signal_only_this_artist_mbid().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_list_view_ab_select_artist
        )) ;

        m_ListViewAlbums->signal_start_playback().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_ab_start_playback
        )) ;

        m_Entry->signal_key_press_event().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_entry_key_press_event
        ), false ) ;

        m_Entry->signal_activate().connect(
            sigc::mem_fun(
                  *this
                , &YoukiController::on_entry_activated
        )) ;

        m_HBox_Main->add_percentage( 0.15 ) ;
        m_HBox_Main->add_percentage( 0.25 ) ;
        m_HBox_Main->add_percentage( 0.60 ) ;

        m_HBox_Main->pack_start( *m_ScrolledWinArtist, true, true, 0 ) ;
        m_HBox_Main->pack_start( *m_ScrolledWinAlbums, true, true, 0 ) ;
        m_HBox_Main->pack_start( *m_ScrolledWinTracks, true, true, 0 ) ;

        std::vector<Gtk::Widget*> widget_v( 6 ) ;
        widget_v[0] = m_Entry ;
        widget_v[1] = m_ScrolledWinArtist ;
        widget_v[2] = m_ScrolledWinAlbums ;
        widget_v[3] = m_ScrolledWinTracks ;
        widget_v[4] = m_main_position ;
        widget_v[5] = m_main_volume ;
        m_main_window->set_focus_chain( widget_v ) ;

        m_HBox_Controls->pack_start( *m_main_position, true, true, 0 ) ;
        m_HBox_Controls->pack_start( *m_main_volume, false, false, 0 ) ;

        m_main_window->set_widget_top( *m_VBox ) ;
        m_main_window->set_widget_drawer( *m_NotebookPlugins ) ; 

        m_VBox_Bottom->pack_start( *m_main_spectrum_titleinfo, false, false, 0 ) ;
        m_VBox_Bottom->pack_start( *m_HBox_Controls, false, false, 0 ) ;

        m_VBox->pack_start( *m_HBox_Entry, false, false, 0 ) ;
        m_VBox->pack_start( *m_HBox_Main, true, true, 0 ) ;
        m_VBox->pack_start( *m_VBox_Bottom, false, false, 0 ) ;

        m_HBox_Bottom->show_all() ;

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
    }

    YoukiController::~YoukiController ()
    {
        delete m_control_status_icon ;
        delete m_main_window ;
        delete private_ ;

        m_control_status_icon = 0 ;
        m_main_window = 0 ;
        private_ = 0 ;

        m_play->request_status( PLAYSTATUS_STOPPED ) ; 

        ShutdownComplete () ;
    }

    Gtk::Window*
    YoukiController::get_widget ()
    {
        return m_main_window ;
    }

////////////////
    MPX::View::Albums::Album_sp
    YoukiController::get_album_from_id( gint64 id )
    {
        SQL::RowV v ;

        try{
          m_library->getSQL( v, (boost::format( "SELECT album, album.mb_album_id, album.id, "
                                                "album_artist.id AS album_artist_id, album_artist, "
                                                "album_artist_sortname, mb_album_artist_id, mb_album_id, mb_release_type, "
                                                "mb_release_date, album_label, album_playscore FROM album "
                                                "JOIN album_artist ON album.album_artist_j = album_artist.id "
                                                "WHERE album.id = '%lld'") % id
                                ).str()) ; 
        } catch( MPX::SQL::SqlGenericError & cxe )
        {
            handle_sql_error( cxe ) ;
        }

        if( v.size() != 1 )
        {
            throw std::logic_error("More than one album with same id = impossible") ;
        }

        SQL::Row & r = v[0] ; 

        Glib::RefPtr<Gdk::Pixbuf> cover_pb ;
        Cairo::RefPtr<Cairo::ImageSurface> cover_is ;

        m_covers->fetch(
              get<std::string>(r["mb_album_id"])
            , cover_pb
            , 64
        ) ;

        if( cover_pb ) 
        {
            cover_is = Util::cairo_image_surface_from_pixbuf( cover_pb ) ;
        }

        SQL::RowV v2 ;
        m_library->getSQL( v2, (boost::format("SELECT count(*) AS cnt FROM track_view WHERE album_j = %lld") % id).str() ) ;

        MPX::View::Albums::Album_sp album ( new MPX::View::Albums::Album ) ;

        album->coverart = cover_is ;
        album->album_id = id ;
        album->artist_id = get<gint64>(r["album_artist_id"]) ;
        album->album = get<std::string>(r["album"]) ;
        album->album_artist = Util::row_get_album_artist_name( r ) ; 
        album->mbid = get<std::string>(r["mb_album_id"]) ;
        album->mbid_artist = get<std::string>(r["mb_album_artist_id"]) ;
        album->type = r.count("mb_release_type") ? get<std::string>(r["mb_release_type"]) : "" ;
        album->year = r.count("mb_release_date") ? get<std::string>(r["mb_release_date"]) : "" ;
        album->label = r.count("album_label") ? get<std::string>(r["album_label"]) : ""  ;
        album->track_count = get<gint64>(v2[0]["cnt"]) ;
        album->album_playscore = get<gdouble>(r["album_playscore"]) ;

        return album ;
    }

    void
    YoukiController::on_style_changed(
    )
    {
        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

        theme->reload() ;

        const ThemeColor& c_bg   = theme->get_color( THEME_COLOR_BACKGROUND ) ; 
        const ThemeColor& c_base = theme->get_color( THEME_COLOR_BASE ) ; 

        Gdk::Color c ;
        c.set_rgb_p( c_base.r, c_base.g, c_base.b ) ; 

        Gdk::Color c2 ;
        c2.set_rgb_p( c_bg.r, c_bg.g, c_bg.b ) ; 

        m_ListViewArtist->modify_bg( Gtk::STATE_NORMAL, c ) ;
        m_ListViewAlbums->modify_bg( Gtk::STATE_NORMAL, c ) ;
        m_ListViewTracks->modify_bg( Gtk::STATE_NORMAL, c ) ;

        c.set_rgb_p( c_bg.r, c_bg.g, c_bg.b ) ; 
    }

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
    YoukiController::on_library_scan_start(
    )
    {
        private_->FilterModelTracks->disable_fragment_cache() ;
        private_->FilterModelTracks->clear_fragment_cache() ;
    }

    void
    YoukiController::on_library_scan_end(
    )
    {
        push_new_tracks() ;
        private_->FilterModelTracks->enable_fragment_cache() ;
    }

    void
    YoukiController::on_library_new_track(
          gint64 id
    )
    {
        m_new_tracks.push_back( id ) ;
       
        if( m_new_tracks.size() == 50 )  
        {
            push_new_tracks() ;
        }
    }

    void
    YoukiController::on_library_new_artist(
          gint64               id
    )
    {
        SQL::RowV v ;

        m_library->getSQL( v, (boost::format( "SELECT * FROM album_artist WHERE id = '%lld'" ) % id ).str() ) ; 
        g_return_if_fail( (v.size() == 1) ) ;

        private_->FilterModelArtist->insert_artist(
              Util::row_get_album_artist_name( v[0] )
            , id 
        ) ; 

        gint64 max_artist, max_albums ;
        private_->FilterModelTracks->get_sizes( max_artist, max_albums ) ;
        max_artist++ ;
        private_->FilterModelTracks->set_sizes( max_artist, max_albums ) ;
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
        SQL::RowV v ;
        m_library->getSQL( v, (boost::format( "SELECT mb_album_id FROM album WHERE album.id = '%lld'") % id ).str()) ; 

        g_return_if_fail( (v.size() == 1) ) ;

        Glib::RefPtr<Gdk::Pixbuf> cover_pb ;

        m_covers->fetch(
              get<std::string>(v[0]["mb_album_id"])
            , cover_pb
            , 64
        ) ;

        if( !cover_pb )
        {
            RequestQualifier rq ; 
            rq.mbid     =   s1 ;
            rq.asin     =   s2 ;
            rq.uri      =   s3 ;
            rq.artist   =   s4 ;
            rq.album    =   s5 ;
            rq.id       =   id ;

            m_covers->cache( rq, true ) ;
        }

        try{
            private_->FilterModelAlbums->insert_album( get_album_from_id( id )) ;
            gint64 max_artist, max_albums ;
            private_->FilterModelTracks->get_sizes( max_artist, max_albums ) ;
            max_albums++ ;
            private_->FilterModelTracks->set_sizes( max_artist, max_albums ) ;
        } catch( std::logic_error ) 
        {
            g_message("Oops") ;
        }
    }

    void
    YoukiController::on_library_entity_deleted(
          gint64                id
        , int                   type
    )
    {
        switch( type )
        {
            case 0: // track
            {
                private_->FilterModelTracks->erase_track( id ) ; 
                private_->FilterModelTracks->regen_mapping() ; 
            }
            break ;

            case 1: // album
            {
                private_->FilterModelAlbums->erase_album( id ) ; 
                private_->FilterModelAlbums->regen_mapping() ;

                gint64 max_artist, max_albums ;
                private_->FilterModelTracks->get_sizes( max_artist, max_albums ) ;
                max_albums-- ;
                private_->FilterModelTracks->set_sizes( max_artist, max_albums ) ;
            }
            break ;

            case 2: // artist
            {
            }
            break ;

            case 3: // album artist
            {
                private_->FilterModelArtist->erase_artist( id ) ; 
                private_->FilterModelArtist->regen_mapping() ; 

                gint64 max_artist, max_albums ;
                private_->FilterModelTracks->get_sizes( max_artist, max_albums ) ;
                max_artist-- ;
                private_->FilterModelTracks->set_sizes( max_artist, max_albums ) ;
            }
            break;
        }
    }

    void
    YoukiController::on_local_library_album_updated( gint64 id )
    {
        on_library_entity_updated( id, 1 ) ;
    }

    void
    YoukiController::on_covers_got_cover( gint64 id )
    {
        Glib::RefPtr<Gdk::Pixbuf> cover_pb ;
        Cairo::RefPtr<Cairo::ImageSurface> cover_is ;

        SQL::RowV v ;
        m_library->getSQL( v, (boost::format( "SELECT mb_album_id FROM album WHERE album.id = '%lld'") % id ).str()) ; 

        m_covers->fetch(
              get<std::string>(v[0]["mb_album_id"])
            , cover_pb
            , 64
        ) ;

        if( cover_pb ) 
        {
            cover_is = Util::cairo_image_surface_from_pixbuf( cover_pb ) ;
        }

        private_->FilterModelAlbums->update_album_cover( id, cover_is ) ;
    }

    void
    YoukiController::on_library_entity_updated(
          gint64                id
        , int                   type
    )
    {
        switch( type )
        {
            case 0: // track
            {
            }
            break ;

            case 1: // album
            {
                try{
                    private_->FilterModelAlbums->update_album( get_album_from_id( id )) ; 
                } catch( std::logic_error ) {
                }
                
            }
            break ;

            case 2: // artist
            {
            }
            break ;

            case 3: // album artist
            {
            }
            break;
        }
    }


    void
    YoukiController::push_new_tracks(
    )
    {
        for( std::vector<gint64>::const_iterator i = m_new_tracks.begin(); i != m_new_tracks.end() ; ++i )
        {
            SQL::RowV v ;

            m_library->getSQL( v, (boost::format( "SELECT * FROM track_view WHERE track_view.id = '%lld'") % *i ).str()) ; 

            if( v.size() != 1)
            {
                g_critical(" Got multiple tracks with the same ID / this can not happen.") ;
                continue ;
            }

            private_->FilterModelTracks->insert_track( v[0], m_library->sqlToTrack( v[0], true, false ) ) ;
        }

        m_new_tracks.clear() ;

        private_->FilterModelTracks->regen_mapping() ;

        while (gtk_events_pending())
            gtk_main_iteration() ;
    }

////////////////

    void
    YoukiController::play_track(
          const MPX::Track_sp& t
    )
    {
        try{
                m_track_previous    = m_track_current ;
                m_track_current     = t ; 

                m_play->switch_stream( m_library->trackGetLocation( t ) ) ;

        } catch( Library::FileQualificationError & cxe )
        {
            g_message("%s: Error: What: %s", G_STRLOC, cxe.what());
        }
    }
    
    void
    YoukiController::on_play_seek(
          gint64 G_GNUC_UNUSED
    )
    {
        m_seek_position.reset() ; 
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

        if( m_seek_position && guint64(position) < m_seek_position.get() ) 
        {
            return ;
        }

        m_main_position->set_position( duration, position ) ;
        m_control_status_icon->set_position( duration, position ) ;
    }

    void
    YoukiController::on_play_eos ()
    {
        m_main_position->stop() ;

        emit_track_out () ;

        Track_sp t = m_track_current ;
        g_return_if_fail( bool(t) ) ;

        const MPX::Track& track = *(t.get()) ;

        boost::optional<gint64> current_id  = boost::get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) ;
        boost::optional<gint64> terminal_id = m_ListViewTracks->get_terminal_id() ;

        if( current_id != terminal_id ) 
        {
            if( !m_play_queue.empty() )
            {
                gint64 next_id = m_play_queue.front() ;
                m_play_queue.pop() ;

                SQL::RowV v ;
                m_library->getSQL(v, (boost::format("SELECT * FROM track_view WHERE id = '%lld'") % next_id ).str()) ; 
                Track_sp p = m_library->sqlToTrack( v[0], true, false ) ;
                play_track( p ) ;
                return ;
            }
            else
            {
                boost::optional<gint64> pos = m_ListViewTracks->get_local_active_track () ;

                if( pos )
                {
                    gint64 pos_next = pos.get() + 1 ;

                    if( pos_next < private_->FilterModelTracks->size() )
                    {
                        play_track( boost::get<4>(private_->FilterModelTracks->row( pos_next )) ) ;
                        return ;
                    }
                }
                else
                if( private_->FilterModelTracks->size() )
                {
                    play_track( boost::get<4>(private_->FilterModelTracks->row( 0 )) ) ;
                    return ;
                }
            }
        }

        if( m_track_previous )
        {
                m_library->trackPlayed(
                      m_track_previous
                    , time(NULL)
                ) ;
        }

        m_play->request_status( PLAYSTATUS_STOPPED ) ; 
        m_ListViewTracks->clear_terminal_id() ;
    }

    void
    YoukiController::on_play_playstatus_changed(
    )
    {
        PlayStatus status = PlayStatus( m_play->property_status().get_value() ) ;

        if( m_control_status_icon )
            m_control_status_icon->set_playstatus( status ) ;

        switch( status )
        {
            case PLAYSTATUS_PLAYING:
                break ;

            case PLAYSTATUS_STOPPED:

                m_track_current.reset() ;
                m_track_previous.reset() ;
                m_seek_position.reset() ; 
                m_playqueue.clear() ;
                m_ListViewTracks->clear_active_track() ;
                m_main_spectrum_titleinfo->clear() ;
                m_main_position->set_position( 0, 0 ) ;

                if( m_control_status_icon )
                    m_control_status_icon->clear() ;

                if( m_main_window )
                    m_main_window->queue_draw () ;    

                break ;

            case PLAYSTATUS_WAITING:

                m_seek_position.reset() ; 
                m_main_spectrum_titleinfo->clear() ;

                if( m_main_window )
                    m_main_window->queue_draw () ;    

                break ;

            case PLAYSTATUS_PAUSED:
                break ;

            default: break ;
        }
    }

    void
    YoukiController::on_play_stream_switched()
    {
        m_main_position->start() ;

        Track_sp t = m_track_current ;
        g_return_if_fail( bool(t) ) ;

        const MPX::Track& track = *(t.get()) ;

        gint64 id_track = boost::get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) ;
        m_ListViewTracks->set_currently_playing_track( id_track ) ;

        if( m_follow_track )
        {
            m_ListViewTracks->scroll_to_id( id_track ) ;
        }

        std::vector<std::string> info ;
        info.push_back( boost::get<std::string>(track[ATTRIBUTE_ARTIST].get()) ) ;
        info.push_back( boost::get<std::string>(track[ATTRIBUTE_ALBUM].get()) ) ;
        info.push_back( boost::get<std::string>(track[ATTRIBUTE_TITLE].get()) ) ;

        if( track.has( ATTRIBUTE_MB_ALBUM_ID ) )
        {
                Glib::RefPtr<Gdk::Pixbuf> cover ;

                if( !m_covers->fetch(
                      boost::get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get())
                    , cover
                ))
                {
                    m_covers->fetch( cover ) ; // we fetch the default cover
                }

                m_control_status_icon->set_metadata( cover, track ) ;
                m_main_spectrum_titleinfo->set_info( info, cover ) ;
        }
        else
        {
            m_control_status_icon->set_metadata( Glib::RefPtr<Gdk::Pixbuf>(0), track ) ;
            m_main_spectrum_titleinfo->set_info( info, Glib::RefPtr<Gdk::Pixbuf>(0) ) ;
        }

        m_playqueue.push_back( id_track ) ; 
        emit_track_new () ;
    }

    void
    YoukiController::on_play_metadata(
            GstMetadataField    field
    )
    {
        // const GstMetadata& m = m_play->get_metadata() ;

        if( field == FIELD_AUDIO_BITRATE )
        {
        }
        else
        if( field == FIELD_AUDIO_CODEC )
        {
        }
    }

    void
    YoukiController::on_list_view_tr_track_activated(
          MPX::Track_sp t 
        , bool          play
    ) 
    {
        const MPX::Track& track = *(t.get()) ;

        emit_track_cancelled() ;

        if( play )
        {
            play_track( t ) ;
        }
        else
        {
            m_play_queue.push( boost::get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) ) ;
            m_ListViewTracks->clear_selection() ;
        }
    }

    void
    YoukiController::on_list_view_tr_find_propagate(
          const std::string&    text
    )
    {
        m_Entry->set_text( (boost::format("title %% \"%s\"") % text).str() ) ;
    }

    void
    YoukiController::on_list_view_aa_selection_changed(
    ) 
    {
        private_->FilterModelTracks->clear_synthetic_constraints_quiet() ;
        m_ListViewAlbums->clear_selection() ;

        //// SET UP CONSTRAINTS

        boost::optional<gint64> id_artist = m_ListViewArtist->get_selected() ;

        if( id_artist ) 
        {
            AQE::Constraint_t c ;
            c.TargetAttr = ATTRIBUTE_MPX_ALBUM_ARTIST_ID ;
            c.TargetValue = id_artist.get() ;
            c.MatchType = AQE::MT_EQUAL ;
            private_->FilterModelTracks->add_synthetic_constraint_quiet( c ) ;
        }

        private_->FilterModelTracks->regen_mapping_iterative() ;

        private_->FilterModelAlbums->set_constraints_albums( private_->FilterModelTracks->m_constraints_albums ) ;
        private_->FilterModelAlbums->set_constraints_artist( private_->FilterModelTracks->m_constraints_artist ) ;
        private_->FilterModelAlbums->regen_mapping() ;
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

        private_->FilterModelTracks->regen_mapping_iterative() ;
    }

    void
    YoukiController::on_list_view_ab_select_album(
        const std::string&  mbid
    )
    {
        on_entry_clear_clicked() ;
        m_Entry->set_text( (boost::format("album-mbid = \"%s\"") % mbid).str() ) ;
    }

    void
    YoukiController::on_list_view_ab_select_artist(
        const std::string&  mbid
    )
    {
        on_entry_clear_clicked() ;
        m_Entry->set_text( (boost::format("album-artist-mbid = \"%s\"") % mbid).str() ) ;
    }

    void
    YoukiController::on_list_view_tr_vadj_changed(
    ) 
    {
        m_follow_track = false ;
    }

    void
    YoukiController::on_entry_activated(
    )
    {
        on_entry_changed() ;
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
                        return true ;
                    }
                }

                break ;
            }

            case GDK_BackSpace:
            {

                if( m_Entry->get_text().empty() )
                {
                    on_entry_clear_clicked() ;
                    return true ;
                }

                break ;
            }

            case GDK_c:
            case GDK_C:
            {
                if( event->state & GDK_CONTROL_MASK )  
                { 
                    m_Entry->set_text( "" ) ;
                   return true ;
                }

                break ;
            }

            case GDK_w:
            case GDK_W:
            {
                if( event->state & GDK_CONTROL_MASK )  
                {
                    Glib::ustring text = m_Entry->get_text() ;

                    if( text.rfind(' ') != Glib::ustring::npos ) 
                    {
                        text = text.substr( 0, text.rfind(' ')) ;
                    }

                    else

                    if( !text.empty() )    
                    {
                        text.clear() ;
                    }

                    m_Entry->set_text( text ) ;
                    m_Entry->set_position( -1 ) ;

                    return true ;
                }

                break ;
            }

            default: break ;
        }

        return false ;
    }

    void
    YoukiController::on_entry_clear_clicked(
    )
    {
        private_->FilterModelAlbums->clear_constraints_artist() ;
        private_->FilterModelAlbums->clear_constraints_album() ;
        private_->FilterModelArtist->clear_constraints_artist() ;
        private_->FilterModelTracks->clear_synthetic_constraints_quiet() ;

        m_Entry->set_text( "" ) ;

        private_->FilterModelAlbums->regen_mapping() ;
        private_->FilterModelArtist->regen_mapping() ;
        private_->FilterModelTracks->regen_mapping() ;

        m_conn1.block() ;
        m_conn2.block() ;
        m_conn3.block() ;
        m_conn4.block() ;

        m_ListViewArtist->scroll_to_row( 0 ) ;
        m_ListViewArtist->select_row( 0 ) ;

        m_ListViewAlbums->scroll_to_row( 0 ) ;
        m_ListViewAlbums->select_row( 0 ) ;

        m_conn1.unblock() ;
        m_conn2.unblock() ;
        m_conn3.unblock() ;
        m_conn4.unblock() ;

        if( m_track_current )
        {
            const MPX::Track& track = *(m_track_current.get()) ;

            gint64 id_track = boost::get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) ;
            m_ListViewTracks->scroll_to_id( id_track ) ;
        }
        else
        {
            m_ListViewTracks->scroll_to_row( 0 ) ;
        }
    }

    void
    YoukiController::on_entry_changed(
    )
    {
        m_ListViewAlbums->clear_selection() ;
        m_ListViewArtist->clear_selection() ;

        private_->FilterModelTracks->clear_synthetic_constraints_quiet() ;
        private_->FilterModelTracks->set_filter( m_Entry->get_text() ) ;

        private_->FilterModelArtist->set_constraints_artist( private_->FilterModelTracks->m_constraints_artist ) ;
//        private_->FilterModelArtist->regen_mapping() ;

        private_->FilterModelAlbums->set_constraints_albums( private_->FilterModelTracks->m_constraints_albums ) ;
        private_->FilterModelAlbums->set_constraints_artist( private_->FilterModelTracks->m_constraints_artist ) ;
//        private_->FilterModelAlbums->regen_mapping() ;
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

    void
    YoukiController::on_ab_start_playback ()
    {
        if( private_->FilterModelTracks->size() )
        {
           play_track( boost::get<4>(private_->FilterModelTracks->row(0))) ;
        }
    }

    bool
    YoukiController::on_main_window_key_press_event_pre(
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

            default: break ;
        }

        return false ;
    }

    bool
    YoukiController::on_main_window_key_press_event_after(
          GdkEventKey* event
    ) 
    {
        switch( event->keyval )
        {
            case GDK_Escape:
                m_main_window->hide() ;
                return true ;

            default: break ;
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

        volume = std::max( 0, (((volume+4)/5)*5) - 5 ) ;

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

        volume = std::min( 100, ((volume/5)*5) + 5 ) ;

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
        if( !m_track_current )
        {
            return std::map<std::string, DBus::Variant>() ;
        }

        std::map<std::string, DBus::Variant> m ;

        const MPX::Track& track = *(m_track_current.get()) ;

        for( int n = ATTRIBUTE_LOCATION; n < N_ATTRIBUTES_STRING; ++n )
        {
            if( track[n].is_initialized() )
            {
                    DBus::Variant val ;
                    DBus::MessageIter iter = val.writer() ;
                    std::string v = boost::get<std::string>( track[n].get() ) ; 
                    iter << v ;
                    m[mpris_attribute_id_str[n-ATTRIBUTE_LOCATION]] = val ;
            }
        }

        for( int n = ATTRIBUTE_TRACK; n < N_ATTRIBUTES_INT; ++n )
        {
            if( track[n].is_initialized() )
            {
                    DBus::Variant val ;
                    DBus::MessageIter iter = val.writer() ;
                    gint64 v = boost::get<gint64>( track[n].get() ) ;
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
        m_play_queue.push( id ) ;
    }

    std::vector<gint64>
    YoukiController::get_current_play_queue(
    )
    {
        std::vector<gint64> v ; 
        std::queue<gint64> queue_copy = m_play_queue ;

        while( !queue_copy.empty() )
        {
            v.push_back( queue_copy.front() ) ;
            queue_copy.pop() ;
        }

        return v ;
    }

    int
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
            return *(m_track_current.get()) ;
        else
            throw std::runtime_error("No current track!") ; // FIXME: Well
    }

    MPX::Track&
    YoukiController::get_metadata_previous(
    )
    {
        if( m_track_previous )
            return *(m_track_previous.get()) ;
        else
            throw std::runtime_error("No previous track!") ;
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
                    play_track( m_library->sqlToTrack( v[0], true, false ) ) ;
                }
        }
    }

    void
    YoukiController::API_stop(
    )
    {
        m_play->request_status( PLAYSTATUS_STOPPED ) ; 
    }

    void
    YoukiController::API_play_track(
        gint64  id
    )
    {
        SQL::RowV v ;
        m_library->getSQL(v, (boost::format("SELECT * FROM track_view WHERE id = '%lld'") % id ).str()) ; 
        Track_sp p = m_library->sqlToTrack( v[0], true, false ) ;
        play_track( p ) ;
    }

}
