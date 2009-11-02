#ifndef _YOUKI_SPECTRUM__HH
#define _YOUKI_SPECTRUM__HH

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <vector>
#include <string>
#include <sigc++/sigc++.h>

#include "mpx/i-youki-play.hh"
#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-types.hh"

namespace MPX
{
    class YoukiSpectrum
    : public Gtk::DrawingArea 
    {
        private:

            sigc::signal<void>        m_signal ;

            std::vector<float>        m_spectrum_data ;
            std::vector<float>        m_spectrum_peak ;

            PlayStatus                m_play_status ;

            boost::shared_ptr<IPlay>              m_play ;
            boost::shared_ptr<IYoukiThemeEngine>  m_theme ;

            enum Mode
            {
                  SPECTRUM_MODE_LINEAR
                , SPECTRUM_MODE_VOCODER
            } ;

            Mode m_mode ;

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

            bool
            on_button_press_event(
                  GdkEventButton*
            ) ;

            virtual bool
            on_expose_event(
                  GdkEventExpose*
            ) ;

            void
            draw_spectrum(
                  Cairo::RefPtr<Cairo::Context>&
            ) ;

            void
            on_play_status_changed(
            ) ;

            bool
            redraw_handler(
            ) ;
   };
}


#endif // _YOUKI_SPECTRUM__HH
