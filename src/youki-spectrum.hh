#ifndef _YOUKI_SPECTRUM__HH
#define _YOUKI_SPECTRUM__HH

#include <cluttermm.h>
#include <clutter-gtkmm.h>
#include <vector>
#include <string>
#include <sigc++/sigc++.h>

#include "mpx/i-youki-play.hh"
#include "mpx/mpx-types.hh"

namespace MPX
{
    class YoukiSpectrum
    : public Clutter::Gtk::Embed
    {
        private:

            Glib::RefPtr<Clutter::Stage> m_stage ;
            Glib::RefPtr<Clutter::Group> m_group_peaks ;
            Glib::RefPtr<Clutter::Group> m_group_bars ;

            sigc::connection          m_timeout ;
            sigc::signal<void>        m_signal ;
            std::vector<float>        m_spectrum_data ;
            std::vector<float>        m_spectrum_peak ;

            PlayStatus                m_play_status ;

            boost::shared_ptr<IPlay>  m_play ;

        public:

            sigc::signal<void>&
            signal_clicked ()
            {
              return m_signal ;
            }

            YoukiSpectrum () ;
            virtual ~YoukiSpectrum () {}

            void
            reset(
            ) ;

            void
            update_spectrum(
                  const std::vector<float>&
            );

        protected:

            virtual void
            on_size_allocate(
                  Gtk::Allocation&
            ) ;

            virtual bool
            on_button_press_event(
                  GdkEventButton*
            ) ;

            virtual void
            on_show(
            ) ;

            void
            redraw(
            ) ;

            bool
            redraw_handler(
            ) ;

            void
            on_play_status_changed(
            ) ;
   };
}


#endif // _YOUKI_SPECTRUM__HH
