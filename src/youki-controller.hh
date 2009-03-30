#ifndef KOBO_CONTROLLER_HH
#define KOBO_CONTROLLER_HH

#include <boost/optional.hpp>

#include "kobo-main.hh"
#include "kobo-position.hh"
#include "kobo-cover.hh"
#include "kobo-titleinfo.hh"
#include "kobo-volume.hh"
#include "mpx/mpx-types.hh"
#include "mpx/com/view-albumartist.hh"
#include "mpx/com/view-albums.hh"
#include "mpx/com/view-tracklist.hh"
#include "infoarea.hh"

#include "mpx-mlibman-dbus-proxy-actual.hh"
#include "mpx-app-dbus-adaptor.hh"
#include "youki-controller-status-icon.hh"

namespace MPX
{
    class Play ;
    class YoukiController
    : public Glib::Object
    , public ::info::backtrace::Youki::App_adaptor
    , public DBus::ObjectAdaptor
    , public DBus::IntrospectableAdaptor
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

            ListViewAA                      * m_ListViewAA ;
            ListViewAlbums                  * m_ListViewAlbums ;
            ListView                        * m_ListViewTracks ;

            Gtk::ScrolledWindow             * m_ScrolledWinAA ;
            Gtk::ScrolledWindow             * m_ScrolledWinAlbums ;
            Gtk::ScrolledWindow             * m_ScrolledWinTracks ;

            Gtk::HPaned                     * m_Paned1 ;
            Gtk::HPaned                     * m_Paned2 ;
            
            Gtk::Entry                      * m_Entry ;
            Gtk::Alignment                  * m_Alignment_Entry ;
            Gtk::HBox                       * m_HBox_Entry ;
            Gtk::HBox                       * m_HBox_Controls ;
            Gtk::Label                      * m_Label_Search ;

            Gtk::VBox                       * m_VBox ;
            Gtk::HBox                       * m_HBox ;

            DataModelFilterAA_SP_t            m_FilterModelAA ;
            DataModelFilterAlbums_SP_t        m_FilterModelAlbums ;
            DataModelFilterTracks_SP_t        m_FilterModelTracks ;

            Glib::RefPtr<Gdk::Pixbuf>           m_icon ; 
            Cairo::RefPtr<Cairo::ImageSurface>  m_disc ;
            Cairo::RefPtr<Cairo::ImageSurface>  m_disc_multiple ;

            Play                            * m_play ;
            gint64                            m_seek_position ;
    
            boost::optional<MPX::Track>       m_current_track ;          

            info::backtrace::Youki::MLibMan_proxy_actual  * m_mlibman_dbus_proxy ;

            sigc::connection                  m_conn1, m_conn2, m_conn3 ;

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

            enum ModelOrigin
            {
                  ORIGIN_MODEL_ALBUM_ARTISTS
                , ORIGIN_MODEL_ALBUMS
            } ;

            void
            on_list_view_selection_changed(
                ModelOrigin
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
            reload_library() ;

            void
            on_library_scan_end() ;

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

            PlayStatus
            get_status(
            )
            {
            }

            MPX::Track&
            get_metadata(
            )
            {
            }

            void
            pause(
            )
            {
            }

            void        
            add_info_widget(
                  Gtk::Widget*
                , const std::string& name
            )
            {
            }

            void
            remove_info_widget(
                  Gtk::Widget*
            )
            {
            }
    } ;
}

#endif // KOBO_CONTROLLER_HH
