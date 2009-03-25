#include "config.h"
#include "infoarea.hh"
#include <glibmm.h>
#include "mpx/widgets/cairo-extensions.hh"

using namespace Glib;
using boost::get;

namespace
{
  struct Color
  {
		guint8 red;
		guint8 green;
		guint8 blue;
  };

  Color colors[] =
  {
        { 0xff, 0xb1, 0x6f },
        { 0xff, 0xc8, 0x7f },
        { 0xff, 0xcf, 0x7e },
        { 0xf6, 0xe6, 0x99 },
        { 0xf1, 0xfc, 0xd4 },
        { 0xbd, 0xd8, 0xab },
        { 0xcd, 0xe6, 0xd0 },
        { 0xce, 0xe0, 0xea },
        { 0xd5, 0xdd, 0xea },
        { 0xee, 0xc1, 0xc8 },
        { 0xee, 0xaa, 0xb7 },
        { 0xec, 0xce, 0xb6 },
  };

  const int RIGHT_MARGIN = 16; 
  const int TOP_MARGIN = 6;
  const int WIDTH = 6;
  const int SPACING = 2;
}

namespace MPX
{
  void
  InfoArea::draw_background (Cairo::RefPtr<Cairo::Context> & cr)
  {
    Gtk::Allocation allocation = get_allocation ();

    cr->set_operator( Cairo::OPERATOR_SOURCE ) ;
    cr->set_source_rgba(
          .12
        , .12
        , .12
        , 1.
    ) ;
    cr->rectangle(
          0
        , 0
        , allocation.get_width()
        , allocation.get_height()
    ) ;
    cr->fill ();
  }

  void
  InfoArea::draw_spectrum (Cairo::RefPtr<Cairo::Context> & cr)
  {
      const int HEIGHT = 36;
      const double ALPHA = 0.4;

      Gtk::Allocation allocation = get_allocation ();

      cr->set_operator(Cairo::OPERATOR_ATOP);

      for (int n = 0; n < SPECT_BANDS; ++n) 
      {
        int x = 0, y = 0, w = 0, h = 0;

        x = allocation.get_width()/2 - ((WIDTH+SPACING)*SPECT_BANDS)/2 + (WIDTH+SPACING)*n ; 
        w = WIDTH; 

        // Peak
        int peak = (int(m_spectrum_peak[n]/2) / 3) * 3;
        y = TOP_MARGIN + (HEIGHT - (peak+HEIGHT)); 
        h = peak+HEIGHT;

        if( w>0 && h>0)
        {
            cr->set_source_rgba(1., 1., 1., 0.1);
            Util::cairo_rounded_rect(cr, x, y, w, h, 1.);
            cr->fill ();
        }

        // Bar
        int data = (int(m_spectrum_data[n]/2) / 3) * 3;
        y = TOP_MARGIN + (HEIGHT - (data+HEIGHT)); 
        h = data+HEIGHT;

        if( w>0 && h>0)
        {
            cr->set_source_rgba (double(colors[n/6].red)/255., double(colors[n/6].green)/255., double(colors[n/6].blue)/255., ALPHA);
            Util::cairo_rounded_rect(cr, x, y, w, h, 1.);
            cr->fill ();
        }
      }
  }
}
