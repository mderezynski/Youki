#ifndef KOBO_CONTROLLER_HH
#define KOBO_CONTROLLER_HH

#include <boost/optional.hpp>

#include "kobo-main.hh"
#include "kobo-position.hh"
#include "kobo-cover.hh"
#include "mpx/mpx-types.hh"
#include "mpx/com/view-tracklist.hh"

namespace MPX
{
    class Play ;
    class KoboController
    {
        protected:

            MainWindow              * m_main_window ;
            KoboPosition            * m_main_position ;
            KoboCover               * m_main_cover ;

            ListView                * m_ListView;
            Gtk::ScrolledWindow     * m_ScrolledWin ;
            Gtk::Entry              * m_Entry ;
            Gtk::VBox               * m_VBox ;
            Gtk::HBox               * m_HBox ;

            DataModelFilterP          m_FilterModel;

            Play                    * m_play ;
    

            boost::optional<MPX::Track> m_current_track ;          

        public: 

            KoboController () ;
            virtual ~KoboController () ;

            Gtk::Window*
            get_widget () ;

        protected:

            void
            on_eos () ;

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
            on_track_activated(
                  MPX::Track        /*track*/
                , bool              /*play or not*/
            ) ;

            void
            on_entry_changed (DataModelFilterP model, Gtk::Entry* entry)
            {
                model->set_filter(entry->get_text());
            }

        protected:

            void
            play_track (
                   MPX::Track&
            ) ;
    } ;
}

#endif // KOBO_CONTROLLER_HH
