#ifndef MPX_INFOAREA_HH
#define MPX_INFOAREA_HH

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <vector>
#include <string>
#include <sigc++/sigc++.h>

namespace MPX
{
  typedef std::vector<std::string>  TextSet ; 
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

          std::vector<float> m_spectrum_data, m_spectrum_peak;

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
            const std::vector<float>&
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
