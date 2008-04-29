
//  MPX
//  Copyright (C) 2007 <http://beep-media-player.org> 
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

#ifndef MPX_VIDEO_WIDGET_GST_HH
#define MPX_VIDEO_WIDGET_GST_HH

#include <gtkmm.h>
#include <gdk/gdkx.h>
#include "mpx/glibaddons.hh"

namespace MPX
{
  enum AspectRatio
  {
    VW_RATIO_AUTO,
    VW_RATIO_SQUARE,
    VW_RATIO_FOURBYTHREE,
    VW_RATIO_ANAMORPHIC,
    VW_RATIO_DVB
  }; 

  typedef std::pair<int, int> Geometry;

  typedef Glib::Property<Geometry> PropGeom;
  typedef Glib::Property<AspectRatio> PropAspectRatio;

  class Play;
  class VideoWindow
    : public Gtk::Widget
  {

    public:

      VideoWindow (Play*);
      virtual ~VideoWindow ();

      ProxyOf<PropBool>::ReadWrite property_playing();
      ProxyOf<PropGeom>::ReadWrite property_geometry();
      ProxyOf<PropAspectRatio>::ReadWrite property_aspect_ratio();

      void
      set_par (GValue const* par)
      {
        if(m_PAR)
        {
            g_value_unset(m_PAR);
        }
        m_PAR = g_new0(GValue, 1);
        g_value_init(m_PAR, G_VALUE_TYPE(par)); 
        g_value_copy(par, m_PAR);
        queue_resize();
      }

      ::Window
      get_video_xid ()
      {
        return GDK_WINDOW_XID (mVideo->gobj());
      }

    private:

      bool
      on_toplevel_configure_event (GdkEventConfigure *);

      void
      get_media_size (int & width, int & height);


    protected:

      virtual void
      on_size_allocate (Gtk::Allocation &);

      virtual void
      on_size_request (Gtk::Requisition *);

      virtual bool
      on_expose_event (GdkEventExpose *);

      virtual bool
      on_configure_event (GdkEventConfigure *);

      virtual void
      on_map ();

      virtual void
      on_unmap ();

      virtual void
      on_unrealize ();

      virtual void
      on_realize ();

    private:

      Glib::RefPtr<Gdk::Window> mWindow, mVideo;

      PropBool        property_playing_;
      PropGeom        property_geometry_;
      PropAspectRatio property_aspect_ratio_;
      int             m_video_width_pixels,
                      m_video_height_pixels;
      GValue        * m_PAR;
      Play          * m_Play;
  };
}

#endif // !MPX_VIDEO_WIDGET_GST_HH
