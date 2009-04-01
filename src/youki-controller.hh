#ifndef KOBO_CONTROLLER_HH
#define KOBO_CONTROLLER_HH

#include <boost/optional.hpp>

#include "kobo-main.hh"
#include "kobo-position.hh"
#include "kobo-cover.hh"
#include "kobo-titleinfo.hh"
#include "kobo-volume.hh"
#include "mpx/mpx-services.hh"
#include "mpx/mpx-types.hh"
#include "infoarea.hh"

#include "mpx-mlibman-dbus-proxy-actual.hh"
#include "mpx-app-dbus-adaptor.hh"
#include "youki-controller-status-icon.hh"

namespace MPX
{
    class Covers ;
    class Play ;

    class ListViewTracks ;
    class ListViewAlbums ;
    class ListViewArtist ;

    struct FakeModel
    : public Gtk::TreeModelColumnRecord 
    {
            Gtk::TreeModelColumn<std::string> Fake ;

            FakeModel ()
            {
                add( Fake ) ;
            }
    } ;

    class YoukiController
    : public Glib::Object
    , public ::info::backtrace::Youki::App_adaptor
    , public DBus::ObjectAdaptor
    , public DBus::IntrospectableAdaptor
    , public Service::Base
    {
        protected:

            YoukiControllerStatusIcon       * m_control_status_icon ;

            MainWindow                      * m_main_window ;

            int                               m_main_window_x 
                                            , m_main_window_y ;
            
            InfoArea                        * m_main_infoarea ;
            KoboPosition                    * m_main_position ;
            KoboCover                       * m_main_cover ;
            KoboTitleInfo                   * m_main_titleinfo ;
            KoboVolume                      * m_main_volume ;

            ListViewArtist                  * m_ListViewArtist ;
            ListViewAlbums                  * m_ListViewAlbums ;
            ListViewTracks                  * m_ListViewTracks ;

            Gtk::ScrolledWindow             * m_ScrolledWinArtist ;
            Gtk::ScrolledWindow             * m_ScrolledWinAlbums ;
            Gtk::ScrolledWindow             * m_ScrolledWinTracks ;

            Gtk::HPaned                     * m_Paned1 ;
            Gtk::HPaned                     * m_Paned2 ;
            
            Gtk::Entry                        * m_Entry ;
            Glib::ustring                       m_Entry_Text ;
            Glib::RefPtr<Gtk::ListStore>        m_Completion_Store ;
            Glib::RefPtr<Gtk::EntryCompletion>  m_Completion ;
            FakeModel                           CC ;
            Glib::ustring                       m_prediction ;

            Gtk::Alignment                  * m_Alignment_Entry ;
            Gtk::HBox                       * m_HBox_Entry ;
            Gtk::HBox                       * m_HBox_Controls ;
            Gtk::Label                      * m_Label_Search ;
            bool                              m_completion_enabled ;
            Glib::Timer                       m_completion_timer ;

            Gtk::VBox                       * m_VBox ;
            Gtk::HBox                       * m_HBox ;

            Gtk::Notebook                   * m_NotebookPlugins ;

            struct Private ;
            Private                         * private_ ;

            Glib::RefPtr<Gdk::Pixbuf>           m_icon ; 
            Cairo::RefPtr<Cairo::ImageSurface>  m_disc ;
            Cairo::RefPtr<Cairo::ImageSurface>  m_disc_multiple ;
    
            Covers                          * m_covers ;
            Play                            * m_play ;
            gint64                            m_seek_position ;
    
            boost::optional<MPX::Track>       m_current_track ;          
            boost::optional<gint64>           m_next_track_queue_id ;

            info::backtrace::Youki::MLibMan_proxy_actual  * m_mlibman_dbus_proxy ;

            sigc::connection                  m_conn1, m_conn2, m_conn3 ;

            guint                             m_C_SIG_ID_metadata_updated ;
            guint                             m_C_SIG_ID_track_cancelled ;

        public: 

            YoukiController( DBus::Connection ) ;
            virtual ~YoukiController () ;

            Gtk::Window*
            get_widget () ;

        protected:

            //// PLAY ENGINE

            void
            on_play_eos(
            ) ;

            void
            on_play_position(
                  gint64
            ) ;

            void
            on_play_playstatus(
                  PlayStatus
            ) ;

            void
            on_play_stream_switched(        
            ) ;

            //// TRACK LIST 

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

            //// MISCELLANEOUS WIDGETS 

            void
            on_info_area_clicked(
            ) ;

            bool
            on_main_window_key_press(
                GdkEventKey*
            ) ;

            void
            on_entry_changed(
            ) ;

            bool
            on_entry_key_press_event(
                  GdkEventKey*
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

        public: // PLUGIN API

            void
            queue_next_track(
                  gint64 id
            )
            {
                m_next_track_queue_id = id ;
            }

            PlayStatus
            get_status(
            )
            {
            }

            MPX::Track&
            get_metadata(
            )
            {
                if( m_current_track )
                    return m_current_track.get() ;
                else
                    throw std::runtime_error("No current track!") ;
            }

            void
            pause(
            )
            {
            }

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
    } ;
}

#endif // KOBO_CONTROLLER_HH
