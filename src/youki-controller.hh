#ifndef KOBO_CONTROLLER_HH
#define KOBO_CONTROLLER_HH

#include <boost/optional.hpp>

#include "kobo-main.hh"
#include "kobo-position.hh"
#include "kobo-cover.hh"
#include "kobo-titleinfo.hh"
#include "kobo-volume.hh"
#include "mpx/mpx-types.hh"
#include "mpx/com/view-tracklist.hh"
#include "mpx/com/view-albumartist.hh"
#include "infoarea.hh"

#if 0
#include "youki-controller-dbus.hh"
#endif

namespace MPX
{
    class Play ;
    class YoukiController
/*
    : public org::freedesktop::MediaPlayer_adaptor
    , public DBus::ObjectAdaptor
*/
    {
        protected:

            MainWindow                      * m_main_window ;
            Glib::RefPtr<Gtk::StatusIcon>     m_main_status_icon ;
            int                               m_main_window_x 
                                            , m_main_window_y ;
            
            InfoArea                        * m_main_infoarea ;
            KoboPosition                    * m_main_position ;
            KoboCover                       * m_main_cover ;
            KoboTitleInfo                   * m_main_titleinfo ;
            KoboVolume                      * m_main_volume ;

            ListView                        * m_ListView ;
            ListViewAA                      * m_ListViewAA ;
            Gtk::ScrolledWindow             * m_ScrolledWin ;
            Gtk::ScrolledWindow             * m_ScrolledWinAA ;
            Gtk::HPaned                     * m_Paned ;
            
            Gtk::Entry                      * m_Entry ;
            Gtk::Alignment                  * m_Alignment_Entry ;
            Gtk::HBox                       * m_HBox_Entry ;
            Gtk::HBox                       * m_HBox_Controls ;
            Gtk::Label                      * m_Label_Search ;

            Gtk::VBox                       * m_VBox ;
            Gtk::HBox                       * m_HBox ;

            DataModelFilterP                  m_FilterModel;
            DataModelFilterAAP                m_FilterModelAA;

            Glib::RefPtr<Gdk::Pixbuf>         m_Icon ; 

            Play                            * m_play ;
            gint64                            m_seek_position ;
    
            boost::optional<MPX::Track>       m_current_track ;          

        public: 

            YoukiController( /*DBus::Connection&*/ ) ;
            virtual ~YoukiController () ;

            Gtk::Window*
            get_widget () ;

        protected:

            void
            on_eos(
            ) ;

            void
            on_position(
                  gint64
            ) ;

            void
            on_playstatus(
                  PlayStatus
            ) ;

            void
            on_stream_switched(        
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
            on_info_area_clicked(
            ) ;

            bool
            on_status_icon_clicked(
                GdkEventButton*
            ) ;

            bool
            on_main_window_key_press(
                GdkEventKey*
            ) ;

            void
            on_entry_changed (DataModelFilterP model, Gtk::Entry* entry)
            {
                model->set_filter(entry->get_text());
            }

            void
            on_seek(
                  gint64
            ) ;

            void
            on_volume(
                  int
            ) ;

        protected:

            void
            play_track (
                   MPX::Track&
            ) ;

        protected:

            void
            reload_library () ;

            void
            on_library_scan_end(
                 gint64
               , gint64
               , gint64
               , gint64
               , gint64
            ) ;

        protected: // DBUS

/*
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
*/
    } ;
}

#endif // KOBO_CONTROLLER_HH
