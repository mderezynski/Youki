
//  MPX
//  Copyright (C) 2007-2008 <http://bmpx.backtrace.info> 
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//  --
//
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#include <cstring>
#include <gtkmm.h>
#include <gst/video/video.h>

#include "video-widget.hh"
#include "play.hh"

namespace MPX
{
  using namespace Gtk;
  using namespace Glib;

  ProxyOf<PropBool>::ReadWrite
  VideoWindow::property_playing()
  {
      return ProxyOf<PropBool>::ReadWrite (this, "playing");
  }

  ProxyOf<PropGeom>::ReadWrite
  VideoWindow::property_geometry()
  {
      return ProxyOf<PropGeom>::ReadWrite (this, "geometry");
  }

  ProxyOf<PropAspectRatio>::ReadWrite
  VideoWindow::property_aspect_ratio()
  {
      return ProxyOf<PropAspectRatio>::ReadWrite (this, "aspect-ratio");
  }


  VideoWindow::VideoWindow (Play * obj_play)
  : ObjectBase("MPXVideoWindow")
  , Widget()
  , property_playing_(*this, "playing", false)
  , property_geometry_(*this, "geometry", Geometry (1,1))
  , property_aspect_ratio_ (*this, "aspect-ratio", VW_RATIO_AUTO)
  , m_video_width_pixels(0)
  , m_video_height_pixels(0)
  , m_PAR(0)
  , m_Play(obj_play)
  {
    property_playing().signal_changed().connect( sigc::mem_fun( *this, &VideoWindow::queue_resize ) );
    property_aspect_ratio().signal_changed().connect( sigc::mem_fun( *this, &VideoWindow::queue_resize ) );
  }

  VideoWindow::~VideoWindow ()
  {}

  void
  VideoWindow::on_size_allocate (Gtk::Allocation& allocation)
  {
    GTK_WIDGET(gobj())->allocation = *(allocation.gobj());

    if(is_realized())
    {
      get_window()->move_resize (allocation.get_x(), allocation.get_y(),
          allocation.get_width(), allocation.get_height());

      if(property_playing() == true)
      {
        /* resize video_window */
        int w = 0, h = 0;
        get_media_size (w, h);

        if (!w || !h)
        {
          w = allocation.get_width();
          h = allocation.get_height();
        }

        gdouble width = w;
        gdouble height = h;
        gdouble ratio = 0.;

        if (((gdouble (allocation.get_width())) / width) >
             ((gdouble (allocation.get_height())) / height))
        {
          ratio = (gdouble (allocation.get_height())) / height;
        }
        else
        {
          ratio = (gdouble (allocation.get_width())) / width;
        }

        width *= ratio;
        height *= ratio;

        mVideo->move_resize (int ((allocation.get_width() - width) / 2),
                             int ((allocation.get_height() - height) / 2),
                             int (width), int(height));
      }

      queue_draw ();
    }
    //
  }

  void
  VideoWindow::on_size_request (Gtk::Requisition* requisition)
  {
    if (!requisition)
    {
      *requisition = Gtk::Requisition();
      requisition->width = 240;
      requisition->height = 180;
    }
    //
  }

  bool
  VideoWindow::on_toplevel_configure_event (GdkEventConfigure * event)
  {
    if( property_playing() == true )
    {
      queue_draw ();
    }

    return false;
    //
  }

  bool
  VideoWindow::on_configure_event (GdkEventConfigure * event)
  {
    Widget::on_configure_event( event );

    if( property_playing() == true )
    {
      queue_draw ();
    }

    return false;
    //
  }

  bool
  VideoWindow::on_expose_event (GdkEventExpose * event)
  {
    if (event && event->count > 0)
      return true;

    if( property_playing() == true )
    {
      m_Play->set_window_id (GDK_WINDOW_XID (mVideo->gobj()));
    }

    GtkWidget * widget = GTK_WIDGET(gobj());
    gdk_draw_rectangle (widget->window, widget->style->black_gc, TRUE, 0, 0,
      widget->allocation.width, widget->allocation.height);

    if( property_playing() == true )
    {
      m_Play->video_expose(); 
    }

    return true;
    //
  }
   
  void
  VideoWindow::on_map ()
  {
      Widget::on_map();
      mVideo->show();
  } 

  void
  VideoWindow::on_unmap ()
  {
      Widget::on_unmap();
      mVideo->hide();
  }

  void
  VideoWindow::on_unrealize ()
  {
      mVideo.clear();
      Widget::on_unrealize (); 
  }

  void
  VideoWindow::get_media_size (int & width, int & height)
  {
        GValue * disp_par = NULL;
        guint movie_par_n, movie_par_d, disp_par_n, disp_par_d, num, den;
        
        /* Create and init the fraction value */
        disp_par = g_new0 (GValue, 1);
        g_value_init (disp_par, GST_TYPE_FRACTION);

        /* Square pixel is our default */
        gst_value_set_fraction (disp_par, 1, 1);
      
        /* Now try getting display's pixel aspect ratio */
        if (m_Play->has_video() && m_Play->x_overlay())
        {
          GObjectClass *klass;
          GParamSpec *pspec;

          klass = G_OBJECT_GET_CLASS (m_Play->x_overlay());
          pspec = g_object_class_find_property (klass, "pixel-aspect-ratio");
        
          if (pspec != NULL) {
            GValue disp_par_prop = { 0, };

            g_value_init (&disp_par_prop, pspec->value_type);
            g_object_get_property (G_OBJECT (m_Play->x_overlay()),
                "pixel-aspect-ratio", &disp_par_prop);

            if (!g_value_transform (&disp_par_prop, disp_par)) {
              GST_WARNING ("Transform failed, assuming pixel-aspect-ratio = 1/1");
              gst_value_set_fraction (disp_par, 1, 1);
            }
          
            g_value_unset (&disp_par_prop);
          }
        }
        
        disp_par_n = gst_value_get_fraction_numerator (disp_par);
        disp_par_d = gst_value_get_fraction_denominator (disp_par);
        
        /* If movie pixel aspect ratio is enforced, use that */
        if (property_aspect_ratio().get_value() != VW_RATIO_AUTO)
        {
          switch (property_aspect_ratio().get_value())
          {
            case VW_RATIO_SQUARE:
              movie_par_n = 1;
              movie_par_d = 1;
              break;
            case VW_RATIO_FOURBYTHREE:
              movie_par_n = 4 * property_geometry().get_value().second; 
              movie_par_d = 3 * property_geometry().get_value().first; 
              break;
            case VW_RATIO_ANAMORPHIC:
              movie_par_n = 16 * property_geometry().get_value().second; 
              movie_par_d = 9 * property_geometry().get_value().first; 
              break;
            case VW_RATIO_DVB:
              movie_par_n = 20 * property_geometry().get_value().second; 
              movie_par_d = 9 * property_geometry().get_value().first; 
              break;
            /* handle these to avoid compiler warnings */
            case VW_RATIO_AUTO:
            default:
              movie_par_n = 0;
              movie_par_d = 0;
              g_assert_not_reached ();
          }
        }
        else
        {
          /* Use the movie pixel aspect ratio if any */
          if (m_PAR != 0)
          {
            movie_par_n = gst_value_get_fraction_numerator (m_PAR);
            movie_par_d = gst_value_get_fraction_denominator (m_PAR);
          }
          else
          {
            /* Square pixels */
            movie_par_n = 1;
            movie_par_d = 1;
          }
        }

        if (!gst_video_calculate_display_ratio (&num, &den,
            property_geometry().get_value().first, property_geometry().get_value().second, 
            movie_par_n, movie_par_d, disp_par_n, disp_par_d))
        {
          num = 1;   /* FIXME: what values to use here? */
          den = 1;
        }

        /* now find a width x height that respects this display ratio.
         * prefer those that have one of w/h the same as the incoming video
         * using wd / hd = num / den */
      
        /* start with same height, because of interlaced video */
        /* check hd / den is an integer scale factor, and scale wd with the PAR */

        int video_width = property_geometry().get_value().first;
        int video_height = property_geometry().get_value().second;

        /* now find a width x height that respects this display ratio.
         * prefer those that have one of w/h the same as the incoming video
         * using wd / hd = num / den */

        /* start with same height, because of interlaced video */
        /* check hd / den is an integer scale factor, and scale wd with the PAR */

        if (video_height % den == 0)
        {
          m_video_width_pixels =
              (guint) gst_util_uint64_scale (video_height, num, den);
          m_video_height_pixels = video_height;
        }
        else if (video_width % num == 0)
        {
          m_video_width_pixels = video_width;
          m_video_height_pixels =
              (guint) gst_util_uint64_scale (video_width, den, num);
        }
        else
        {
          m_video_width_pixels =
              (guint) gst_util_uint64_scale (video_height, num, den);
          m_video_height_pixels = video_height;
        }

        width = m_video_width_pixels;
        height = m_video_height_pixels;

        /* Free the PAR fraction */
        g_value_unset (disp_par);
        g_free (disp_par);
  }


  void
  VideoWindow::on_realize ()
  {
      Allocation allocation = get_allocation();
      GdkWindowAttr attributes;
      memset (&attributes, 0, sizeof(attributes));

      /* Creating our widget's window */
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.x = allocation.get_x();
      attributes.y = allocation.get_y();
      attributes.width = allocation.get_width();
      attributes.height = allocation.get_height();
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.visual = get_visual()->gobj();
      attributes.colormap = get_colormap()->gobj();
      attributes.event_mask = (int)(Gdk::EXPOSURE_MASK | get_events()); 

      mWindow = Gdk::Window::create (get_parent_window(), &attributes,
            GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP);
      mWindow->set_user_data(gobj());
      set_window(mWindow);

      /* Creating our video window */
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.x = 0;
      attributes.y = 0; 
      attributes.width = allocation.get_width(); 
      attributes.height = allocation.get_height();
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.event_mask = (int)(
          Gdk::EXPOSURE_MASK |
/*
          Gdk::SCROLL_MASK |
          Gdk::POINTER_MOTION_MASK |
          Gdk::ENTER_NOTIFY_MASK |
          Gdk::LEAVE_NOTIFY_MASK |
          Gdk::BUTTON_PRESS_MASK | 
          Gdk::BUTTON_RELEASE_MASK |
*/
          get_events());

      mVideo = Gdk::Window::create (mWindow, &attributes,
          GDK_WA_X | GDK_WA_Y);
      mVideo->set_user_data (gobj());

      mWindow->set_background (get_style()->get_black());
      //mVideo->set_background (get_style()->get_black());
      GTK_WIDGET(gobj())->style = gtk_style_attach (GTK_WIDGET(gobj())->style, mWindow->gobj()); 
      set_flags(REALIZED);
      unset_flags(CAN_FOCUS);
      set_double_buffered( false );
      set_app_paintable( true );
      get_toplevel()->signal_configure_event().connect( sigc::mem_fun( *this, &VideoWindow::on_toplevel_configure_event ) );
      get_parent()->signal_configure_event().connect( sigc::mem_fun( *this, &VideoWindow::on_toplevel_configure_event ) );
  }
}

