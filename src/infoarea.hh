#ifndef MPX_INFOAREA_HH
#define MPX_INFOAREA_HH

#include "mpx/widgets/widgetloader.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-string.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-types.hh"

#include "mpx/i-playbacksource.hh"

#include <boost/optional.hpp>
#include <gtkmm.h>
#include <cairomm/cairomm.h>

namespace MPX
{
  typedef std::vector<std::string> TextSet; 
  /*
  struct TextSet
  {
    std::string     Artist;
    std::string     Album;
    std::string     Title;
    std::string     Genre;
  };*/

  class InfoArea
    : public Gtk::DrawingArea 
  {
    private:

          Spectrum m_spectrum_data, m_spectrum_peak;

          sigc::signal<void>  m_signal ;

    public:

          sigc::signal<void>&
          signal_clicked ()
          {
            return m_signal ;
          }

          InfoArea () ;
          virtual ~InfoArea () {}

    protected:

          bool
          on_button_press_event (GdkEventButton * event);

          bool
          on_button_release_event (GdkEventButton * event);

    private:

          void
          play_status_changed ();

	public:

          void
          reset ();

          void
          update_spectrum(
            const Spectrum&
          );

    private:

          void
          draw_background(Cairo::RefPtr<Cairo::Context>&);

          void
          draw_spectrum(Cairo::RefPtr<Cairo::Context>&);

    protected:

          virtual bool
          on_expose_event (GdkEventExpose * event);
 };
}


#endif // MPX_INFOAREA_HH
