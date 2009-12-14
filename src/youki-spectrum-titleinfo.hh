#ifndef _YOUKI_SPECTRUM_TITLEINFO__HH
#define _YOUKI_SPECTRUM_TITLEINFO__HH

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <vector>
#include <string>
#include <sigc++/sigc++.h>

#include "mpx/algorithm/modulo.hh"

#include "mpx/i-youki-play.hh"
#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-types.hh"

namespace MPX
{
    class YoukiSpectrumTitleinfo
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

            YoukiSpectrumTitleinfo () ;
            virtual ~YoukiSpectrumTitleinfo () {}

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
            draw_titleinfo(
                  Cairo::RefPtr<Cairo::Context>&
            );

            void
            draw_background(
                  Cairo::RefPtr<Cairo::Context>&
            );

            void
            draw_cover(
                  Cairo::RefPtr<Cairo::Context>&
            );

            void
            on_play_status_changed(
            ) ;

            bool
            redraw_handler(
            ) ;

      public:

          void
          set_info(
              const std::vector<std::string>&
            , Glib::RefPtr<Gdk::Pixbuf>
          ) ;

          void
          clear () ;
    
      private:

          double total_animation_time ;
          double start_time ;
          double end_time ; 

          std::vector<std::string>    m_info ;
          sigc::connection            m_update_connection;
          Glib::Timer                 m_timer;
          Modulo<double>              m_tmod ;
          double                      m_current_time ;

          bool
          update_frame ();

          double
          cos_smooth (double x) ;

          std::string 
          get_text_at_time () ;

          double
          get_text_alpha_at_time () ;

      private:

          Glib::RefPtr<Gdk::Pixbuf> m_cover ;

   };
}


#endif // _YOUKI_SPECTRUM_TITLEINFO__HH
