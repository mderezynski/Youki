#ifndef _YOUKI_CONTROLLER__HH
#define _YOUKI_CONTROLLER__HH

#include <queue>

#include <boost/optional.hpp>

#include "mpx/mpx-services.hh"
#include "mpx/mpx-types.hh"

#include "kobo-main.hh"
#include "kobo-position.hh"
#include "kobo-cover.hh"
#include "kobo-titleinfo.hh"
#include "kobo-volume.hh"

#include "youki-simpleinfo.hh"
#include "youki-spectrum.hh"

#include "mpx/widgets/youki-tristate-button.hh"
#include "mpx/widgets/youki-toggle-button.hh"

#include "mpx-mlibman-dbus-proxy-actual.hh"
#include "mpx-app-dbus-adaptor.hh"

#include "mpx/i-youki-controller.hh"
#include "mpx/i-youki-play.hh"

namespace MPX
{
    class Covers ;
    class Library ;
    class Play ;

    namespace View
    {
        namespace Tracks
        {
            class Class ;
        }

        namespace Albums
        {
            class Class ;
        }

        namespace Artist
        {
            class Class ;
        }
    }

    struct YoukiControllerStatusIcon ;
    class PercentualDistributionHBox ;

    class YoukiController
    : public IYoukiController
    , public ::info::backtrace::Youki::App_adaptor
    , public DBus::ObjectAdaptor
    , public DBus::IntrospectableAdaptor
    , public Service::Base
    {
        public: 

            typedef sigc::signal<void>          Signal0_void ;

        private:

            Signal0_void        m_SIG_track_new ;
            Signal0_void        m_SIG_track_out ;
            Signal0_void        m_SIG_track_cancelled ;

        public:

            Signal0_void&
            on_track_new ()
            {
                return m_SIG_track_new ;
            }

            Signal0_void&
            on_track_out ()
            {
                return m_SIG_track_out ;
            }

            Signal0_void&
            on_track_cancelled ()
            {
                return m_SIG_track_cancelled ;
            }

        private:

            void
            emit_track_new ()
            {
                m_SIG_track_new.emit() ;

                g_signal_emit(
                      G_OBJECT(gobj())
                    , m_C_SIG_ID_track_new
                    , 0
                ) ;
            }

            void
            emit_track_out ()
            {
                m_SIG_track_out.emit() ;

                g_signal_emit(
                      G_OBJECT(gobj())
                    , m_C_SIG_ID_track_out
                    , 0
                ) ;
            }

            void
            emit_track_cancelled ()
            {
                m_SIG_track_cancelled.emit() ;

                g_signal_emit(
                      G_OBJECT(gobj())
                    , m_C_SIG_ID_track_cancelled
                    , 0
                ) ;
            }

        public:

            YoukiController( DBus::Connection ) ;
            virtual ~YoukiController () ;

            Gtk::Window*
            get_widget () ;

        public: // PLUGIN API

            void
            queue_next_track(
                gint64
            ) ;

            std::vector<gint64>
            get_current_play_queue () ;

            int
            get_status(
            ) ;

            MPX::Track&
            get_metadata(
            ) ;

            MPX::Track&
            get_metadata_previous(
            ) ;

            void
            API_pause_toggle(
            ) ;

            void
            API_next(
            ) ;

            void
            API_prev(
            ) ;

            void        
            add_info_widget(
                  Gtk::Widget*          w
                , const std::string&    name
            )
            {
                m_NotebookPlugins->append_page(
                      *w
                    , name
                ) ;
                w->show_all() ;
            }

            void
            remove_info_widget(
                  Gtk::Widget*          w
            )
            {
                m_NotebookPlugins->remove_page(
                      *w
                ) ;
            }

        protected:

            struct Private ;
            Private                         * private_ ;

            YoukiControllerStatusIcon       * m_control_status_icon ;

            MainWindow                      * m_main_window ;

            int                               m_main_window_x 
                                            , m_main_window_y ;
            
            YoukiSpectrum                   * m_main_spectrum ;
            KoboPosition                    * m_main_position ;
            KoboTitleInfo                   * m_main_titleinfo ;
            KoboVolume                      * m_main_volume ;
            YoukiTristateButton             * m_main_love_button;

            View::Artist::Class             * m_ListViewArtist ;
            View::Albums::Class             * m_ListViewAlbums ;
            View::Tracks::Class             * m_ListViewTracks ;

            Gtk::ScrolledWindow             * m_ScrolledWinArtist ;
            Gtk::ScrolledWindow             * m_ScrolledWinAlbums ;
            Gtk::ScrolledWindow             * m_ScrolledWinTracks ;

            Gtk::HPaned                     * m_Paned1 ;
            Gtk::HPaned                     * m_Paned2 ;

            PercentualDistributionHBox      * m_HBox_Main ;
            Gtk::HBox                       * m_HBox_Bottom ;
            Gtk::VBox                       * m_VBox_Bottom ;
            
            Gtk::Entry                      * m_Entry ;

            KoboCover                       * m_cover ;

            Gtk::Alignment                  * m_Alignment_Entry ;
            Gtk::HBox                       * m_HBox_Entry ;
            Gtk::HBox                       * m_HBox_Info ;
            Gtk::HBox                       * m_HBox_Controls ;
            Gtk::Label                      * m_Label_Search ;

            Gtk::VBox                       * m_VBox ;

            Gtk::Notebook                   * m_NotebookPlugins ;

            Glib::RefPtr<Gdk::Pixbuf>         m_icon ;  
    
            Covers                          * m_covers ;
            Play                            * m_play ;
            Library                         * m_library ;
    
            boost::optional<MPX::Track>       m_track_current ;          
            boost::optional<MPX::Track>       m_track_previous ;          
            std::queue<gint64>                m_play_queue ;
            bool                              m_follow_track ;

            boost::optional<guint64>          m_seek_position ;

            info::backtrace::Youki::MLibMan_proxy_actual * m_mlibman_dbus_proxy ;

            sigc::connection                  m_conn1
                                            , m_conn2
                                            , m_conn3
                                            , m_conn4
                                            , m_conn5 ;

            guint                             m_C_SIG_ID_track_new ;
            guint                             m_C_SIG_ID_track_out ;
            guint                             m_C_SIG_ID_track_cancelled ;

            std::vector<guint64>              m_playqueue ;

            std::vector<gint64>               m_new_tracks ;

        protected:

            //// PLAY ENGINE

            void
            on_play_eos(
            ) ;

            void
            on_play_seek(
                  gint64
            ) ;

            void
            on_play_position(
                  gint64
            ) ;

            void
            on_play_playstatus_changed(
            ) ;

            void
            on_play_stream_switched(        
            ) ;

            void
            on_play_metadata(        
                GstMetadataField
            ) ;

            void
            on_list_view_tr_track_activated(
                  MPX::Track        /*track*/
                , bool              /*play or not*/
            ) ;

            void
            on_list_view_tr_find_propagate(
                  const std::string&
            ) ;

            void
            on_list_view_aa_selection_changed(
            ) ;

            void
            on_list_view_ab_selection_changed(
            ) ;

            void
            on_list_view_tr_vadj_changed(
            ) ;

            void
            on_info_area_clicked(
            ) ;

            bool
            on_title_clicked(
                  GdkEventButton*
            ) ;

            bool
            on_main_window_key_press_event_pre(
                  GdkEventKey*
            ) ;

            bool
            on_main_window_key_press_event_after(
                  GdkEventKey*
            ) ;

            void
            on_entry_changed(
            ) ;

            void
            on_advanced_changed(
            ) ;

            bool
            on_entry_key_press_event(
                GdkEventKey*
            ) ;

            void
            on_entry_clear_clicked(
            ) ;

            void
            on_entry_activated(
            ) ;

            void
            on_position_seek(
                  gint64
            ) ;

            void
            on_volume_set_volume(
                  int
            ) ;

            void
            on_status_icon_clicked(
            ) ;

            void
            on_status_icon_scroll_up(
            ) ;

            void
            on_status_icon_scroll_down(
            ) ;

            bool
            on_alignment_expose(
                  GdkEventExpose* event
                , Gtk::Alignment* widget
            ) ;
    
        protected:

            void
            on_style_changed(
            ) ;

        protected:

            void
            play_track (
                  const MPX::Track&
            ) ;

        protected:

            void
            on_library_scan_start() ;

            void
            on_library_scan_end() ;

            void
            on_library_new_album(
                  gint64
                , const std::string&
                , const std::string&
                , const std::string&
                , const std::string&
                , const std::string&
            ) ;

            void
            on_library_new_artist(
                  gint64
            ) ;

            void
            on_library_new_track(
                  gint64
            ) ;

            void
            push_new_tracks(
            ) ;

            void
            on_library_album_deleted(
                  gint64
            ) ;

            void
            on_library_artist_deleted(
                  gint64
            ) ;

            void
            on_library_track_deleted(
                  gint64
            ) ;

        protected:
    
            void
            initiate_quit() ;

            bool
            quit_timeout() ;

        protected: // DBUS

            virtual void
            Present () ;

            virtual void
            Startup () ;

            virtual void
            Quit () ;

            virtual std::map<std::string, DBus::Variant>
            GetMetadata () ;

            virtual void
            Next () ;

            virtual void
            Prev () ;

            virtual void
            Pause () ;
    } ;
}

#endif // YOUKI_CONTROLLER_HH
