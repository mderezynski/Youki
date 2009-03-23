#ifndef KOBO_CONTROLLER_HH
#define KOBO_CONTROLLER_HH

#include <boost/optional.hpp>

#include "kobo-main.hh"
#include "kobo-position.hh"
#include "kobo-cover.hh"
#include "alltracks.hh"

#include "mpx/mpx-types.hh"

namespace MPX
{
    class Play ;
    class KoboController
    {
        protected:

            MainWindow          * m_main_window ;
            AllTracksView       * m_main_track_view ;
            Gtk::VBox           * m_vbox_bottom ;
            KoboPosition        * m_main_position ;
            KoboCover           * m_main_cover ;
            Play                * m_play ;

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

        protected:

            void
            play_track (
                   MPX::Track&
            ) ;
    } ;
}

#endif // KOBO_CONTROLLER_HH
