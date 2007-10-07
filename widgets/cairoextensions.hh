#ifndef CAIROEXTENSIONS_HH_
#define CAIROEXTENSIONS_HH_

#include <cairomm/cairomm.h>

namespace Bmp
{
  typedef Cairo::RefPtr<Cairo::Context> RefContext;
  typedef Cairo::RefPtr<Cairo::ImageSurface> RefSurface;

  struct CairoCorners
  {
    enum CORNERS
    {
        NONE        = 0,
        TOPLEFT     = 1,
        TOPRIGHT    = 2,
        BOTTOMLEFT  = 4,
        BOTTOMRIGHT = 8,
        ALL         = 15
    };
  };

  void RoundedRectangle (RefContext cr, double x, double y, double w, double h, double r, CairoCorners::CORNERS corners = CairoCorners::ALL);
}

#endif /*CAIROEXTENSIONS_HH_*/
