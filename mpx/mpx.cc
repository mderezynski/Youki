//
//  mpx.cc
//
//  Authors:
//      Milosz Derezynski <milosz@backtrace.info>
//
//  (C) 2008 MPX Project
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
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

// PyGTK NO_IMPORT
#define NO_IMPORT

#include "config.h"
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/python.hpp>
#include <cairomm/cairomm.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <giomm.h>
#include <glib/gstdio.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <gdk/gdkx.h>
#include <gdl/gdl.h>

#include "mpx.hh"
#include "mpx/error.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/mpx-python.hh"
#include "mpx/mpx-sql.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-string.hh"
#include "mpx/util-ui.hh"
#include "mpx/xml/xspf.hh"
 
#include "dialog-filebrowser.hh"
#include "import-share.hh"
#include "import-folder.hh"
#include "request-value.hh"
#include "sidebar.hh"
#include "splash-screen.hh"

#include "mlibmanager.hh"
#include "plugin.hh"
#include "plugin-manager-gui.hh"
#include "play.hh"
#include "preferences.hh"

#include "flow_widget.hh"
#include "flow_engine.hh"

using namespace Glib;
using namespace Gtk;
using namespace std;
using namespace Gnome::Glade;
using namespace MPX::Util;
using boost::get;
using boost::algorithm::is_any_of;
using namespace boost::python;

namespace
{
  char const * MenubarMain =
  "<ui>"
  ""
  "<menubar name='MenubarMain'>"
  "   <menu action='MenuMusic'>"
  "         <menuitem action='action-play-files'/>"
  "         <separator name='sep00'/>"
#ifndef HAVE_HAL
  "         <menuitem action='action-import-folder'/>"
  "         <menuitem action='action-import-share'/>"
#endif // !HAVE_HAL
  "         <menuitem action='action-quit'/>"
  "   </menu>"
  "   <menu action='MenuEdit'>"
#ifdef HAVE_HAL
  "         <menuitem action='action-mlibmanager'/>"
#endif // HAVE_HAL
  "			<menuitem action='action-vacuum-lib'/>"
  "         <separator/>"
  "         <menuitem action='action-plugins'/>"
  "         <separator/>"
  "         <menuitem action='action-preferences'/>"
  "   </menu>"
  "   <menu action='MenuView'>"
  "   </menu>"
  "   <menu action='MenuTrack'>"
  "         <placeholder name='placeholder-track-actions'/>"
  "   </menu>"
  "   <placeholder name='placeholder-source'/>"
  "</menubar>"
  ""
  "</ui>"
  "";

  char const ACTION_PLAY [] = "action-play";
  char const ACTION_STOP [] = "action-stop";
  char const ACTION_NEXT [] = "action-next";
  char const ACTION_PREV [] = "action-prev";
  char const ACTION_PAUSE [] = "action-pause";
  char const ACTION_PLUGINS [] = "action-plugins";
  char const ACTION_PREFERENCES[] ="action-preferences";
  char const ACTION_SHOW_INFO[] = "action-show-info";
  char const ACTION_SHOW_VIDEO[] = "action-show-video";
  char const ACTION_SHOW_SOURCES[] = "action-show-sources";

  std::string
  gprintf (const char *format, ...)
  {
    va_list args;
    va_start (args, format);
    char *v = g_strdup_vprintf (format, args);
    va_end (args);
    std::string r (v);
    g_free (v);
    return r; 
  }
}

namespace
{
  inline bool
  is_module (std::string const& basename)
  {
    return MPX::Util::str_has_suffix_nocase(basename.c_str (), G_MODULE_SUFFIX);
  } 
}

namespace MPX
{
  struct LayoutData
  {
    double amod;
    int    x;
    int    y;
  };

  LayoutData const layout_info[] =
  {
    {1.  , 84,  6},
    {0.65, 84, 20},
    {1.  , 84, 54},
  };

  // WARNING: If you set the gravity or timescale too high, the cover
  // animation will never come to a stop. Visually it might well be but it may be
  // performing miniscule bounces.

  double const cover_anim_area_width       = 72.0;
  double const cover_anim_area_height      = 72.0;
  double const cover_anim_area_x0          = 6.0;
  double const cover_anim_area_y0          = 6.0;
  double const cover_anim_area_x1          = cover_anim_area_x0 + cover_anim_area_width;
  double const cover_anim_area_y1          = cover_anim_area_y0 + cover_anim_area_height;

  double const cover_anim_initial_pos      = 6.0 - 72.0;
  double /*const*/ cover_anim_initial_velocity = 323.3; 
  double /*const*/ cover_anim_gravity          = 359.1; 
  double const cover_anim_wall             = cover_anim_area_x1;
  double /*const*/ cover_anim_wall_elasticity  = 0.120; //0.074;
  double const cover_anim_time_scale       = 1.0;

  int    const cover_anim_fps              = 25;
  int    const cover_anim_interval         = 1000 / cover_anim_fps;
  double const cover_anim_dt               = cover_anim_time_scale / cover_anim_fps;

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

  struct TextSet
  {
    std::string     Artist;
    std::string     Album;
    std::string     Title;
    std::string     Genre;
  };

  class InfoArea
    : public WidgetLoader<EventBox>
  {
    private:

          MPX::Play&                            m_Play;
          Spectrum                              m_spectrum_data;
          boost::optional<Glib::ustring>        m_info_text;
          boost::optional<TextSet>              m_text_cur;
          boost::optional<TextSet>              m_text_new;
          boost::optional<Cairo::RefPtr<Cairo::ImageSurface> >   m_cover_surface_cur;
          boost::optional<Cairo::RefPtr<Cairo::ImageSurface> >   m_cover_surface_new;
          double                                m_cover_pos;
          double                                m_cover_velocity;
          double                                m_cover_accel;
          double                                m_cover_alpha;
          double                                m_layout_alpha;
          sigc::connection                      m_cover_anim_conn_fade;
          sigc::connection                      m_cover_anim_conn_slide;
          sigc::connection                      m_decay_conn;
          sigc::connection                      m_fade_conn;

          bool								    m_pressed;
          Glib::Mutex                           m_surface_lock;
          Glib::Mutex                           m_layout_lock;
          Glib::Mutex                           m_info_lock;

    public:

          typedef sigc::signal<void> SignalCoverClicked;
          typedef sigc::signal<void, Util::FileList const&> SignalUris;

          SignalCoverClicked&
          signal_cover_clicked()
          {
            return m_SignalCoverClicked;
          }

          SignalUris &
          signal_uris_dropped()
          {
            return m_SignalUrisDropped;
          }

    private:

          SignalUris            m_SignalUrisDropped;
          SignalCoverClicked    m_SignalCoverClicked;

    public:

          InfoArea (MPX::Play & play,
                    Glib::RefPtr<Gnome::Glade::Xml> const& xml)
          : WidgetLoader<Gtk::EventBox>(xml, "infoarea")
          , m_Play(play)
          , m_spectrum_data(SPECT_BANDS, 0)
          , m_cover_alpha(0.)
          , m_layout_alpha(0.)
          , m_pressed(false)
          {
            add_events (Gdk::BUTTON_PRESS_MASK);

            m_Play.signal_spectrum().connect(
                sigc::mem_fun( *this, &InfoArea::play_update_spectrum )
            );

            m_Play.property_status().signal_changed().connect(
                sigc::mem_fun( *this, &InfoArea::play_status_changed)
            );

            enable_drag_dest ();

            set_tooltip_text(_("Drag and drop files here to play them."));
          }

          ~InfoArea ()
          {}

    protected:

          bool
          on_button_press_event (GdkEventButton * event)
          {
              int x = int (event->x);
              int y = int (event->y);

              if ((event->window == get_window()->gobj()) && (x >= 6) && (x <= (m_cover_pos+72)) && (y >= 3) && (y <= 75))
              {
                  m_pressed = true;
                  queue_draw ();
              }

              return false;
          }

          bool
          on_button_release_event (GdkEventButton * event)
          {
              int x = int (event->x);
              int y = int (event->y);

              m_pressed = false;
              queue_draw ();

              if ((event->window == get_window()->gobj()) && (x >= 6) && (x <= (m_cover_pos+72)) && (y >= 3) && (y <= 75))
              {
                  m_SignalCoverClicked.emit ();
              }

              return false;
          }

          bool
          on_drag_drop (Glib::RefPtr<Gdk::DragContext> const& context,
                        int                                   x,
                        int                                   y,
                        guint                                 time)
          {
              ustring target (drag_dest_find_target (context));
              if( !target.empty() )
              {
                drag_get_data (context, target, time);
                context->drag_finish  (true, false, time);
                return true;
              }
              else
              {
                context->drag_finish  (false, false, time);
                return false;
              }
          }

          void
          on_drag_data_received (Glib::RefPtr<Gdk::DragContext> const& context,
                                 int                                   x,
                                 int                                   y,
                                 Gtk::SelectionData const&             data,
                                 guint                                 info,
                                 guint                                 time)
          {
              if( data.get_data_type() == "text/uri-list")
              {
                Util::FileList u = data.get_uris();
                m_SignalUrisDropped.emit (u);
              }
              else
              if( data.get_data_type() == "text/plain")
              {
                using boost::algorithm::split;
                using boost::algorithm::is_any_of;
                using boost::algorithm::replace_all;

                std::string text = data.get_data_as_string ();
                replace_all (text, "\r", "");

                StrV v;
                split (v, text, is_any_of ("\n"));

                if( v.empty ()) // we're taking chances here
                {
                  v.push_back (text);
                }

                Util::FileList u;

                for(StrV::const_iterator i = v.begin(); i != v.end(); ++i)
                {
                  try{
                    URI uri (*i);
                    u.push_back (*i);
                  }
                  catch (URI::ParseError & cxe)
                  {
                      // seems like not it
                  }
                }

                if( !u.empty() )
                {
                  m_SignalUrisDropped.emit (u);
                }
              }
          }

    private:

          void
          parse_metadata (MPX::Metadata & metadata,
                          TextSet       & set)
          {
            using namespace MPX;

            static char const *
              text_b_f ("<span size='12500'><b>%s</b></span>");

            static char const *
              text_i_f ("<span size='12500'><i>%s</i></span>");

            static char const *
              text_b_f2 ("<span size='12500'><b>%s</b> (%s)</span>");

            static char const *
              text_big_f ("<span size='15000'><b>%s</b></span>");

            static char const *
              text_album_artist_f ("<span size='12500'><b>%s</b> (%s)</span>");

            static char const *
              text_album_f ("<span size='12500'><b>%s</b></span>");

            static char const *
              text_artist_f ("<span size='12500'>(%s)</span>");

            if( (metadata[ATTRIBUTE_MB_ALBUM_ARTIST_ID] != metadata[ATTRIBUTE_MB_ARTIST_ID]) && metadata[ATTRIBUTE_ALBUM_ARTIST] )
            {
                if( metadata[ATTRIBUTE_ALBUM] )
                {
                  set.Album = gprintf(
                    text_album_artist_f,
                    Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM].get())).c_str(),
                    Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM_ARTIST].get())).c_str()
                  );
                }
                else
                {
                  set.Album = gprintf(
                    text_artist_f,
                    Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM_ARTIST].get())).c_str()
                  );
                }
            }
            else
            {
                if( metadata[ATTRIBUTE_ALBUM] )
                {
                  set.Album = gprintf(
                    text_album_f,
                    Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM].get())).c_str()
                  );
                }
            }

            if ((metadata[ATTRIBUTE_ARTIST_SORTNAME] && metadata[ATTRIBUTE_ARTIST]) &&
                (metadata[ATTRIBUTE_ARTIST_SORTNAME] != metadata[ATTRIBUTE_ARTIST])) /* let's display the artist if it's not identical to the sortname */
            {
              set.Artist = gprintf(
                text_b_f2,
                Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST_SORTNAME].get())).c_str(),
                Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST].get())).c_str()
              );
            }
            else
            if( metadata[ATTRIBUTE_ARTIST] )
            {
              set.Artist = gprintf (text_b_f, Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST].get())).c_str());
            }

            if( metadata[ATTRIBUTE_TITLE] )
            {
              set.Title = gprintf (text_big_f, Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_TITLE].get())).c_str());
            }

            if( metadata[ATTRIBUTE_GENRE] )
            {
              set.Genre = gprintf (text_i_f, Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_GENRE].get())).c_str());
            }
          }

          void
          enable_drag_dest ()
          {
              disable_drag_dest ();
              std::vector<Gtk::TargetEntry> target_entries;
              target_entries.push_back (TargetEntry ("text/plain"));
              drag_dest_set (target_entries, Gtk::DEST_DEFAULT_MOTION);
              drag_dest_add_uri_targets ();
          }

          void
          disable_drag_dest ()
          {
              drag_dest_unset ();
          }

          bool
          decay_spectrum ()
          {
              for (int n = 0; n < SPECT_BANDS; ++n)
              {
                m_spectrum_data[n] = (((m_spectrum_data[n] - 5) < 0) ? 0 : (m_spectrum_data[n] - 5));
              }
              queue_draw ();
              return true;
          }

          void
          play_update_spectrum (Spectrum const& spectrum)
          {
              m_spectrum_data = spectrum;
              queue_draw ();

          }

          void
          play_status_changed ()
          {
              if( m_Play.property_status ().get_value() == PLAYSTATUS_PAUSED )
              {
                m_decay_conn = Glib::signal_timeout ().connect(
                    sigc::mem_fun(*this, &InfoArea::decay_spectrum), 50
                );
              }
              else
              {
                m_decay_conn.disconnect ();
              }
          }

	public:

          void
          reset ()
          {
            std::fill (m_spectrum_data.begin (), m_spectrum_data.end (), 0.);

            m_cover_surface_new.reset();
            m_cover_surface_cur.reset();
            m_text_new.reset();
            m_text_cur.reset();

            m_layout_alpha = 1.;
            m_cover_alpha = 1.;

            m_fade_conn.disconnect();
            m_decay_conn.disconnect();
            m_cover_anim_conn_fade.disconnect ();
            m_cover_anim_conn_slide.disconnect ();

            m_info_text.reset();

            queue_draw ();
          }

          void
          set_info ( const Glib::ustring& text )
          {
            Mutex::Lock L (m_info_lock);

            m_info_text = text;
            queue_draw ();
          }
    
          void
          clear_info ()
          {
            Mutex::Lock L (m_info_lock);

            m_info_text.reset();
            queue_draw ();
          }

          void
          set_metadata( Metadata & metadata )
          {
            Mutex::Lock L (m_layout_lock);

            if( metadata.Image )
            {
		        set_cover (metadata.Image->scale_simple (72, 72, Gdk::INTERP_BILINEAR));
            }
            else
                g_message("%s: No image", G_STRFUNC);

            TextSet set;
		    parse_metadata( metadata, set ); 

            if( m_text_cur )
            {
                m_text_new = set;
                m_fade_conn = signal_timeout().connect(
                    sigc::mem_fun( *this, &InfoArea::fade_out_layout ), 15
                );
            }
            else
            {
                m_text_cur = set;
                m_layout_alpha = 1.;
                queue_draw();
            }
          }

          void
          set_cover (RefPtr<Gdk::Pixbuf> const& pixbuf)
          {
            set_cover (Util::cairo_image_surface_round(Util::cairo_image_surface_from_pixbuf (pixbuf), 6.));
          }

          void
          set_cover (Cairo::RefPtr<Cairo::ImageSurface> const& surface)
          {
            Mutex::Lock L (m_surface_lock);

            m_cover_pos           = cover_anim_initial_pos;
            m_cover_velocity      = cover_anim_initial_velocity;
            m_cover_accel         = cover_anim_gravity;
            m_cover_alpha         = 1.;

            if(!m_cover_surface_cur)
            {
                m_cover_surface_cur = surface; 

                m_cover_anim_conn_slide.disconnect ();
                m_cover_anim_conn_slide = signal_timeout ().connect(
                    sigc::mem_fun( *this, &InfoArea::slide_in_cover ), cover_anim_interval
                );
            }
            else
            {
                m_cover_surface_new = surface;

                m_cover_anim_conn_slide.disconnect ();

                m_cover_anim_conn_fade.disconnect ();
                m_cover_anim_conn_fade = signal_timeout ().connect(
                    sigc::mem_fun( *this, &InfoArea::fade_out_cover ), 10
                );
            }
          }

          void
          clear_cover ()
          {
                m_cover_surface_new.reset();
                m_cover_anim_conn_fade.disconnect ();
                m_cover_anim_conn_fade = signal_timeout ().connect(
                    sigc::mem_fun( *this, &InfoArea::fade_out_cover ), 10
                );
          }

    private:

          bool
          fade_out_layout ()
          {
            Mutex::Lock L (m_layout_lock);

            m_layout_alpha = fmax(m_layout_alpha - 0.1, 0.);
            bool cond = m_layout_alpha > 0;
            if(!cond && m_text_new)
            {
                m_text_cur = m_text_new.get();
                m_layout_alpha = 1.;
            }
            else
            if(!cond && !m_text_new)
            {
                m_text_cur.reset();
                m_layout_alpha = 1.;
            }
            queue_draw ();
            return cond;
          }

          bool
          fade_out_cover ()
          {
            Mutex::Lock L (m_surface_lock);

            m_cover_alpha = fmax(m_cover_alpha - 0.1, 0.);
            bool cond = m_cover_alpha > 0;
            if(m_cover_alpha < 0.2 && m_cover_surface_new)
            {
                if(!m_cover_anim_conn_slide.connected())
                {
                    m_cover_pos = cover_anim_initial_pos;
                    m_cover_velocity = cover_anim_initial_velocity;
                    m_cover_accel = cover_anim_gravity;
                    m_cover_anim_conn_slide.disconnect ();
                    m_cover_anim_conn_slide = signal_timeout ().connect(
                        sigc::mem_fun( *this, &InfoArea::slide_in_cover ), cover_anim_interval
                    );
                }
            }
            if(!cond && m_cover_surface_new)
            {
                m_cover_surface_cur = m_cover_surface_new.get(); 
                m_cover_surface_new.reset();
                m_cover_alpha = 1.;
            }
            else
            if(!cond && !m_cover_surface_new)
            {
                m_cover_surface_cur.reset();
                m_cover_alpha = 1.;
            }
            queue_draw ();
            return cond;

          }

          bool
          slide_in_cover ()
          {
            Mutex::Lock L (m_surface_lock);

            if(!m_cover_surface_cur)
              return false;

            m_cover_pos      += m_cover_velocity * cover_anim_dt;
            m_cover_velocity += m_cover_accel    * cover_anim_dt;
            m_cover_accel     = cover_anim_gravity;

            bool r = true;

            if (m_cover_pos + m_cover_surface_cur.get()->get_width () >= cover_anim_wall)
            {
                m_cover_pos       = cover_anim_wall - m_cover_surface_cur.get()->get_width ();
                m_cover_velocity *= - (cover_anim_wall_elasticity + g_random_double_range(0.0, 0.05));

                // FIXME: Need a better test. This runs into stability problems when
                // dt or acceleration is too high

                double next_velocity = m_cover_velocity + m_cover_accel * cover_anim_dt;

                if (next_velocity >= 0.0)
                {
                    m_cover_pos      = cover_anim_wall - m_cover_surface_cur.get()->get_width ();
                    m_cover_velocity = 0.0;
                    m_cover_accel    = 0.0;

                    r = false;
                }
            }

            queue_draw ();

            if(!r)
            {
                m_cover_surface_new.reset();
            }

            return r;
          }

          void
          draw_background (Cairo::RefPtr<Cairo::Context> & cr)
          {
            Gtk::Allocation allocation = get_allocation ();
            cr->set_source_rgb(0., 0., 0.);
            Util::cairo_rounded_rect(cr, 0, 0, allocation.get_width(), allocation.get_height(), 7.);
            cr->fill ();

            /*Gdk::Cairo::set_source_color(cr, get_style()->get_background(get_state()));
            cr->set_line_width(2);
            cr->stroke();*/
          }

          void
          draw_cover (Cairo::RefPtr<Cairo::Context> & cr)
          {
            Mutex::Lock L (m_surface_lock);

            if( m_cover_surface_new && m_cover_surface_cur )
            {
                if( m_cover_pos >= cover_anim_area_x0 - m_cover_surface_new.get()->get_width ())
                {
                  cr->save ();
                  cr->rectangle (cover_anim_area_x0+(m_pressed?1:0),
                                 cover_anim_area_y0+(m_pressed?1:0), cover_anim_area_width+(m_pressed?1:0),
                                                                     cover_anim_area_height+(m_pressed?1:0));
                  cr->clip ();
                  double y = (cover_anim_area_y0 + cover_anim_area_y1 - m_cover_surface_new.get()->get_height ()) / 2;
                  draw_cairo_image (cr, m_cover_surface_new.get(), m_cover_pos + (m_pressed?1:0), y + (m_pressed?1:0), 1.);
                  cr->restore ();
                }
                cr->save ();
                cr->rectangle (cover_anim_area_x0 + (cover_anim_area_width - (cover_anim_area_width*m_cover_alpha)),
                               cover_anim_area_y0 + (cover_anim_area_height - (cover_anim_area_height*m_cover_alpha)),
                               cover_anim_area_width,
                               cover_anim_area_height);
                cr->clip ();
                cr->scale(m_cover_alpha, m_cover_alpha);
                double y = (cover_anim_area_y0 + cover_anim_area_y1 - m_cover_surface_cur.get()->get_height ()) / 2;
                draw_cairo_image (cr, m_cover_surface_cur.get(), cover_anim_area_x0, y, sqrt(m_cover_alpha));
                cr->restore ();
            }
            else
            if( m_cover_surface_cur && (m_cover_pos >= cover_anim_area_x0 - m_cover_surface_cur.get()->get_width ()) )
            {
                  cr->save ();

                  cr->rectangle (cover_anim_area_x0+(m_pressed?1:0),
                                 cover_anim_area_y0+(m_pressed?1:0), cover_anim_area_width+(m_pressed?1:0),
                                                                     cover_anim_area_height+(m_pressed?1:0));
                  cr->clip ();

                  double y = (cover_anim_area_y0 + cover_anim_area_y1 - m_cover_surface_cur.get()->get_height ()) / 2;

                  draw_cairo_image (cr, m_cover_surface_cur.get(), m_cover_pos + (m_pressed?1:0), y + (m_pressed?1:0), m_cover_alpha);

                  cr->restore ();
            }
          }

          void
          draw_text (Cairo::RefPtr<Cairo::Context> const& cr)
          {
            Mutex::Lock L (m_layout_lock);

            if(!m_text_cur)
              return;

            Gtk::Allocation allocation = get_allocation ();

            for( int n = 0; n < 3; ++n )
            {
              std::string text_cur;
              std::string text_new;

              switch(n)
              {
                  case 0:
                      text_cur = m_text_cur.get().Artist; 
                      break;
                  case 1:
                      text_cur = m_text_cur.get().Album; 
                      break;
                  case 2:
                      text_cur = m_text_cur.get().Title; 
                      break;
              }

              if(m_text_new)
              {
                  switch(n)
                  {
                      case 0:
                          text_new = m_text_new.get().Artist; 
                          break;
                      case 1:
                          text_new = m_text_new.get().Album; 
                          break;
                      case 2:
                          text_new = m_text_new.get().Title; 
                          break;
                  }
              }

              if( text_new == text_cur )
              {
                  cr->set_source_rgba (1.0, 1.0, 1.0, 1. * layout_info[n].amod); 
              }
              else    
              {
                  cr->set_source_rgba (1.0, 1.0, 1.0, m_layout_alpha * layout_info[n].amod); 
              }

              cr->set_operator (Cairo::OPERATOR_ATOP);
        
              Glib::RefPtr<Pango::Layout> layout = create_pango_layout ("");
              layout->set_markup(text_cur);
              layout->set_single_paragraph_mode (true);
              layout->set_wrap(Pango::WRAP_CHAR);
              cr->move_to(layout_info[n].x, layout_info[n].y);
              pango_cairo_show_layout (cr->cobj(), layout->gobj());
            }
          }

          void
          draw_spectrum (Cairo::RefPtr<Cairo::Context> & cr)
          {
              const int RIGHT_MARGIN = 16; 
              const int TOP_MARGIN = 6;
              const int WIDTH = 6;
              const int SPACING = 2;
              const int HEIGHT = 72;
              const double ALPHA = 0.3;

              Gtk::Allocation allocation = get_allocation ();

              for (int n = 0; n < SPECT_BANDS; ++n)
              {
                int x = 0, y = 0, w = 0, h = 0;

                // Bar
                x = allocation.get_width() - (WIDTH+SPACING)*SPECT_BANDS + (WIDTH+SPACING)*n - RIGHT_MARGIN;
                y = TOP_MARGIN + (HEIGHT - (m_spectrum_data[n]+HEIGHT)); 
                w = WIDTH; 
                h = m_spectrum_data[n]+HEIGHT;

                if( w>0 && h>0)
                {
                    cr->set_source_rgba (double(colors[n/2].red)/255., double(colors[n/2].green)/255., double(colors[n/2].blue)/255., ALPHA);
                    Util::cairo_rounded_rect(cr, x, y, w, h, 1.);
                    cr->fill ();
                }
              }
          }

          void
          draw_info (Cairo::RefPtr<Cairo::Context> & cr)
          {
            Mutex::Lock L (m_info_lock);

            if( m_info_text )
            {
                    int w, h;
                    Gtk::Allocation a = get_allocation();
                    w = a.get_width();
                    h = a.get_height();

                    Glib::RefPtr<Pango::Layout> Layout = create_pango_layout(Markup::escape_text(m_info_text.get()));
                    Pango::AttrList list;
                    Pango::Attribute attr1 = Pango::Attribute::create_attr_size(12000);
                    Pango::Attribute attr2 = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
                    list.insert(attr1);
                    list.insert(attr2);
                    Layout->set_attributes(list);

                    Pango::Rectangle Ink, Logical;
                    Layout->get_pixel_extents(Ink, Logical);

                    int x = (w/2 - Logical.get_width()/2);
                    int y = (h/2 - Logical.get_height()/2);

                    Util::cairo_rounded_rect(cr, x, y, Logical.get_width() + 24, Logical.get_height()+12, 6.);
                    cr->set_operator(Cairo::OPERATOR_ATOP);
                    cr->set_source_rgba(0.65, 0.65, 0.65, 0.55);
                    cr->fill();

                    cr->move_to(x + 12, y + 6);
                    cr->set_source_rgba(1., 1., 1., 1.);
                    pango_cairo_show_layout(cr->cobj(), Layout->gobj());
                }
          }

    protected:

          virtual bool
          on_expose_event (GdkEventExpose * event)
          {
              Widget::on_expose_event (event);
              Cairo::RefPtr<Cairo::Context> cr = get_window ()->create_cairo_context ();
              draw_background (cr);
              draw_cover (cr);
              draw_spectrum (cr);
              draw_text (cr);
              draw_info (cr);

              return true;
          }

    public:

          static InfoArea*
          create(MPX::Play & play, Glib::RefPtr<Gnome::Glade::Xml> & xml)
          {
               return new InfoArea(play,xml);
          } 

  };
}

namespace MPX
{
#define TYPE_DBUS_OBJ_MPX (DBusMPX::get_type ())
#define DBUS_OBJ_MPX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DBUS_OBJ_MPX, DBusMPX))

    struct DBusMPXClass
    {
        GObjectClass parent;
    };

    struct Player::DBusMPX
    {
        GObject parent;
        Player * player;

        enum
        {
          SIGNAL_STARTUP_COMPLETE,
          SIGNAL_SHUTDOWN_COMPLETE,
          SIGNAL_QUIT,
          N_SIGNALS,
        };

        static guint signals[N_SIGNALS];

        static gpointer parent_class;

        static GType
        get_type ();

        static DBusMPX *
        create (Player &, DBusGConnection*);

        static void
        class_init (gpointer klass,
                    gpointer class_data);

        static GObject *
        constructor (GType                   type,
                     guint                   n_construct_properties,
                     GObjectConstructParam * construct_properties);

        static gboolean
        startup (DBusMPX * self,
                 GError ** error);

        static void
        startup_complete (DBusMPX * self);
      
        static void
        shutdown_complete (DBusMPX * self);

        static void
        quit (DBusMPX * self);
    };

    gpointer Player::DBusMPX::parent_class       = 0;
    guint    Player::DBusMPX::signals[N_SIGNALS] = { 0 };

// HACK: Hackery to rename functions in glue
#define mpx_startup     startup
#define mpx_quit        quit

#include "dbus-obj-MPX-glue.h"

	void
	Player::DBusMPX::class_init (gpointer klass, gpointer class_data)
	{
        parent_class = g_type_class_peek_parent (klass);

        GObjectClass *gobject_class = reinterpret_cast<GObjectClass*>(klass);
        gobject_class->constructor  = &DBusMPX::constructor;

        signals[SIGNAL_STARTUP_COMPLETE] =
          g_signal_new ("startup-complete",
                        G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                        GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                        0,
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

        signals[SIGNAL_SHUTDOWN_COMPLETE] =
          g_signal_new ("shutdown-complete",
                        G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                        GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                        0,
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

        signals[SIGNAL_QUIT] =
          g_signal_new ("quit",
                        G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                        GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                        0,
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	}

	GObject *
	Player::DBusMPX::constructor (GType                   type,
							guint                   n_construct_properties,
							GObjectConstructParam*  construct_properties)
	{
        GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_properties, construct_properties);
        return object;
	}

	Player::DBusMPX *
	Player::DBusMPX::create (Player & player, DBusGConnection * session_bus)
	{
		dbus_g_object_type_install_info (TYPE_DBUS_OBJ_MPX, &dbus_glib_mpx_object_info);

		DBusMPX * self = DBUS_OBJ_MPX (g_object_new (TYPE_DBUS_OBJ_MPX, NULL));
		self->player = &player;

        if(session_bus)
	    {
		    dbus_g_connection_register_g_object (session_bus, "/MPX", G_OBJECT(self));
		    g_message("%s: /MPX Object registered on session DBus", G_STRLOC);
        }

        return self;
	}

	GType
	Player::DBusMPX::get_type ()
	{
        static GType type = 0;

        if (G_UNLIKELY (type == 0))
          {
            static GTypeInfo const type_info =
              {
                sizeof (DBusMPXClass),
                NULL,
                NULL,
                &class_init,
                NULL,
                NULL,
                sizeof (DBusMPX),
                0,
                NULL
              };

            type = g_type_register_static (G_TYPE_OBJECT, "MPX", &type_info, GTypeFlags (0));
          }

        return type;
	}

	gboolean
	Player::DBusMPX::startup  (DBusMPX* self,
						       GError** error)
	{
        if(self->player->m_startup_complete)
        {
            self->player->present();
        }

	    return TRUE;
	}

	void
	Player::DBusMPX::startup_complete (DBusMPX* self)
	{
	    g_signal_emit (self, signals[SIGNAL_STARTUP_COMPLETE], 0);
        self->player->m_startup_complete = true;
	}

	void
	Player::DBusMPX::shutdown_complete (DBusMPX* self)
	{
        g_signal_emit (self, signals[SIGNAL_SHUTDOWN_COMPLETE], 0);
	}

	void
	Player::DBusMPX::quit (DBusMPX *self)
	{
        gtk_main_quit ();
	}

#define TYPE_DBUS_OBJ_PLAYER (DBusPlayer::get_type ())
#define DBUS_OBJ_PLAYER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DBUS_OBJ_PLAYER, DBusPlayer))

    struct DBusPlayerClass
    {
        GObjectClass parent;
    };

    struct Player::DBusPlayer
    {
        GObject parent;
        Player * player;

        enum
        {
          PLAYER_SIGNAL_CAPS,
          PLAYER_SIGNAL_STATE,
          N_SIGNALS,
        };

        static guint signals[N_SIGNALS];

        static gpointer parent_class;

        static GType
        get_type ();

        static DBusPlayer *
        create (Player &, DBusGConnection*);

        static void
        class_init (gpointer klass,
                    gpointer class_data);

        static GObject *
        constructor (GType                   type,
                     guint                   n_construct_properties,
                     GObjectConstructParam * construct_properties);

        static gboolean
        get_metadata (DBusPlayer* self,
                      GHashTable** metadata,
                      GError** error);

        static gboolean
        play_tracks (DBusPlayer* self,
                     char** uris,
                     GError** error);
    };

    gpointer Player::DBusPlayer::parent_class       = 0;
    guint    Player::DBusPlayer::signals[N_SIGNALS] = { 0 };

// HACK: Hackery to rename functions in glue
#define player_next next 
#define player_prev prev
#define player_pause pause
#define player_stop stop
#define player_play play
#define player_get_metadata get_metadata
#define player_play_tracks play_tracks

#include "dbus-obj-PLAYER-glue.h"

	void
	Player::DBusPlayer::class_init (gpointer klass, gpointer class_data)
	{
	  parent_class = g_type_class_peek_parent (klass);

	  GObjectClass *gobject_class = reinterpret_cast<GObjectClass*>(klass);
	  gobject_class->constructor  = &DBusPlayer::constructor;
	}

	GObject *
	Player::DBusPlayer::constructor (GType                   type,
							guint                   n_construct_properties,
							GObjectConstructParam*  construct_properties)
	{
	  GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_properties, construct_properties);

	  return object;
	}

	Player::DBusPlayer *
	Player::DBusPlayer::create (Player & player, DBusGConnection * session_bus)
	{
		dbus_g_object_type_install_info (TYPE_DBUS_OBJ_PLAYER, &dbus_glib_player_object_info);

		DBusPlayer * self = DBUS_OBJ_PLAYER (g_object_new (TYPE_DBUS_OBJ_PLAYER, NULL));
		self->player = &player;

	  if(session_bus)
	  {
		dbus_g_connection_register_g_object (session_bus, "/Player", G_OBJECT(self));
		g_message("%s: /Player Object registered on session DBus", G_STRLOC);
	  }

	  return self;
	}

	GType
	Player::DBusPlayer::get_type ()
	{
	  static GType type = 0;

	  if (G_UNLIKELY (type == 0))
		{
		  static GTypeInfo const type_info =
			{
			  sizeof (DBusPlayerClass),
			  NULL,
			  NULL,
			  &class_init,
			  NULL,
			  NULL,
			  sizeof (DBusPlayer),
			  0,
			  NULL
			};

		  type = g_type_register_static (G_TYPE_OBJECT, "Player", &type_info, GTypeFlags (0));
		}

	  return type;
	}
    
    const char* mpris_attribute_id_str[] =
    {
        "location",
        "title",
        "genre",
        "comment",
        "puid fingerprint",
        "mpx tag hash",
        "mb track id",
        "artist",
        "mb artist sort name",
        "mb artist id",
        "album",
        "mb album id",
        "mb release date",
        "mb release country",
        "mb release type",
        "asin",
        "mb album artist",
        "mb album artist sort name",
        "mb album artist id",
        "mime type",
        "mpx hal volume udi",
        "mpx hal device udi",
        "mpx hal volume relative path",
        "mpx insert path",
        "mpx location name",
    };

    const char* mpris_attribute_id_int[] =
    {
        "tracknumber",
        "time",
        "rating",
        "year",
        "mtime",
        "audio-bitrate",
        "audio-samplerate",
        "mpx play count",
        "mpx play date",
        "mpx insert date",
        "mpx is mb album artist",
        "mpx active",
        "mpx track id",
        "mpx album id",
    };

    gboolean
    Player::DBusPlayer::get_metadata (DBusPlayer* self,
                                      GHashTable** metadata,
                                      GError** error)
    {
        MPX::Metadata m = self->player->get_metadata();

        GHashTable * table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
        GValue* value = 0;

        for(int n = ATTRIBUTE_LOCATION; n < N_ATTRIBUTES_STRING; ++n)
        {
            if(m[n].is_initialized())
            {
                value = g_new0(GValue,1);
                g_value_init(value, G_TYPE_STRING);
                g_value_set_string(value, (boost::get<std::string>(m[n].get())).c_str());
                g_hash_table_insert(table, g_strdup(mpris_attribute_id_str[n-ATTRIBUTE_LOCATION]), value);
            }
        }

        for(int n = ATTRIBUTE_TRACK; n < N_ATTRIBUTES_INT; ++n)
        {
            if(m[n].is_initialized())
            {
                value = g_new0(GValue,1);
                g_value_init(value, G_TYPE_INT64);
                g_value_set_int64(value, boost::get<gint64>(m[n].get()));
                g_hash_table_insert(table, g_strdup(mpris_attribute_id_int[n-ATTRIBUTE_TRACK]), value);
            }
        }

        *metadata = table;
        return TRUE;
    }

    gboolean
    Player::DBusPlayer::play_tracks (DBusPlayer* self,
                                     char** uris,
                                     GError** error)
    {
        Util::FileList list;
        if( uris )
        {
            while( *uris )
            {
                list.push_back( *uris );
                ++uris;
            }
        }
        self->player->play_tracks(list);
        return TRUE;
    }


#ifdef HAVE_HAL
    Player::Player(const Glib::RefPtr<Gnome::Glade::Xml>& xml,
                   MPX::Library & obj_library,
                   MPX::Covers & obj_amazon,
                   MPX::HAL & obj_hal)
#else
    Player::Player(const Glib::RefPtr<Gnome::Glade::Xml>& xml,
                   MPX::Library & obj_library,
                   MPX::Covers & obj_amazon)
#endif // HAVE_HAL
    : WidgetLoader<Gtk::Window>(xml, "mpx")
    , sigx::glib_auto_dispatchable()
    , m_ref_xml(xml)
    , m_startup_complete(false)
	, m_NextSourceId(0)
	, m_SourceUI(0)
    , m_Covers(obj_amazon)
#ifdef HAVE_HAL
    , m_HAL(obj_hal)
#endif // HAVE_HAL
    , m_Library(obj_library)
   {
        m_ErrorManager = new ErrorManager;

        Splashscreen splash;
        splash.set_message(_("Startup..."), 0.);

        mpx_py_init ();

        m_ref_xml->get_widget("statusbar", m_Statusbar);
        m_ref_xml->get_widget("notebook-info", m_InfoNotebook);
        m_ref_xml->get_widget("volumebutton", m_Volume);

		m_PluginManager = new PluginManager(this);
        m_Sidebar = new Sidebar(m_ref_xml, *m_PluginManager);

        try{

            std::list<Glib::RefPtr<Gdk::Pixbuf> > icon_list;

            icon_list.push_back(Gdk::Pixbuf::create_from_file(
                build_filename(
                DATA_DIR,
                "icons" G_DIR_SEPARATOR_S "mpx.png"
            )));

            icon_list.push_back(Gdk::Pixbuf::create_from_file(
                build_filename(
                DATA_DIR,
                "icons" G_DIR_SEPARATOR_S "mpx_128.png"
            )));

            icon_list.push_back(Gdk::Pixbuf::create_from_file(
                build_filename(
                DATA_DIR,
                "icons" G_DIR_SEPARATOR_S "mpx_64.png"
            )));

            icon_list.push_back(Gdk::Pixbuf::create_from_file(
                build_filename(
                DATA_DIR,
                "icons" G_DIR_SEPARATOR_S "mpx_48.png"
            )));

            icon_list.push_back(Gdk::Pixbuf::create_from_file(
                build_filename(
                DATA_DIR,
                 "icons" G_DIR_SEPARATOR_S "mpx_16.png"
            )));

            set_icon_list(icon_list);

        } catch (Gdk::PixbufError & cxe)
        {
            g_warning("%s: Couldn't set main window icon", G_STRLOC);
        }

        IconTheme::get_default()->prepend_search_path(build_filename(DATA_DIR,"icons"));
		register_default_stock_icons();

        splash.set_message(_("Initializing Playback Engine"), 0.2);


        m_Play = new Play();

		m_Play->signal_eos().connect(
            sigc::mem_fun( *this, &MPX::Player::on_play_eos
        ));

		m_Play->signal_position().connect(
            sigc::mem_fun( *this, &MPX::Player::on_play_position
        ));

		m_Play->signal_metadata().connect(
            sigc::mem_fun( *this, &MPX::Player::on_play_metadata
        ));

		m_Play->signal_buffering().connect(
            sigc::mem_fun( *this, &MPX::Player::on_play_buffering
        ));

		m_Play->property_status().signal_changed().connect(
            sigc::mem_fun( *this, &MPX::Player::on_play_status_changed
        ));

		m_Play->signal_stream_switched().connect(
            sigc::mem_fun( *this, &MPX::Player::on_stream_switched
        ));


        m_VideoWidget = manage(new VideoWidget(m_Play));
        m_VideoWidget->show ();
        gtk_widget_realize (GTK_WIDGET (m_VideoWidget->gobj()));

        if( m_Play->has_video() )
        {
            m_Play->signal_request_window_id ().connect
                    (sigc::mem_fun( *this, &Player::on_gst_set_window_id ));
            m_Play->signal_video_geom ().connect
                    (sigc::mem_fun( *this, &Player::on_gst_set_window_geom ));
        }
			  
        m_Preferences = Preferences::create(*m_Play);

#ifdef HAVE_HAL
        m_MLibManager = MLibManager::create(obj_hal, obj_library);
#endif // HAVE_HAL

        m_scrolllock_mask   = 0;
        m_numlock_mask      = 0;
        m_capslock_mask     = 0;
        mmkeys_get_offending_modifiers ();
        on_mm_edit_done (); // bootstraps the settings

        m_Preferences->signal_mm_edit_begin().connect(
            sigc::mem_fun( *this, &Player::on_mm_edit_begin
        ));

        m_Preferences->signal_mm_edit_done().connect(
            sigc::mem_fun( *this, &Player::on_mm_edit_done
        ));

		signals[PSIGNAL_NEW_TRACK] =
			g_signal_new ("new-track",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_FIRST),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

		signals[PSIGNAL_TRACK_PLAYED] =
			g_signal_new ("track-played",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_FIRST),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

		signals[PSIGNAL_INFOAREA_CLICK] =
			g_signal_new ("infoarea-click",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_FIRST),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

		signals[PSIGNAL_STATUS_CHANGED] =
			g_signal_new ("play-status-changed",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_FIRST),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT); 

		signals[PSIGNAL_METADATA_PREPARE] =
			g_signal_new ("metadata-prepare",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_FIRST),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

		signals[PSIGNAL_METADATA_UPDATED] =
			g_signal_new ("metadata-updated",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_FIRST),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

		signals[PSIGNAL_NEW_COVERART] =
			g_signal_new ("new-coverart",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_FIRST),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 0);

		signals[PSIGNAL_NEW_SOURCE] =
			g_signal_new ("new-source",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_FIRST),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1, g_type_from_name("PyObject")); 

		m_DiscDefault = Gdk::Pixbuf::create_from_file(
            build_filename(
                DATA_DIR,
			    build_filename(
                    "images",
                    "disc-default.png"
        )))->scale_simple(64,64,Gdk::INTERP_BILINEAR);

        splash.set_message(_("Setting up DBus"), 0.4);

		init_dbus ();
		DBusObjects.mpx = DBusMPX::create(*this, m_SessionBus);
        DBusObjects.player = DBusPlayer::create(*this, m_SessionBus);

        splash.set_message(_("Loading Plugins"), 0.6);
	
		/*- Connect Library -----------------------------------------------*/ 

        m_Library.scanner().signal_scan_start().connect(
            sigc::mem_fun( *this, &Player::on_library_scan_start
        ));

        m_Library.scanner().signal_scan_run().connect(
            sigc::mem_fun( *this, &Player::on_library_scan_run
        ));

        m_Library.scanner().signal_scan_end().connect(
            sigc::mem_fun( *this, &Player::on_library_scan_end
        ));

        /*- UIManager + Menus + Proxy Widgets -----------------------------*/

		m_ui_manager = UIManager::create ();

		m_actions = ActionGroup::create ("Actions_Player");

		m_actions->add(Action::create(
            "MenuMusic",
             _("_Music")
        ));

		m_actions->add(Action::create(
            "MenuEdit",
            _("_Edit")
        ));

		m_actions->add(Action::create(
            "MenuView",
            _("_View")
        ));

		m_actions->add(Action::create(
            "MenuTrack",
            _("_Track")
        ));

		m_actions->add (Action::create ("action-play-files",
										Gtk::Stock::OPEN,
										_("_Play Files...")),
        sigc::mem_fun (*this, &Player::on_play_files));

#ifndef HAVE_HAL
		m_actions->add (Action::create ("action-import-folder",
										Gtk::Stock::HARDDISK,
										_("Import _Folder")),
		sigc::mem_fun (*this, &Player::on_import_folder));

		m_actions->add (Action::create ("action-import-share",
										Gtk::Stock::NETWORK,
										_("Import _Share")),
        sigc::mem_fun (*this, &Player::on_import_share));
#else

		m_actions->add (Action::create ("action-mlibmanager",
										_("Manage Music Library...")),
	    sigc::mem_fun (*m_MLibManager, &MLibManager::present));
#endif


		m_actions->add (Action::create ("action-vacuum-lib",
										_("Vacuum Music Library")),
	    sigc::mem_fun (m_Library, &Library::vacuum));



		m_actions->add (Action::create (ACTION_PLUGINS,
										Gtk::StockID(MPX_STOCK_PLUGIN),
										_("Manage Plugins...")),
	    sigc::mem_fun (*this, &Player::on_show_plugins ));



		m_actions->add (Action::create (ACTION_PREFERENCES,
										Gtk::Stock::EXECUTE,
										_("Edit Preferences...")),
	    sigc::mem_fun (*m_Preferences, &Gtk::Widget::show ));



		m_actions->add (Action::create ("action-quit",
										Gtk::Stock::QUIT,
										_("_Quit")),
										AccelKey("<ctrl>Q"),
		sigc::ptr_fun ( &Gtk::Main::quit ));



		m_actions->add (Action::create (ACTION_PLAY,
										Gtk::Stock::MEDIA_PLAY,
										_("Play")),
		sigc::mem_fun (*this, &Player::on_controls_play ));



		m_actions->add (ToggleAction::create (ACTION_PAUSE,
										Gtk::Stock::MEDIA_PAUSE,
										_("Pause")),
		sigc::mem_fun (*this, &Player::on_controls_pause ));


		m_actions->add (Action::create (ACTION_STOP,
										Gtk::Stock::MEDIA_STOP,
										_("Stop")),
        sigc::mem_fun (*this, &Player::stop ));


		m_actions->add (Action::create (ACTION_NEXT,
										Gtk::Stock::MEDIA_NEXT,
										_("Next")),
	    sigc::mem_fun (*this, &Player::on_controls_next ));


		m_actions->add (Action::create (ACTION_PREV,
										Gtk::Stock::MEDIA_PREVIOUS,
										_("Prev")),
        sigc::mem_fun (*this, &Player::on_controls_prev ));


		m_ui_manager->insert_action_group (m_actions);

		if(Util::ui_manager_add_ui(m_ui_manager, MenubarMain, *this, _("Main Menubar")))
		{
			dynamic_cast<Alignment*>(
                m_ref_xml->get_widget("alignment-menu")
            )->add(
                *(m_ui_manager->get_widget ("/MenubarMain"))
            );
		}

		/*- Playback Controls ---------------------------------------------*/ 

		m_actions->get_action (ACTION_PLAY)->connect_proxy
			  (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-play"))));
		m_actions->get_action (ACTION_PREV)->connect_proxy
			  (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-previous"))));
		m_actions->get_action (ACTION_NEXT)->connect_proxy
			  (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-next"))));
		m_actions->get_action (ACTION_PAUSE)->connect_proxy
			  (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-pause"))));
		m_actions->get_action (ACTION_STOP)->connect_proxy
			  (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("controls-stop"))));

		m_actions->get_action (ACTION_PLAY)->set_sensitive( false );
		m_actions->get_action (ACTION_PAUSE)->set_sensitive( false );
		m_actions->get_action (ACTION_PREV)->set_sensitive( false );
		m_actions->get_action (ACTION_NEXT)->set_sensitive( false );
		m_actions->get_action (ACTION_STOP)->set_sensitive( false );

        add_accel_group (m_ui_manager->get_accel_group());

        splash.set_message(_("Loading Sources"), 0.8);

		/*- Load Sources --------------------------------------------------*/ 

		m_Sidebar->signal_id_changed().connect(
            sigc::mem_fun(
                *this,
                &Player::on_source_changed
        ));

        m_Sidebar->addItem(
            _("Now Playing"),
            m_VideoWidget,
            0,
            Gdk::Pixbuf::create_from_file(
                build_filename(
                    DATA_DIR,
                    "images" G_DIR_SEPARATOR_S "now-playing.png"
                )
            )->scale_simple(20,20,Gdk::INTERP_BILINEAR),
            0 
        );

        m_Sidebar->addItem(
            _("Plugins"),
            m_InfoNotebook,
            0,
            Gdk::Pixbuf::create_from_file(
                build_filename(
                    DATA_DIR,
                    "images" G_DIR_SEPARATOR_S "trackdetails.png"
                )
            )->scale_simple(20,20,Gdk::INTERP_BILINEAR),
            1 
        );

        m_NextSourceId = 2;

        std::string sources_path = build_filename(PLUGIN_DIR, "sources");
        if(file_test(sources_path, FILE_TEST_EXISTS))
        {
    		Util::dir_for_each_entry(
                sources_path,
                sigc::mem_fun(*this, &MPX::Player::source_load)
            );  
        }

		/*- Volume ---------------------------------------------------------*/

		std::vector<Glib::ustring> Icons;
		Icons.push_back("audio-volume-muted");
		Icons.push_back("audio-volume-high");
		Icons.push_back("audio-volume-low");
		Icons.push_back("audio-volume-medium");
		m_Volume->property_size() = Gtk::ICON_SIZE_SMALL_TOOLBAR;
		m_Volume->set_icons(Icons);

		m_Volume->signal_value_changed().connect(
            sigc::mem_fun( *this, &Player::on_volume_value_changed
        ));

        m_Volume->set_value(double(mcs->key_get<int>("mpx", "volume")));


		/*- Time -----------------------------------------------------------*/

        m_ref_xml->get_widget("label-time", m_TimeLabel);

		/*- Seek -----------------------------------------------------------*/

        m_ref_xml->get_widget("scale-seek", m_Seek);

		m_Seek->signal_event().connect(
            sigc::mem_fun( *this, &Player::on_seek_event
        ));

		m_Seek->set_sensitive(false);
		g_atomic_int_set(&m_Seeking,0);

        /*- Infoarea--------------------------------------------------------*/

        m_InfoArea = InfoArea::create(*m_Play, m_ref_xml);

		m_InfoArea->signal_cover_clicked().connect
		  (sigc::mem_fun( *this, &Player::on_cover_clicked
        ));

		m_InfoArea->signal_uris_dropped().connect
		  (sigc::mem_fun( *this, &MPX::Player::play_tracks
        ));

		/*- Load Plugins -------------------------------------------------*/

		std::string const user_path =
            build_filename(
                build_filename(
                    g_get_user_data_dir(),
                    "mpx"),
                "python-plugins"
            );

		if(file_test(user_path, FILE_TEST_EXISTS))
        {
			m_PluginManager->append_search_path (user_path);
        }

		m_PluginManager->append_search_path
            (build_filename(
                DATA_DIR,
                "python-plugins"
        ));

		m_PluginManager->load_plugins();
		m_PluginManager->activate_plugins();
		m_PluginManagerGUI = PluginManagerGUI::create(*m_PluginManager);

        splash.set_message(_("Ready"), 1.0);

        resize(
            mcs->key_get<int>("mpx","window-w"),
            mcs->key_get<int>("mpx","window-h")
        );

        move(
            mcs->key_get<int>("mpx","window-x"),
            mcs->key_get<int>("mpx","window-y")
        );

#if 0
        m_CoverFlow = new CoverFlowWidget;
        CoverFlowEngine::SongList songs;

        SQL::RowV v;
        obj_library.getSQL(v, "SELECT DISTINCT mb_album_id FROM album WHERE mb_album_id IS NOT NULL;");
        for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
        {
            std::string asin = boost::get<std::string>((*i)["mb_album_id"]);
            songs.push_back(std::make_pair(asin, asin));
        }
        m_CoverFlow->load_covers(songs);
        m_CoverFlow->show ();
        add_widget(m_CoverFlow);
#endif

		show ();

		DBusObjects.mpx->startup_complete(DBusObjects.mpx);
    }

	void
	Player::init_dbus ()
	{
		DBusGProxy * m_SessionBus_proxy;
		guint dbus_request_name_result;
		GError * error = NULL;

		m_SessionBus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
		if (!m_SessionBus)
		{
		  g_error_free (error);
		  return;
		}

		m_SessionBus_proxy = dbus_g_proxy_new_for_name (m_SessionBus,
													   "org.freedesktop.DBus",
													   "/org/freedesktop/DBus",
													   "org.freedesktop.DBus");

		if (!dbus_g_proxy_call (m_SessionBus_proxy, "RequestName", &error,
								G_TYPE_STRING, "info.backtrace.mpx",
								G_TYPE_UINT, 0,
								G_TYPE_INVALID,
								G_TYPE_UINT, &dbus_request_name_result,
								G_TYPE_INVALID))
		{
		  g_critical ("%s: RequestName Request Failed: %s", G_STRFUNC, error->message);
		  g_error_free (error);
		  error = NULL;
		}
		else
		{
		  switch (dbus_request_name_result)
		  {
			case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
			{
			  break;
			}

			case DBUS_REQUEST_NAME_REPLY_EXISTS:
			{
			  g_object_unref(m_SessionBus);
			  m_SessionBus = NULL;
			  return;
			}
		  }
		}

		g_message("%s: info.backtrace.mpx service registered on session DBus", G_STRLOC);
	}

    bool
    Player::source_load (std::string const& path)
    {
		enum
		{
			LIB_BASENAME,
			LIB_PLUGNAME,
			LIB_SUFFIX
		};

		const std::string type = "mpxsource";

		std::string basename (path_get_basename (path));
		std::string pathname (path_get_dirname  (path));

		if (!is_module (basename))
			return false;

		StrV subs; 
		split (subs, basename, is_any_of ("-."));
		std::string name  = type + std::string("-") + subs[LIB_PLUGNAME];
		std::string mpath = Module::build_path (build_filename(PLUGIN_DIR, "sources"), name);

		Module module (mpath, ModuleFlags (0)); 
		if (!module)
		{
			g_message("Source plugin load FAILURE '%s': %s", mpath.c_str (), module.get_last_error().c_str());
			return false;
		}

		g_message("LOADING Source plugin: %s", mpath.c_str ());

		module.make_resident();

		SourcePluginPtr plugin = SourcePluginPtr (new SourcePlugin());
		if (!g_module_symbol (module.gobj(), "get_instance", (gpointer*)(&plugin->get_instance)))
		{
          g_message("Source plugin load FAILURE '%s': get_instance hook missing", mpath.c_str ());
		  return false;
		}

        if (!g_module_symbol (module.gobj(), "del_instance", (gpointer*)(&plugin->del_instance)))
		{
          g_message("Source plugin load FAILURE '%s': del_instance hook missing", mpath.c_str ());
		  return false;
		}

		
        PlaybackSource * p = plugin->get_instance(m_ui_manager, *this); 
        Gtk::Alignment * a = new Gtk::Alignment;

        if(p->get_ui()->get_parent())
            p->get_ui()->reparent(*a);
        else
            a->add(*p->get_ui());
        a->show();

        m_Sidebar->addItem(
            p->get_name(),
            a,  
            p,
            p->get_icon()->scale_simple(
                    20,
                    20,
                    Gdk::INTERP_BILINEAR
            ),
            m_NextSourceId
        );

        ItemKey key (boost::optional<gint64>(), m_NextSourceId++);
		m_Sources[key] = p;
		source_install(key);

		return false;
    }

	void
	Player::source_install (ItemKey const& source_id)
	{
        g_message("%s: Installing Source '%s'", G_STRLOC, m_Sources[source_id]->get_name().c_str());

        m_Sources[source_id]->set_key(source_id);

		m_Sources[source_id]->signal_caps().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_caps), source_id));

		m_Sources[source_id]->signal_flags().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_flags), source_id));

		m_Sources[source_id]->signal_playback_request().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_play_request), source_id));

		m_Sources[source_id]->signal_stop_request().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_stop), source_id));

		m_Sources[source_id]->signal_play_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::play_async_cb), source_id));

		m_Sources[source_id]->signal_next_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::next_async_cb), source_id));

		m_Sources[source_id]->signal_prev_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::prev_async_cb), source_id));

		m_Sources[source_id]->signal_segment().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_segment), source_id));

		m_Sources[source_id]->signal_track_metadata().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_track_metadata), source_id));

		m_Sources[source_id]->signal_items().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_items), source_id));

		m_Sources[source_id]->send_caps();
		m_Sources[source_id]->send_flags();

		UriSchemes v = m_Sources[source_id]->get_uri_schemes ();
		if(v.size())
		{
		  for(UriSchemes::const_iterator i = v.begin(); i != v.end(); ++i)
		  {
				if(m_UriMap.find(*i) != m_UriMap.end())
				{
				  g_warning("%s: Source '%s' tried to override URI handler for scheme '%s'!",
					  G_STRLOC,
					  m_Sources[source_id]->get_name().c_str(),
					  i->c_str());	
				  continue;
				}
				g_message("%s: Source '%s' registers for URI scheme '%s'", G_STRLOC, m_Sources[source_id]->get_name().c_str(), i->c_str()); 
				m_UriMap.insert (std::make_pair (std::string(*i), source_id));
		  }
		}

        m_Sources[source_id]->post_install();

#if 0
        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
        try{
            using namespace boost::python;
            object obj(boost::ref(m_Sources[source_id]));
    		g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_NEW_SOURCE], 0, obj.ptr());
        } catch( boost::python::error_already_set )
        {
            PyErr_Print();
        }
        pyg_gil_state_release(state);
#endif
	}

    void
    Player::play_tracks (Util::FileList const& uris)
    {
        if( uris.size() )
        {
            try{
                URI u (uris[0]); //TODO/FIXME: We assume all URIs are of the same scheme
                UriSchemeMap::const_iterator i = m_UriMap.find (u.scheme);
                if( i != m_UriMap.end ())
                {
                  m_Sources[i->second]->process_uri_list( uris, true );
                }
              }
            catch (URI::ParseError & cxe)
              {
                g_warning (G_STRLOC ": Couldn't parse URI %s", uris[0].c_str());
              }
        }
    }

    void
    Player::play_uri (std::string const& uri)
    {
        Util::FileList uris;
        uris.push_back(uri);
        try{
            URI u (uris[0]); //TODO/FIXME: We assume all URIs are of the same scheme
            UriSchemeMap::const_iterator i = m_UriMap.find (u.scheme);
            if( i != m_UriMap.end ())
            {
              m_Sources[i->second]->process_uri_list( uris, true );
            }
          }
        catch (URI::ParseError & cxe)
          {
            g_warning (G_STRLOC ": Couldn't parse URI %s", uris[0].c_str());
          }
    }

    void
    Player::on_source_items(gint64 count, ItemKey const& key)
    {
        m_Sidebar->setItemCount(key, count);
    }

	void
	Player::on_play_files ()
	{
		typedef boost::shared_ptr<FileBrowser> FBRefP;
		FBRefP p (FileBrowser::create());
		int response = p->run ();
		p->hide ();

		if( response == GTK_RESPONSE_OK )
		{
			Util::FileList uris = p->get_uris ();

			if( uris.size() ) 
			{
				try{
					URI u (uris[0]); //TODO/FIXME: We assume all URIs are of the same scheme
					UriSchemeMap::const_iterator i = m_UriMap.find (u.scheme);
					if( i != m_UriMap.end ())
					{
					  m_Sources[i->second]->process_uri_list( uris, true );
					}
				  }
				catch (URI::ParseError & cxe)
				  {
					g_warning (G_STRLOC ": Couldn't parse URI %s", uris[0].c_str());
				  }
			}
		}
	}

	void
	Player::on_show_plugins ()
	{
		m_PluginManagerGUI->present ();
	}

    PyObject*
    Player::get_source(std::string const& uuid)
    {
        for(SourcesMap::iterator i = m_Sources.begin(); i != m_Sources.end(); ++i)
        {
            if((*i).second->get_guid() == uuid)
            {
                return (*i).second->get_py_obj();
            }
        }

        Py_INCREF(Py_None);
        return Py_None;
    }

    PyObject*
    Player::get_sources_by_class(std::string const& uuid)
    {
        using namespace boost::python;
        boost::python::list l;
        for(SourcesMap::iterator i = m_Sources.begin(); i != m_Sources.end(); ++i)
        {
            if((*i).second->get_class_guid() == uuid)
            {
                l.append(object(handle<>(borrowed((*i).second->get_py_obj()))));
            }
        }

        Py_INCREF(l.ptr());
        return l.ptr();
    }

	void
	Player::get_object (PAccess<MPX::Library> & pa)
	{
		pa = PAccess<MPX::Library>(m_Library);
	}

	void	
	Player::get_object (PAccess<MPX::Covers> & pa)
	{
		pa = PAccess<MPX::Covers>(m_Covers);
	}

	void
	Player::get_object (PAccess<MPX::Play> & pa)
	{
		pa = PAccess<MPX::Play>(*m_Play);
	}

#ifdef HAVE_HAL
	void
	Player::get_object (PAccess<MPX::HAL> & pa)
	{
		pa = PAccess<MPX::HAL>(m_HAL);
	}
#endif // HAVE_HAL

    Glib::RefPtr<Gtk::UIManager>&
    Player::ui ()
    {
        return m_ui_manager;
    }

    MPXPlaystatus
    Player::get_status ()
    {
        return MPXPlaystatus(m_Play->property_status().get_value());
    }


	Metadata&
	Player::get_metadata ()
	{
        if(m_Metadata)
        {
    		return m_Metadata.get();
        }
        else
        {
            throw std::runtime_error("No Current Metadata");
        }
	}

	void
	Player::add_widget (Gtk::Widget *widget)
	{
		dynamic_cast<Gtk::Box*>(m_ref_xml->get_widget("vbox3"))->pack_start(*widget);
	}

	void
	Player::add_info_widget (Gtk::Widget *widget, std::string const& name)
	{
        widget->show();
        m_InfoNotebook->append_page(*widget, name);
        m_InfoWidgetMap.insert(std::make_pair(widget, widget));
	}
   
    void
    Player::add_subsource(PlaybackSource* p, ItemKey const& parent, gint64 id)
    {
        Gtk::Alignment * a = new Gtk::Alignment;

        if(p->get_ui()->get_parent())
            p->get_ui()->reparent(*a);
        else
            a->add(*p->get_ui());
        a->show();

        m_Sidebar->addSubItem(
            p->get_name(),
            a,  
            p,
            p->get_icon()->scale_simple(
                    20,
                    20,
                    Gdk::INTERP_BILINEAR
            ),
            parent.second,
            id
        );
        ItemKey key (parent.second, id);
		m_Sources[key] = p;
		source_install(key);
    }

	void
	Player::remove_widget (Gtk::Widget *widget)
	{
		Gtk::VBox * box;
		m_ref_xml->get_widget("vbox-top", box);
		box->remove(*widget);
	}

	void
	Player::remove_info_widget (Gtk::Widget *widget)
    {
        WidgetWidgetMap::iterator i = m_InfoWidgetMap.find(widget);
        if( i != m_InfoWidgetMap.end() )
        {
            m_InfoNotebook->remove(*((*i).second));
        }
    }

	void
	Player::on_volume_value_changed (double volume)
	{
		m_Play->property_volume() = volume*100;	
		mcs->key_set("mpx","volume", int(volume*100));
	}

	bool
	Player::on_seek_event (GdkEvent *event)
	{
	  if( event->type == GDK_KEY_PRESS )
	  {
			  GdkEventKey * ev = ((GdkEventKey*)(event));
			  gint64 status = m_Play->property_status().get_value();
			  if((status == PLAYSTATUS_PLAYING) || (status == PLAYSTATUS_PAUSED))
			  {
					  gint64 pos = m_Play->property_position().get_value();

					  int delta = (ev->state & GDK_SHIFT_MASK) ? 1 : 15;

					  if(ev->keyval == GDK_Left)
					  {
						  m_Play->seek( pos - delta );
						  m_TrackPlayedSeconds -= delta;
						  return true;
					  }
					  else if(ev->keyval == GDK_Right)
					  {
						  m_Play->seek( pos + delta );
						  return true;
					  }
			  }
			  return false;
	  }
	  else if( event->type == GDK_BUTTON_PRESS )
	  {
		g_atomic_int_set(&m_Seeking,1);
		m_PreSeekPosition = m_Play->property_position().get_value();
		goto SET_SEEK_POSITION;
	  }
	  else if( event->type == GDK_BUTTON_RELEASE && g_atomic_int_get(&m_Seeking))
	  {
		g_atomic_int_set(&m_Seeking,0);
		if(m_PreSeekPosition > m_Seek->get_value())
		{
			m_TrackPlayedSeconds -= (m_PreSeekPosition - m_Seek->get_value());
		}
		m_Play->seek (gint64(m_Seek->get_value()));
	  }
	  else if( event->type == GDK_MOTION_NOTIFY && g_atomic_int_get(&m_Seeking))
	  {
		SET_SEEK_POSITION:

		guint64 duration = m_Play->property_duration().get_value();
		guint64 position = m_Seek->get_value();

		guint64 m_pos = position / 60;
		guint64 m_dur = duration / 60;
		guint64 s_pos = position % 60;
		guint64 s_dur = duration % 60;

		static boost::format time_f ("%02d:%02d … %02d:%02d");

		m_TimeLabel->set_text ((time_f % m_pos % s_pos % m_dur % s_dur).str());
	  }
	  return false;
	}

	void
	Player::on_play_metadata (MPXGstMetadataField field)
	{
        Glib::Mutex::Lock L (m_MetadataLock);

		MPXGstMetadata const& m = m_Play->get_metadata();

		switch (field)
		{
		  case FIELD_IMAGE:
            if(!m_Metadata.get().Image)
            {
			    m_Metadata.get().Image = m.m_image.get();
                if(m_Metadata.get().Image)
                {
			        m_InfoArea->set_cover (m.m_image.get()->scale_simple (72, 72, Gdk::INTERP_HYPER));

                    PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
                    g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_NEW_COVERART], 0);
                    check_py_error();
                    pyg_gil_state_release(state);
                }
            }
			return;
	
		  case FIELD_TITLE:
			if(m_Metadata.get()[ATTRIBUTE_TITLE])
			{
				std::string album = get<std::string>(m_Metadata.get()[ATTRIBUTE_TITLE].get());
				if(album == m.m_title.get())
					return;
			}

			m_Metadata.get()[ATTRIBUTE_TITLE] = std::string(m.m_title.get());
			metadata_reparse();
			return;

		  case FIELD_ALBUM:
			if(m_Metadata.get()[ATTRIBUTE_ALBUM])
			{
				std::string album = get<std::string>(m_Metadata.get()[ATTRIBUTE_ALBUM].get());
				if(album == m.m_album.get())
					return;
			}

			m_Metadata.get()[ATTRIBUTE_ALBUM] = std::string(m.m_album.get());
			metadata_reparse();
			break;

		  case FIELD_AUDIO_BITRATE:
			break;

		  case FIELD_AUDIO_CODEC:
			break;

		  case FIELD_VIDEO_CODEC:
			break;
		}
	}

	void
	Player::on_play_position (guint64 position)
	{
	  if (g_atomic_int_get(&m_Seeking))
		return;

	  guint64 duration = m_Play->property_duration().get_value();

	  if( (duration > 0) && (position <= duration) && (position >= 0) )
	  {
		if (duration <= 0)
		  return;

		if (position < 0)
		  return;

		guint64 m_pos = position / 60;
		guint64 s_pos = position % 60;
		guint64 m_dur = duration / 60;
		guint64 s_dur = duration % 60;

		static boost::format time_f ("%02d:%02d … %02d:%02d");

		m_TimeLabel->set_text((time_f % m_pos % s_pos % m_dur % s_dur).str());
		m_Seek->set_range(0., duration);
		m_Seek->set_value(double (position));

		m_TrackPlayedSeconds += 0.5; // this is slightly incorrect, the tick is every 500ms, but nothing says that the time always also progresses by exactly 0.5s
		m_TrackDuration = m_Play->property_duration().get_value();
	  }
	}

    void
    Player::on_play_buffering (double size)
    {
        if( size >= 1.0 )
        {
            m_InfoArea->set_info((boost::format (_("Buffering: %d")) % (int(size*100))).str());
            m_InfoArea->clear_info();
        }
        else
        {
            m_InfoArea->set_info((boost::format (_("Buffering: %d")) % (int(size*100))).str());
        }
    }

#ifdef HAVE_HAL
    Player*
    Player::create (MPX::Library & obj_library, MPX::Covers & obj_amazn, MPX::HAL & obj_hal)
    {
		const std::string path (build_filename(DATA_DIR, build_filename("glade","mpx.glade")));
		Player * p = new Player(Gnome::Glade::Xml::create (path), obj_library, obj_amazn, obj_hal); 
		return p;
    }
#else
    Player*
    Player::create (MPX::Library & obj_library, MPX::Covers & obj_amazn)
    {
		const std::string path (build_filename(DATA_DIR, build_filename("glade","mpx.glade")));
		Player * p = new Player(Gnome::Glade::Xml::create (path), obj_library, obj_amazn); 
		return p;
    }
#endif // HAVE_HAL

    Player::~Player()
    {
        Gtk::Window::get_position( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-x")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-y")));
        Gtk::Window::get_size( Mcs::Key::adaptor<int>(mcs->key("mpx", "window-w")), Mcs::Key::adaptor<int>(mcs->key("mpx", "window-h")));
		DBusObjects.mpx->shutdown_complete(DBusObjects.mpx); 
		g_object_unref(G_OBJECT(DBusObjects.mpx));
		delete m_PluginManager;
    }

	void
	Player::on_cover_clicked ()
	{
        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
		g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_INFOAREA_CLICK], 0);
        check_py_error();
        pyg_gil_state_release(state);
	}

	void
	Player::on_source_caps (PlaybackSource::Caps caps, ItemKey const& source_id)
	{
	  Mutex::Lock L (m_SourceCFLock);

	  m_source_c[source_id] = caps;

	  if( m_Sidebar->getVisibleId() == source_id )
	  {
		m_actions->get_action (ACTION_PLAY)->set_sensitive (caps & PlaybackSource::C_CAN_PLAY);
	  }

	  if( m_ActiveSource && source_id == m_ActiveSource.get() )
	  {
    		m_actions->get_action (ACTION_PREV)->set_sensitive (caps & PlaybackSource::C_CAN_GO_PREV);
	    	m_actions->get_action (ACTION_NEXT)->set_sensitive (caps & PlaybackSource::C_CAN_GO_NEXT);
	  }
      else
      {
    		m_actions->get_action (ACTION_PREV)->set_sensitive (false);
	    	m_actions->get_action (ACTION_NEXT)->set_sensitive (false);
      }
	}

	void
	Player::on_source_flags (PlaybackSource::Flags flags, ItemKey const& source_id)
	{
	  Mutex::Lock L (m_SourceCFLock);
	  m_source_f[source_id] = flags;
	}

	void
	Player::on_source_track_metadata (Metadata const& metadata, ItemKey const& source_id)
	{
        Glib::Mutex::Lock L (m_MetadataLock);

        m_Metadata = metadata;

        if( !m_Metadata.get().Image && m_Metadata.get()[ATTRIBUTE_MB_ALBUM_ID]) 
        {
            m_Covers.fetch(
                get<std::string>(m_Metadata.get()[ATTRIBUTE_MB_ALBUM_ID].get()),
                m_Metadata.get().Image
            );
        }

        if(!m_Metadata.get()[ATTRIBUTE_LOCATION])
        {
            m_Metadata.get()[ATTRIBUTE_LOCATION] = m_Play->property_stream().get_value();
        }

        metadata_reparse ();
	}

	void
	Player::on_source_play_request (ItemKey const& source_id)
	{
	  if( m_ActiveSource ) 
	  {
        stop ();
      }

	  RefPtr<ToggleAction>::cast_static (m_actions->get_action(ACTION_PAUSE))->set_active(false);

	  PlaybackSource* source = m_Sources[source_id];
      m_PreparingSource = source_id;

	  if( m_source_f[source_id] & PlaybackSource::F_ASYNC)
	  {
			m_actions->get_action( ACTION_STOP )->set_sensitive (true);
			source->play_async ();
	  }
	  else
	  {
			  if( source->play () )
			  {
                m_PlayDirection = PD_PLAY;
			    switch_stream (source->get_uri(), source->get_type());
			  }
			  else
			  {
                m_Play->request_status (PLAYSTATUS_STOPPED);
			  }
	  }
	}

	void
	Player::on_source_segment (ItemKey const& source_id)
	{
	  g_return_if_fail( m_ActiveSource == source_id );
	  m_Sources[source_id]->segment ();
	}

	void
	Player::on_source_stop (ItemKey const& source_id)
	{
	  g_return_if_fail( m_ActiveSource == source_id );
      stop ();
	}

	//////////////////////////////// Internal Playback Control

    void
    Player::switch_stream(std::string const& uri, std::string const& type)
    {
        m_Play->switch_stream( uri, type );
    }

	void
	Player::track_played ()
	{
        Glib::Mutex::Lock L (m_MetadataLock);

        if(
            m_Metadata
                && 
            m_Metadata.get()[ATTRIBUTE_MPX_TRACK_ID]
                && 
            ((m_TrackPlayedSeconds >= 240) || (m_TrackPlayedSeconds >= m_TrackDuration/2))
        )
        {
            m_Library.trackPlayed(
              get<gint64>(m_Metadata.get()[ATTRIBUTE_MPX_TRACK_ID].get()),
              time(NULL)
            );

            PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
            g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_TRACK_PLAYED], 0);
            check_py_error();
            pyg_gil_state_release(state);
        }

        m_TrackPlayedSeconds = 0.;
	}

	void
	Player::play_async_cb (ItemKey const& source_id)
	{
	  PlaybackSource* source = m_Sources[source_id];

      m_PreparingSource = source_id;
      m_PlayDirection = PD_PLAY;

	  switch_stream(source->get_uri(), source->get_type());
	}

	void
	Player::on_stream_switched ()
	{
      g_return_if_fail(m_PreparingSource);

      m_ActiveSource = m_PreparingSource.get();
	  PlaybackSource* source = m_Sources[m_PreparingSource.get()];
      m_Sidebar->setActiveId(m_ActiveSource.get());

	  source->send_metadata ();

      switch( m_PlayDirection )
      {
        case PD_NONE:
            g_return_if_fail(0);
            break;

        case PD_PREV:
            source->prev_post();
            break;
       
        case PD_NEXT: 
            source->next_post();

        case PD_PLAY:
            source->play_post();
            break;
      };

	  source->send_caps ();

	  Mutex::Lock L (m_SourceCFLock);

      PlaybackSource::Caps caps = m_source_c[m_ActiveSource.get()];
      m_Seek->set_sensitive( caps & PlaybackSource::C_CAN_SEEK );
	  m_actions->get_action( ACTION_PAUSE )->set_sensitive( caps & PlaybackSource::C_CAN_PAUSE );

      PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
      g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_NEW_TRACK], 0);
      check_py_error();
      pyg_gil_state_release(state);
	}

	void
	Player::play ()
	{
	  ItemKey const& source_id = m_Sidebar->getVisibleId(); 

      if( m_Sources.count(source_id) ) 
      {
          PlaybackSource::Caps c = m_source_c[source_id];

          if( c & PlaybackSource::C_CAN_PLAY )
          {
            if( m_ActiveSource && (m_Play->property_status().get_value() != PLAYSTATUS_STOPPED))
            {
                  track_played ();
                  m_Sources[m_ActiveSource.get()]->stop ();
                  if( m_ActiveSource != source_id)
                  {
                        m_Play->request_status (PLAYSTATUS_STOPPED);
                  }
            }

	        RefPtr<ToggleAction>::cast_static (m_actions->get_action(ACTION_PAUSE))->set_active(false);

            PlaybackSource* source = m_Sources[source_id];
            m_PreparingSource = source_id;
            
            if( m_source_f[source_id] & PlaybackSource::F_ASYNC)
            {
                    source->play_async ();
                    m_actions->get_action( ACTION_STOP )->set_sensitive (true);
                    return;
            }
            else
            {
                    if( source->play() )
                    {
                          m_PlayDirection = PD_PLAY;
                          switch_stream (source->get_uri(), source->get_type());
                          return;
                    }
            }
          }

          stop ();
      }
	}

	void
	Player::pause ()
	{
        g_return_if_fail(m_ActiveSource);

        PlaybackSource::Caps c = m_source_c[m_ActiveSource.get()];

        if(c & PlaybackSource::C_CAN_PAUSE )
        {
            if( m_Play->property_status() == PLAYSTATUS_PAUSED)
            {
                m_Play->request_status (PLAYSTATUS_PLAYING);
            }
            else
            {
              m_Play->request_status (PLAYSTATUS_PAUSED);
            }
        }
	}

	void
	Player::prev_async_cb (ItemKey const& source_id)
	{
	  PlaybackSource* source = m_Sources[source_id];
	  PlaybackSource::Flags f = m_source_f[source_id];

	  if( (f & PlaybackSource::F_PHONY_PREV) == 0 )
	  {
            m_PlayDirection = PD_PREV;
			switch_stream (source->get_uri(), source->get_type());
	  }
	}

	void
	Player::prev ()
	{
	  ItemKey const& source_id = m_ActiveSource.get(); 

	  PlaybackSource*           source = m_Sources[source_id];
	  PlaybackSource::Flags          f = m_source_f[source_id];
	  PlaybackSource::Caps           c = m_source_c[source_id];

	  if( c & PlaybackSource::C_CAN_GO_PREV )
	  {
			track_played();

	        RefPtr<ToggleAction>::cast_static (m_actions->get_action(ACTION_PAUSE))->set_active(false);

			if( f & PlaybackSource::F_ASYNC )
			{
					m_actions->get_action (ACTION_PREV)->set_sensitive( false );
					source->go_prev_async ();
					return;
			}
			else
			if( source->go_prev () )
			{
					prev_async_cb (source_id);
					return;
			}
	  }
	
	  stop ();
	}

	void
	Player::next_async_cb (ItemKey const& source_id)
	{
	  PlaybackSource* source = m_Sources[source_id];
	  PlaybackSource::Flags f = m_source_f[source_id];

	  if( (f & PlaybackSource::F_PHONY_NEXT) == 0 )
	  {
            m_PlayDirection = PD_NEXT;
			switch_stream (source->get_uri(), source->get_type());
	  }
	}

	void
	Player::on_play_eos ()
	{
	  ItemKey const& source_id = m_ActiveSource.get(); 

	  PlaybackSource           *source = m_Sources[source_id];
	  PlaybackSource::Flags     f      = m_source_f[source_id];
	  PlaybackSource::Caps      c      = m_source_c[source_id];

	  if( c & PlaybackSource::C_CAN_GO_NEXT )
	  {
			track_played();
			if( f & PlaybackSource::F_ASYNC )
			{
				m_actions->get_action (ACTION_NEXT)->set_sensitive( false );
				source->go_next_async ();
				return;
			}
			else
			if( source->go_next () )
			{
				next_async_cb (m_ActiveSource.get());
				return;
			}
	  }

	  stop ();
	}

	void
	Player::next ()
	{
      g_return_if_fail(m_ActiveSource);

      m_Sources[m_ActiveSource.get()]->skipped();
	  on_play_eos ();
	}

	void
	Player::stop ()
	{
      g_return_if_fail(m_ActiveSource);

      track_played();
      m_Sources[m_ActiveSource.get()]->stop ();
      m_Sources[m_ActiveSource.get()]->send_caps ();
	  m_Play->request_status( PLAYSTATUS_STOPPED );
	}

	void
	Player::on_source_changed (ItemKey const& source_id)
	{
	    Mutex::Lock L (m_SourceCFLock);

        if(m_SourceUI)
        {
            m_ui_manager->remove_ui(m_SourceUI);
        }

        PlaybackSource::Caps caps = m_source_c[source_id];

        if( m_Sources.count(source_id) ) 
        {
            m_SourceUI = m_Sources[source_id]->add_menu();

            if( (m_Play->property_status() == PLAYSTATUS_PLAYING)
                && ( m_ActiveSource && m_ActiveSource.get() == source_id ))
            {
                PlaybackSource::Caps caps = m_source_c[source_id];
                m_actions->get_action (ACTION_PREV)->set_sensitive( caps & PlaybackSource::C_CAN_GO_PREV );
                m_actions->get_action (ACTION_NEXT)->set_sensitive( caps & PlaybackSource::C_CAN_GO_NEXT );
            }
            else
            {
                m_actions->get_action (ACTION_PREV)->set_sensitive( false ); 
                m_actions->get_action (ACTION_NEXT)->set_sensitive( false); 
            }

            m_actions->get_action (ACTION_PLAY)->set_sensitive( caps & PlaybackSource::C_CAN_PLAY );
        }
	}

	void
	Player::on_play_status_changed ()
	{
	  MPXPlaystatus status = MPXPlaystatus (m_Play->property_status().get_value());

	  switch( status )
	  {
		  case PLAYSTATUS_NONE:
			/* ... */
			break;

		  case PLAYSTATUS_SEEKING:
			/* ... */
			break;

		  case PLAYSTATUS_STOPPED:
		  {
                Glib::Mutex::Lock L (m_MetadataLock);

                if( m_ActiveSource ) 
                {
                    m_Sources[m_ActiveSource.get()]->stop ();
                    m_Sources[m_ActiveSource.get()]->send_caps ();
                }

                RefPtr<ToggleAction>::cast_static (m_actions->get_action(ACTION_PAUSE))->set_active(false);

                g_atomic_int_set(&m_Seeking, 0);
                m_Seek->set_range(0., 1.);
                m_Seek->set_value(0.);
                m_Seek->set_sensitive(false);

                m_actions->get_action (ACTION_STOP)->set_sensitive( false );
                m_actions->get_action (ACTION_NEXT)->set_sensitive( false );
                m_actions->get_action (ACTION_PREV)->set_sensitive( false );
                m_actions->get_action (ACTION_PAUSE)->set_sensitive( false );

                m_VideoWidget->property_playing() = false; 
                m_VideoWidget->queue_draw();

                m_TimeLabel->set_text("      …      ");

                set_title (_("AudioSource: (Not Playing)"));

                m_InfoArea->reset();
                m_Metadata.reset();	

                m_Sidebar->clearActiveId();

                m_ActiveSource.reset();
                m_PreparingSource.reset();

                break;
		  }

		  case PLAYSTATUS_WAITING:
		  {
                g_atomic_int_set(&m_Seeking, 0);

                m_TimeLabel->set_text("      …      ");

                m_Seek->set_range(0., 1.);
                m_Seek->set_value(0.);
                m_Seek->set_sensitive(false);

                m_VideoWidget->property_playing() = false;
                m_VideoWidget->queue_draw();

                m_Metadata.reset();	
                m_InfoArea->reset();

			    break;
		  }

		  case PLAYSTATUS_PLAYING:
		  {
                m_actions->get_action( ACTION_STOP )->set_sensitive( true );
                break;
		  }

		  case PLAYSTATUS_PAUSED:
		  {
                break;
		  }
	  }

      PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
      g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_STATUS_CHANGED], 0, int(status));
      check_py_error();
      pyg_gil_state_release(state);
	}


	void
	Player::on_controls_play ()
	{
		play ();
	}

	void
	Player::on_controls_pause ()
	{
		pause ();
	}

	void
	Player::on_controls_next ()
	{
		next ();
	}

	void
	Player::on_controls_prev ()
	{
		prev ();
	}	

    bool
    Player::on_key_press_event (GdkEventKey* event)
    {
      if( event->keyval == GDK_Escape )
      {
          iconify ();
          return true;
      }
      return Widget::on_key_press_event (event);
    }

    void
    Player::on_import_folder()
    {
        boost::shared_ptr<DialogImportFolder> d = boost::shared_ptr<DialogImportFolder>(DialogImportFolder::create());
        
        if(d->run() == 0) // Import
        {
            Glib::ustring uri; 
            d->get_folder_infos(uri);
            d->hide();
            StrV v;
            v.push_back(uri);
            m_Library.initScan(v);
        }
    }

    void
    Player::on_import_share()
    {
        boost::shared_ptr<DialogImportShare> d = boost::shared_ptr<DialogImportShare>(DialogImportShare::create());
        
        rerun_import_share_dialog:

            if(d->run() == 0) // Import
            {
                Glib::ustring login, password;
                d->get_share_infos(m_Share, m_ShareName, login, password);
                d->hide ();

                if(m_ShareName.empty())
                {
                    MessageDialog dialog (*this, (_("The Share's name can not be empty")));
                    dialog.run();
                    goto rerun_import_share_dialog;
                }

                m_MountFile = Glib::wrap (g_vfs_get_file_for_uri (g_vfs_get_default(), m_Share.c_str()));
                if(!m_MountFile)
                {
                    MessageDialog dialog (*this, (boost::format (_("An Error occcured getting a handle for the share '%s'\n"
                        "Please veryify the share URI and credentials")) % m_Share.c_str()).str());
                    dialog.run();
                    goto rerun_import_share_dialog;
                }

                m_MountOperation = Gio::MountOperation::create();
                if(!m_MountOperation)
                {
                    MessageDialog dialog (*this, (boost::format (_("An Error occcured trying to mount the share '%s'")) % m_Share.c_str()).str());
                    dialog.run();
                    return; 
                }

				m_MountOperation->set_username(login);
				m_MountOperation->set_password(password);
				m_MountOperation->signal_ask_password().connect(sigc::mem_fun(*this, &Player::ask_password_cb));
				m_MountFile->mount_mountable(m_MountOperation, sigc::mem_fun(*this, &Player::mount_ready_callback));
            }
    }

    void
    Player::ask_password_cb (const Glib::ustring& message,
                             const Glib::ustring& default_user,
                             const Glib::ustring& default_domain,
                             Gio::AskPasswordFlags flags)
    {
      Glib::ustring value;
      if (flags & Gio::ASK_PASSWORD_NEED_USERNAME)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Username:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            m_MountOperation->reply (Gio::MOUNT_OPERATION_ABORTED /*abort*/);
            return;
        }
        p->get_request_infos(value);
        m_MountOperation->set_username (value);
        delete p;
        value.clear();
      }

      if (flags & Gio::ASK_PASSWORD_NEED_DOMAIN)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Domain:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            m_MountOperation->reply (Gio::MOUNT_OPERATION_ABORTED /*abort*/);
            return;
        }
        p->get_request_infos(value);
        m_MountOperation->set_domain (value);
        delete p;
        value.clear();
      }

      if (flags & Gio::ASK_PASSWORD_NEED_PASSWORD)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Password:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            m_MountOperation->reply (Gio::MOUNT_OPERATION_ABORTED /*abort*/);
            return;
        }
        p->get_request_infos(value);
        m_MountOperation->set_password (value);
        delete p;
        value.clear();
      }

      m_MountOperation->reply (Gio::MOUNT_OPERATION_HANDLED);
    }

    void
    Player::mount_ready_callback (Glib::RefPtr<Gio::AsyncResult>& res)
    {
        try
	{
            m_MountFile->mount_mountable_finish(res);
	}
	catch(const Glib::Error& error)
	{
            if(error.code() != G_IO_ERROR_ALREADY_MOUNTED)
            {
                MessageDialog dialog (*this, (boost::format ("An Error occcured while mounting the share: %s") % error.what()).str());
                dialog.run();
                return;
            }
            else
                g_warning("%s: Location '%s' is already mounted", G_STRLOC, m_Share.c_str());

        }

        Util::FileList v (1, m_Share);
        m_Library.initScan(v);
    }

    void
    Player::unmount_ready_callback (Glib::RefPtr<Gio::AsyncResult>& res)
    {
    }

    void
    Player::on_library_scan_start()
    {
        m_Statusbar->pop();        
        m_Statusbar->push(_("Library Scan Started"));
    }

    void
    Player::on_library_scan_run(gint64 x, gint64 y)
    {
        m_Statusbar->pop();
        m_Statusbar->push((boost::format(_("Library Scan: %1% / %2%")) % x % y).str());
    }

    void
    Player::on_library_scan_end(gint64 x, gint64 y, gint64 a, gint64 b, gint64 s)
    {
        m_Statusbar->pop();        
        m_Statusbar->push((boost::format(_("Library Scan: Done (%1% Files, %2% added, %3% up to date, %4% updated, %5% erroneous)")) % s % x % y % a % b).str());
    }

    // MM-Keys stuff (C) Rhythmbox Developers 2007

    /*static*/ void
    Player::media_player_key_pressed (DBusGProxy *proxy,
                                           const gchar *application,
                                           const gchar *key,
                                           gpointer data)
    {
      Player & player = *reinterpret_cast<Player*>(data);

      if( strcmp (application, "MPX"))
        return;

      if( !player.m_ActiveSource )
        return;

      PlaybackSource::Caps c = player.m_source_c[player.m_ActiveSource.get()];

      if (strcmp (key, "Play") == 0) {
        if( player.m_Play->property_status() == PLAYSTATUS_PAUSED)
          player.pause ();
        else
        if( player.m_Play->property_status() != PLAYSTATUS_WAITING)
          player.play ();
      } else if (strcmp (key, "Pause") == 0) {
        if( c & PlaybackSource::C_CAN_PAUSE )
          player.pause ();
      } else if (strcmp (key, "Stop") == 0) {
        player.stop ();
      } else if (strcmp (key, "Previous") == 0) {
        if( c & PlaybackSource::C_CAN_GO_PREV )
          player.prev ();
      } else if (strcmp (key, "Next") == 0) {
        if( c & PlaybackSource::C_CAN_GO_NEXT )
          player.next ();
      }
    }

    /*static*/ bool
    Player::window_focus_cb (GdkEventFocus *event)
    {
      dbus_g_proxy_call (m_mmkeys_dbusproxy,
             "GrabMediaPlayerKeys", NULL,
             G_TYPE_STRING, "MPX",
             G_TYPE_UINT, 0,
             G_TYPE_INVALID, G_TYPE_INVALID);

      return false;
    }

    /* Taken from xbindkeys */
    void
    Player::mmkeys_get_offending_modifiers ()
    {
      Display * dpy = gdk_x11_display_get_xdisplay (gdk_display_get_default());
      int i;
      XModifierKeymap *modmap;
      KeyCode nlock, slock;
      static int mask_table[8] = {
        ShiftMask, LockMask, ControlMask, Mod1Mask,
        Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
      };

      nlock = XKeysymToKeycode (dpy, XK_Num_Lock);
      slock = XKeysymToKeycode (dpy, XK_Scroll_Lock);

      /*
      * Find out the masks for the NumLock and ScrollLock modifiers,
      * so that we can bind the grabs for when they are enabled too.
      */
      modmap = XGetModifierMapping (dpy);

      if (modmap != NULL && modmap->max_keypermod > 0)
      {
        for (i = 0; i < 8 * modmap->max_keypermod; i++)
        {
          if (modmap->modifiermap[i] == nlock && nlock != 0)
            m_numlock_mask = mask_table[i / modmap->max_keypermod];
          else if (modmap->modifiermap[i] == slock && slock != 0)
            m_scrolllock_mask = mask_table[i / modmap->max_keypermod];
        }
      }

      m_capslock_mask = LockMask;

      if (modmap)
        XFreeModifiermap (modmap);
    }

    /*static*/ void
    Player::grab_mmkey (int key_code,
                             GdkWindow *root)
    {
      gdk_error_trap_push ();

      XGrabKey (GDK_DISPLAY (), key_code,
          0,
          GDK_WINDOW_XID (root), True,
          GrabModeAsync, GrabModeAsync);
      XGrabKey (GDK_DISPLAY (), key_code,
          Mod2Mask,
          GDK_WINDOW_XID (root), True,
          GrabModeAsync, GrabModeAsync);
      XGrabKey (GDK_DISPLAY (), key_code,
          Mod5Mask,
          GDK_WINDOW_XID (root), True,
          GrabModeAsync, GrabModeAsync);
      XGrabKey (GDK_DISPLAY (), key_code,
          LockMask,
          GDK_WINDOW_XID (root), True,
          GrabModeAsync, GrabModeAsync);
      XGrabKey (GDK_DISPLAY (), key_code,
          Mod2Mask | Mod5Mask,
          GDK_WINDOW_XID (root), True,
          GrabModeAsync, GrabModeAsync);
      XGrabKey (GDK_DISPLAY (), key_code,
          Mod2Mask | LockMask,
          GDK_WINDOW_XID (root), True,
          GrabModeAsync, GrabModeAsync);
      XGrabKey (GDK_DISPLAY (), key_code,
          Mod5Mask | LockMask,
          GDK_WINDOW_XID (root), True,
          GrabModeAsync, GrabModeAsync);
      XGrabKey (GDK_DISPLAY (), key_code,
          Mod2Mask | Mod5Mask | LockMask,
          GDK_WINDOW_XID (root), True,
          GrabModeAsync, GrabModeAsync);

      gdk_flush ();

      if (gdk_error_trap_pop ())
      {
        g_message(G_STRLOC ": Error grabbing key");
      }
    }

    /*static*/ void
    Player::grab_mmkey (int key_code,
                             int modifier,
                             GdkWindow *root)
    {
      gdk_error_trap_push ();

      modifier &= ~(m_numlock_mask | m_capslock_mask | m_scrolllock_mask);

      XGrabKey (GDK_DISPLAY (), key_code, modifier, GDK_WINDOW_XID (root),
        False, GrabModeAsync, GrabModeAsync);

      if (modifier == AnyModifier)
        return;

      if (m_numlock_mask)
        XGrabKey (GDK_DISPLAY (), key_code, modifier | m_numlock_mask,
          GDK_WINDOW_XID (root),
          False, GrabModeAsync, GrabModeAsync);

      if (m_capslock_mask)
        XGrabKey (GDK_DISPLAY (), key_code, modifier | m_capslock_mask,
          GDK_WINDOW_XID (root),
          False, GrabModeAsync, GrabModeAsync);

      if (m_scrolllock_mask)
        XGrabKey (GDK_DISPLAY (), key_code, modifier | m_scrolllock_mask,
          GDK_WINDOW_XID (root),
          False, GrabModeAsync, GrabModeAsync);

      if (m_numlock_mask && m_capslock_mask)
        XGrabKey (GDK_DISPLAY (), key_code, modifier | m_numlock_mask | m_capslock_mask,
          GDK_WINDOW_XID (root),
          False, GrabModeAsync, GrabModeAsync);

      if (m_numlock_mask && m_scrolllock_mask)
        XGrabKey (GDK_DISPLAY (), key_code, modifier | m_numlock_mask | m_scrolllock_mask,
          GDK_WINDOW_XID (root),
          False, GrabModeAsync, GrabModeAsync);

      if (m_capslock_mask && m_scrolllock_mask)
        XGrabKey (GDK_DISPLAY (), key_code, modifier | m_capslock_mask | m_scrolllock_mask,
          GDK_WINDOW_XID (root),
          False, GrabModeAsync, GrabModeAsync);

      if (m_numlock_mask && m_capslock_mask && m_scrolllock_mask)
        XGrabKey (GDK_DISPLAY (), key_code,
          modifier | m_numlock_mask | m_capslock_mask | m_scrolllock_mask,
          GDK_WINDOW_XID (root), False, GrabModeAsync,
          GrabModeAsync);

      gdk_flush ();

      if (gdk_error_trap_pop ())
      {
        g_message(G_STRLOC ": Error grabbing key");
      }
    }

    /*static*/ void
    Player::ungrab_mmkeys (GdkWindow *root)
    {
      gdk_error_trap_push ();

      XUngrabKey (GDK_DISPLAY (), AnyKey, AnyModifier, GDK_WINDOW_XID (root));

      gdk_flush ();

      if (gdk_error_trap_pop ())
      {
        g_message(G_STRLOC ": Error grabbing key");
      }
    }


    /*static*/ GdkFilterReturn
    Player::filter_mmkeys (GdkXEvent *xevent,
                                GdkEvent *event,
                                gpointer data)
    {
      Player & player = *reinterpret_cast<Player*>(data);

      if( !player.m_ActiveSource ) 
        return GDK_FILTER_CONTINUE;

      PlaybackSource::Caps c = player.m_source_c[player.m_ActiveSource.get()];

      XEvent * xev = (XEvent *) xevent;

      if (xev->type != KeyPress)
      {
        return GDK_FILTER_CONTINUE;
      }

      XKeyEvent * key = (XKeyEvent *) xevent;

      guint keycodes[] = {0, 0, 0, 0, 0};
      int sys = mcs->key_get<int>("hotkeys","system");

      if( sys == 0)
      {
        keycodes[0] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPlay);
        keycodes[1] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPause);
        keycodes[2] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev);
        keycodes[3] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext);
        keycodes[4] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop);
      }
      else
      {
        keycodes[0] = mcs->key_get<int>("hotkeys","key-1");
        keycodes[1] = mcs->key_get<int>("hotkeys","key-2");
        keycodes[2] = mcs->key_get<int>("hotkeys","key-3");
        keycodes[3] = mcs->key_get<int>("hotkeys","key-4");
        keycodes[4] = mcs->key_get<int>("hotkeys","key-5");
      }

      if (keycodes[0] == key->keycode) {
          (dynamic_cast<Gtk::Button*>(player.m_ref_xml->get_widget("controls-play")))->clicked();
          //player.play ();
        return GDK_FILTER_REMOVE;
      } else if (keycodes[1] == key->keycode) {
        if( c & PlaybackSource::C_CAN_PAUSE )
          (dynamic_cast<Gtk::ToggleButton*>(player.m_ref_xml->get_widget("controls-pause")))->set_active(!(dynamic_cast<Gtk::ToggleButton*>(player.m_ref_xml->get_widget("controls-pause")))->get_active());
          //player.pause ();
        return GDK_FILTER_REMOVE;
      } else if (keycodes[2] == key->keycode) {
        if( c & PlaybackSource::C_CAN_GO_PREV )
          (dynamic_cast<Gtk::Button*>(player.m_ref_xml->get_widget("controls-previous")))->clicked();
          //player.prev ();
        return GDK_FILTER_REMOVE;
      } else if (keycodes[3] == key->keycode) {
        if( c & PlaybackSource::C_CAN_GO_NEXT )
          (dynamic_cast<Gtk::Button*>(player.m_ref_xml->get_widget("controls-next")))->clicked();
          //player.next ();
        return GDK_FILTER_REMOVE;
      } else if (keycodes[4] == key->keycode) {
          (dynamic_cast<Gtk::Button*>(player.m_ref_xml->get_widget("controls-stop")))->clicked();
          //player.stop ();
        return GDK_FILTER_REMOVE;
      } else {
        return GDK_FILTER_CONTINUE;
      }
    }

    /*static*/ void
    Player::mmkeys_grab (bool grab)
    {
      gint keycodes[] = {0, 0, 0, 0, 0};
      keycodes[0] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPlay);
      keycodes[1] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop);
      keycodes[2] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev);
      keycodes[3] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext);
      keycodes[4] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPause);

      GdkDisplay *display;
      GdkScreen *screen;
      GdkWindow *root;

      display = gdk_display_get_default ();
      int sys = mcs->key_get<int>("hotkeys","system");

      for (int i = 0; i < gdk_display_get_n_screens (display); i++) {
        screen = gdk_display_get_screen (display, i);

        if (screen != NULL) {
          root = gdk_screen_get_root_window (screen);
          if(!grab)
          {
            ungrab_mmkeys (root);
            continue;
          }

          for (guint j = 1; j < 6 ; ++j)
          {
            if( sys == 2 )
            {
              int key = mcs->key_get<int>("hotkeys", (boost::format ("key-%d") % j).str());
              int mask = mcs->key_get<int>("hotkeys", (boost::format ("key-%d-mask") % j).str());

              if (key)
              {
                grab_mmkey (key, mask, root);
              }
            }
            else
            if( sys == 0 )
            {
              grab_mmkey (keycodes[j-1], root);
            }
          }

          if (grab)
            gdk_window_add_filter (root, filter_mmkeys, this);
          else
            gdk_window_remove_filter (root, filter_mmkeys, this);
        }
      }
    }

    /*static*/ void
    Player::mmkeys_activate ()
    {
      if( mm_active )
        return;

      DBusGConnection *bus;

      g_message(G_STRLOC ": Activating media player keys");

      if (m_mmkeys_grab_type == SETTINGS_DAEMON)
      {
        bus = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
        if(bus != NULL)
        {
          GError *error = NULL;

          m_mmkeys_dbusproxy = dbus_g_proxy_new_for_name (bus,
              "org.gnome.SettingsDaemon",
              "/org/gnome/SettingsDaemon/MediaKeys",
              "org.gnome.SettingsDaemon.MediaKeys");

          if(!m_mmkeys_dbusproxy)
          {
              m_mmkeys_dbusproxy = dbus_g_proxy_new_for_name (bus,
                  "org.gnome.SettingsDaemon",
                  "/org/gnome/SettingsDaemon",
                  "org.gnome.SettingsDaemon");
          }

          if (m_mmkeys_dbusproxy)
          {
            dbus_g_proxy_call (m_mmkeys_dbusproxy,
                   "GrabMediaPlayerKeys", &error,
                   G_TYPE_STRING, "MPX",
                   G_TYPE_UINT, 0,
                   G_TYPE_INVALID,
                   G_TYPE_INVALID);

            if (error == NULL)
            {
              g_message(G_STRLOC ": created dbus proxy for org.gnome.SettingsDaemon; grabbing keys");

              dbus_g_object_register_marshaller (mpx_dbus_marshal_VOID__STRING_STRING,
                  G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

              dbus_g_proxy_add_signal (m_mmkeys_dbusproxy,
                     "MediaPlayerKeyPressed",
                     G_TYPE_STRING,G_TYPE_STRING,G_TYPE_INVALID);

              dbus_g_proxy_connect_signal (m_mmkeys_dbusproxy,
                         "MediaPlayerKeyPressed",
                         G_CALLBACK (media_player_key_pressed),
                         this, NULL);

              mWindowFocusConn = signal_focus_in_event().connect( sigc::mem_fun( *this, &Player::window_focus_cb ) );
            }
            else if (error->domain == DBUS_GERROR &&
                       (error->code != DBUS_GERROR_NAME_HAS_NO_OWNER ||
                        error->code != DBUS_GERROR_SERVICE_UNKNOWN))
            {
              /* settings daemon dbus service doesn't exist.
               * just silently fail.
               */
              g_message(G_STRLOC ": org.gnome.SettingsDaemon dbus service not found: %s", error->message);
              g_error_free (error);
            }
            else
            {
              g_warning (G_STRLOC ": Unable to grab media player keys: %s", error->message);
              g_error_free (error);
            }
          }
        }
        else
        {
          g_message(G_STRLOC ": couldn't get dbus session bus");
        }
      }
      else
      if (m_mmkeys_grab_type == X_KEY_GRAB)
      {
        g_message(G_STRLOC ": attempting old-style key grabs");
        mmkeys_grab (true);
      }

      mm_active = true;
    }

    /*static*/ void
    Player::mmkeys_deactivate ()
    {
      if( !mm_active )
        return;

      if (m_mmkeys_dbusproxy)
      {
        GError *error = NULL;

        if (m_mmkeys_grab_type == SETTINGS_DAEMON)
        {
          dbus_g_proxy_call (m_mmkeys_dbusproxy,
                 "ReleaseMediaPlayerKeys", &error,
                 G_TYPE_STRING, "MPX",
                 G_TYPE_INVALID, G_TYPE_INVALID);
          if (error != NULL)
          {
            g_warning (G_STRLOC ": Could not release media player keys: %s", error->message);
            g_error_free (error);
          }
          mWindowFocusConn.disconnect ();
          m_mmkeys_grab_type = NONE;
        }

        g_object_unref (m_mmkeys_dbusproxy);
        m_mmkeys_dbusproxy = 0;
      }

      if (m_mmkeys_grab_type == X_KEY_GRAB)
      {
        g_message(G_STRLOC ": undoing old-style key grabs");
        mmkeys_grab (false);
        m_mmkeys_grab_type = NONE;
      }

      mm_active = false;
    }

    void
    Player::on_mm_edit_begin ()
    {
      mmkeys_deactivate ();
    }

    void
    Player::on_mm_edit_done ()
    {
      if( mcs->key_get<bool>("hotkeys","enable") )
      {
        int sys = mcs->key_get<int>("hotkeys","system");
        if( (sys == 0) || (sys == 2))
          m_mmkeys_grab_type = X_KEY_GRAB;
        else
          m_mmkeys_grab_type = SETTINGS_DAEMON;
        mmkeys_activate ();
      }
    }

    ::Window
    Player::on_gst_set_window_id ()
    {
      m_VideoWidget->property_playing() = true; //FIXME: This does not really belong here.
      m_VideoWidget->queue_draw();
      return m_VideoWidget->get_video_xid(); 
    }

    void
    Player::on_gst_set_window_geom (int width, int height, GValue const* par)
    {
      m_VideoWidget->property_geometry() = Geometry( width, height );
      m_VideoWidget->set_par( par );
    }

	void
	Player::metadata_reparse ()
	{
        m_InfoArea->set_metadata(m_Metadata.get());

        if(m_Metadata.get()[ATTRIBUTE_TITLE] && m_Metadata.get()[ATTRIBUTE_ARTIST])
        {
            set_title(
                (boost::format ("AudioSource: %1% - %2%") 
                        % get<std::string>(m_Metadata.get()[ATTRIBUTE_ARTIST].get())
                        % get<std::string>(m_Metadata.get()[ATTRIBUTE_TITLE].get())
                ).str()
            );
        }
        else
        {
            set_title(_("AudioSource: [Untitled Track]"));
        }

        signal_idle().connect(
            sigc::mem_fun(
                *this,
                &Player::metadata_updated
        ));
	}

    bool
    Player::metadata_updated ()
    {
        PyGILState_STATE state = (PyGILState_STATE)(pyg_gil_state_ensure ());
        g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_METADATA_PREPARE], 0);
        check_py_error();
        pyg_gil_state_release(state);

        state = (PyGILState_STATE)(pyg_gil_state_ensure ());
        g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_METADATA_UPDATED], 0);
        check_py_error();
        pyg_gil_state_release(state);

        return false;
    }

    void
    Player::deactivate_plugin(gint64 id)
    {
        m_PluginManager->deactivate(id);
    }

    void
    Player::info_set (const std::string& info)
    {
        m_InfoArea->set_info(info);
    }

    void
    Player::info_clear ()
    {
        m_InfoArea->clear_info();
    }

    void
    Player::check_py_error ()
    {
#if 0
        GError * gerr = NULL;

        if(pyg_error_check(&gerr))
        {
                g_message("Error occurred");
                Error err (_("Python Plugins"), "", gerr->message);
                g_error_free(gerr);
                m_ErrorManager->new_error(err);
        }
#endif
    }
}
