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
#include "youki-togglebutton.hh"
#include "youki-spectrum.hh"

#include "mpx-mlibman-dbus-proxy-actual.hh"
#include "mpx-app-dbus-adaptor.hh"

#include "mpx/i-youki-controller.hh"
#include "mpx/i-youki-play.hh"

namespace MPX
{
    class Covers ;
    class Library ;
    class Play ;

    class ListViewTracks ;
    class ListViewAlbums ;
    class ListViewArtist ;
    struct YoukiControllerStatusIcon ;

    class YoukiController
    : public IYoukiController
    , public ::info::backtrace::Youki::App_adaptor
    , public DBus::ObjectAdaptor
    , public DBus::IntrospectableAdaptor
    , public Service::Base
    {
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
            YoukiToggleButton               * m_main_love_button;

            ListViewArtist                  * m_ListViewArtist ;
            ListViewAlbums                  * m_ListViewAlbums ;
            ListViewTracks                  * m_ListViewTracks ;

            Gtk::ScrolledWindow             * m_ScrolledWinArtist ;
            Gtk::ScrolledWindow             * m_ScrolledWinAlbums ;
            Gtk::ScrolledWindow             * m_ScrolledWinTracks ;

            Gtk::HPaned                     * m_Paned1 ;
            Gtk::HPaned                     * m_Paned2 ;
            
            Gtk::Entry                      * m_Entry ;
            Glib::ustring                     m_Entry_Text ;
            Glib::ustring                     m_prediction ;
            Glib::ustring                     m_prediction_last ;

            Gtk::Alignment                  * m_Alignment_Entry ;
            Gtk::HBox                       * m_HBox_Entry ;
            Gtk::HBox                       * m_HBox_Info ;
            Gtk::HBox                       * m_HBox_Controls ;
            Gtk::Label                      * m_Label_Search ;
            Glib::Timer                       m_completion_timer ;
            bool                              m_predicted ;

            Gtk::VBox                       * m_VBox ;

            Gtk::Notebook                   * m_NotebookPlugins ;

            Glib::RefPtr<Gdk::Pixbuf>           m_icon ;  
            Cairo::RefPtr<Cairo::ImageSurface>  m_disc ;
            Cairo::RefPtr<Cairo::ImageSurface>  m_disc_multiple ;
    
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

            sigc::connection                  m_conn_completion ;

            guint                             m_C_SIG_ID_track_new ;
            guint                             m_C_SIG_ID_track_out ;
            guint                             m_C_SIG_ID_track_cancelled ;

            std::vector<guint64>              m_playqueue ;


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
            on_main_window_key_press_event(
                  GdkEventKey*
            ) ;

            void
            on_entry_changed__update_completion(
            ) ;

            void
            on_entry_changed__process_filtering(
                bool = false /*force processing*/
            ) ;

            bool
            on_entry_key_press_event(
                GdkEventKey*
            ) ;

            void
            on_entry_activated(
            ) ;

            bool
            completion_timer_check(
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
                  MPX::Track&
            ) ;

        protected:

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

        protected:
    
            bool
            on_completion_match(
                  const Glib::ustring&
                , const Gtk::TreeIter&
            ) ;

            void
            reload_library() ;

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
