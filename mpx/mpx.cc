//  MPX
//  Copyright (C) 2007 MPX Project 
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
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>

#include "import-share.hh"
#include "import-folder.hh"
#include "dialog-filebrowser.hh"

#include "mpx.hh"
#include "mpxpy.hh"
#include "mpx-sources.hh"
#include "mpx/uri.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-string.hh"
#include "mpx/util-ui.hh"

#include "request-value.hh"
#include "stock.hh"

using namespace Glib;
using namespace Gtk;
using namespace std;
using namespace Gnome::Glade;
using namespace MPX::Util;
using boost::get;
using boost::algorithm::is_any_of;
using namespace boost::python;

// TODO: Saner API for sources
#define SOURCE_NONE (-1)

namespace
{
  char const * MenubarMain =
  "<ui>"
  ""
  "<menubar name='MenubarMain'>"
  "   <menu action='MenuMusic'>"
  "         <menuitem action='action-play-files'/>"
  "         <separator name='sep00'/>"
  "         <menuitem action='action-import-folder'/>"
  "         <menuitem action='action-import-share'/>"
  "			<menuitem action='action-vacuum-lib'/>"
  "         <separator name='sep01'/>"
  "         <menuitem action='action-quit'/>"
  "   </menu>"
  "   <menu action='MenuEdit'>"
  "         <menuitem action='action-plugins'/>"
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

  void
  parse_metadata (MPX::Metadata & metadata,
                  ustring       & artist,
                  ustring       & album,
                  ustring       & title)
  {
    using namespace MPX;

    static char const *
      text_b_f ("<b>%s</b>");

    static char const *
      text_b_f2 ("<b>%s</b> (%s)");

    static char const *
      text_big_f ("<span size='14000'><b>%s</b></span>");

    static char const *
      text_album_artist_f ("%s (%s)");

    static char const *
      text_album_f ("%s");

    static char const *
      text_artist_f ("(%s)");

    if( (metadata[ATTRIBUTE_MB_ALBUM_ARTIST_ID] != metadata[ATTRIBUTE_MB_ARTIST_ID]) && metadata[ATTRIBUTE_ALBUM_ARTIST] )
    {
        if( metadata[ATTRIBUTE_ALBUM] )
        {
          album = gprintf (text_album_artist_f,
                               Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM].get())).c_str(),
                               Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM_ARTIST].get())).c_str());
        }
        else
        {
          album = gprintf (text_artist_f,
                               Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM_ARTIST].get())).c_str());
        }
    }
    else
    {
        if( metadata[ATTRIBUTE_ALBUM] )
        {
          album = gprintf (text_album_f,
                               Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM].get())).c_str());
        }
    }

    if ((metadata[ATTRIBUTE_ARTIST_SORTNAME] && metadata[ATTRIBUTE_ARTIST]) &&
        (metadata[ATTRIBUTE_ARTIST_SORTNAME] != metadata[ATTRIBUTE_ARTIST])) /* let's display the artist if it's not identical to the sortname */
    {
      std::string a = get<std::string>(metadata[ATTRIBUTE_ARTIST_SORTNAME].get());
      artist = gprintf (text_b_f2,
                            Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST_SORTNAME].get())).c_str(),
                            Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST].get())).c_str());
    }
    else
    if( metadata[ATTRIBUTE_ARTIST] )
    {
      artist = gprintf (text_b_f, Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST].get())).c_str());
    }

    if( metadata[ATTRIBUTE_TITLE] )
    {
      title = gprintf (text_big_f, Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_TITLE].get())).c_str());
    }
  }
}

namespace
{
  inline bool
  is_module (std::string const& basename)
  {
    return MPX::Util::str_has_suffix_nocase
      (basename.c_str (), G_MODULE_SUFFIX);
  } 
}

namespace MPX
{
  enum LayoutID
  {
    L_TITLE,
    L_ARTIST,
    L_ALBUM,
    N_LAYOUTS
  };

  struct LayoutData
  {
    double alpha;
    double target;
    int    x;
    int    y;
  };

  LayoutData const layout_info[] = {
    {-0.0, 1.0, 86, 55},
    {-0.4, 1.0, 86,  8},
    {-0.4, 0.8, 86, 26}
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
  double /*const*/ cover_anim_initial_velocity = 223.3; 
  double /*const*/ cover_anim_gravity          = 359.1; 
  double const cover_anim_wall             = cover_anim_area_x1;
  double /*const*/ cover_anim_wall_elasticity  = 0.074;
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

  class InfoArea
    : public WidgetLoader<EventBox>
  {
    private:

	  MPX::Play & m_Play;

      Spectrum m_spectrum_data;

      struct Text
      {
        Text (Gtk::Widget &  widget,
              ustring const& text,
              LayoutID       id)
        : alpha   (layout_info[id].alpha)
        , target  (layout_info[id].target)
        , x       (layout_info[id].x)
        , y       (layout_info[id].y)
        {
          layout = widget.create_pango_layout ("");
          layout->set_markup (text);
        }

        ~Text () {}

        RefPtr<Pango::Layout> layout;
        double                alpha;
        double                target;
        int                   x, y;
        sigc::connection      conn;
      };

      typedef boost::shared_ptr<Text>   TextP;
      typedef std::map<LayoutID, TextP> Layouts;

      Layouts m_layouts;
      ustring m_text[N_LAYOUTS];

      Cairo::RefPtr<Cairo::ImageSurface>  m_cover_surface;
      Glib::RefPtr<Gdk::Pixbuf>           m_source_icon;
      double                              m_cover_pos;
      double                              m_cover_velocity;
      double                              m_cover_accel;
      double                              m_cover_alpha;
	  bool								  m_pressed;
      sigc::connection                    m_cover_anim_conn;
      sigc::connection                    m_decay_conn;

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

      SignalUris m_SignalUrisDropped;
      SignalCoverClicked m_SignalCoverClicked;

    public:

      InfoArea (MPX::Play & play,
			    Glib::RefPtr<Gnome::Glade::Xml> const& xml)
	  : WidgetLoader<Gtk::EventBox>(xml, "infoarea")
	  , m_Play			(play)
      , m_spectrum_data (SPECT_BANDS, 0)
      , m_source_icon   (Glib::RefPtr<Gdk::Pixbuf> (0))
      , m_cover_alpha   (1.0)
	  , m_pressed		(false)
      {
        add_events (Gdk::BUTTON_PRESS_MASK);

        Gdk::Color const color ("#000000");
        modify_bg (Gtk::STATE_NORMAL, color);
        modify_base (Gtk::STATE_NORMAL, color);

        m_Play.signal_spectrum().connect( sigc::mem_fun( *this, &InfoArea::play_update_spectrum));
		m_Play.property_status().signal_changed().connect( sigc::mem_fun( *this, &InfoArea::play_status_changed));

        enable_drag_dest ();
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
        int status = m_Play.property_status ().get_value ();
        if( status == PLAYSTATUS_PAUSED )
        {
          m_decay_conn = Glib::signal_timeout ().connect (sigc::mem_fun(*this, &InfoArea::decay_spectrum), 50);
        }
        else
        {
          m_decay_conn.disconnect ();
        }
      }

      void
      set_source (Glib::RefPtr<Gdk::Pixbuf> const& source_icon)
      {
        m_source_icon = source_icon;
        queue_draw ();
      }

	public:

      void
      reset ()
      {
        std::fill (m_spectrum_data.begin (), m_spectrum_data.end (), 0.);

        remove_layout_if_exists (L_ARTIST);
        remove_layout_if_exists (L_ALBUM);
        remove_layout_if_exists (L_TITLE);

        m_cover_anim_conn.disconnect ();
        m_cover_surface = Cairo::RefPtr<Cairo::ImageSurface> (0);

        m_text[0].clear ();
        m_text[1].clear ();
        m_text[2].clear ();

        set_source (Glib::RefPtr<Gdk::Pixbuf> (0));
        queue_draw ();
      }

      void
      set_text (LayoutID       id,
                ustring const& text)
      {
        if( text != m_text[id] )
        {
          m_text[id] = text;

          TextP p (new Text (*this, text, id));

          remove_layout_if_exists (id);
          insert_layout_and_connect (id, p);
        }
      }

      void
      set_paused (bool paused)
      {
        m_cover_alpha = (paused ? 0.5 : 1.0);
		queue_draw ();
      }

      void
      set_image (RefPtr<Gdk::Pixbuf> const& pixbuf)
      {
        set_image (Util::cairo_image_surface_from_pixbuf (pixbuf));
      }

      void
      set_image (Cairo::RefPtr<Cairo::ImageSurface> const& surface)
      {
        m_cover_surface  = surface;

        m_cover_pos      = cover_anim_initial_pos;
        m_cover_velocity = cover_anim_initial_velocity;
        m_cover_accel    = cover_anim_gravity;

        m_cover_anim_conn.disconnect ();
        m_cover_anim_conn = signal_timeout ().connect (sigc::mem_fun (this, &InfoArea::slide_in_cover), cover_anim_interval);
      }

    private:

      void
      insert_layout_and_connect (LayoutID id,
                                 TextP &  p)
      {
        m_layouts[id] = p;
        p->conn = signal_timeout().connect (sigc::bind (sigc::mem_fun (this, &InfoArea::fade_in), id), 20);
      }

      void
      remove_layout_if_exists (LayoutID id)
      {
        Layouts::iterator i = m_layouts.find (id);
        if( i != m_layouts.end() )
        {
          TextP & p = i->second;

          p->conn.disconnect ();
          m_layouts.erase (i);
        }
      }

      bool
      fade_in (LayoutID id)
      {
        TextP & p (m_layouts.find (id)->second);
        p->alpha += 0.1;

        bool r = p->alpha < p->target;

        queue_draw ();

        return r;
      }

      bool
      slide_in_cover ()
      {
        m_cover_pos      += m_cover_velocity * cover_anim_dt;
        m_cover_velocity += m_cover_accel    * cover_anim_dt;
        m_cover_accel     = cover_anim_gravity;

        bool r = true;

        if (m_cover_pos + m_cover_surface->get_width () >= cover_anim_wall)
        {
            m_cover_pos       = cover_anim_wall - m_cover_surface->get_width ();
            m_cover_velocity *= -cover_anim_wall_elasticity;

            // FIXME: Need a better test. This runs into stability problems when
            // dt or acceleration is too high

            double next_velocity = m_cover_velocity + m_cover_accel * cover_anim_dt;

            if (next_velocity >= 0.0)
            {
                m_cover_pos      = cover_anim_wall - m_cover_surface->get_width ();
                m_cover_velocity = 0.0;
                m_cover_accel    = 0.0;

                r = false;
            }
        }

        queue_draw ();

        return r;
      }

      void
      draw_background (Cairo::RefPtr<Cairo::Context> const& cr)
      {
        Gtk::Allocation allocation = get_allocation ();

        Gdk::Cairo::set_source_color (cr, get_style()->get_bg (Gtk::STATE_SELECTED));
        cr->rectangle (0, 0, allocation.get_width (), allocation.get_height ());
        cr->stroke ();
      }

      void
      draw_cover (Cairo::RefPtr<Cairo::Context> const& cr)
      {
        if( m_cover_surface && (m_cover_pos >= cover_anim_area_x0 - m_cover_surface->get_width ()) )
        {
          cr->save ();

          cr->rectangle (cover_anim_area_x0+(m_pressed?1:0),
						 cover_anim_area_y0+(m_pressed?1:0), cover_anim_area_width+(m_pressed?1:0),
															 cover_anim_area_height+(m_pressed?1:0));
          cr->clip ();

          double y = (cover_anim_area_y0 + cover_anim_area_y1 - m_cover_surface->get_height ()) / 2;

          draw_cairo_image (cr, m_cover_surface, m_cover_pos + (m_pressed?1:0), y + (m_pressed?1:0), m_cover_alpha);

          cr->restore ();
        }
      }

      void
      draw_text (Cairo::RefPtr<Cairo::Context> const& cr)
      {
        Gtk::Allocation allocation = get_allocation ();

        for (Layouts::const_iterator i = m_layouts.begin(); i != m_layouts.end(); ++i)
        {
          TextP const& p (i->second);
          if( p->alpha < 0 )
            continue;

          cr->set_source_rgba (1.0, 1.0, 1.0, p->alpha);
          cr->set_operator (Cairo::OPERATOR_ATOP);
          cr->move_to (p->x, p->y);

          p->layout->set_single_paragraph_mode (true);
          p->layout->set_ellipsize (Pango::ELLIPSIZE_END);
          p->layout->set_width ((allocation.get_width() - p->x - (8*SPECT_BANDS+12)) * PANGO_SCALE);
          p->layout->set_wrap (Pango::WRAP_CHAR);

          pango_cairo_show_layout (cr->cobj(), p->layout->gobj());
        }
      }

      void
      draw_spectrum (Cairo::RefPtr<Cairo::Context> const& cr)
      {
        Gtk::Allocation allocation = get_allocation ();

        for (int n = 0; n < SPECT_BANDS; ++n)
        {
          int x = 0, y = 0, w = 0, h = 0;

          // Bar
          x = allocation.get_width () - (8*SPECT_BANDS+8) + (n*8);
          y = 11 + (64 - (m_spectrum_data[n]+64));

          w = 6;
          h = (m_spectrum_data[n]+64);

          cr->set_source_rgba (double(colors[n/2].red)/255., double(colors[n/2].green)/255., double(colors[n/2].blue)/255., 0.8);
          cr->rectangle (x,y,w,h);
          cr->fill ();
        }
      }

      void
      draw_source_icon (Cairo::RefPtr<Cairo::Context> const& cr)
      {
        if( m_source_icon )
        {
          Gtk::Allocation allocation = get_allocation ();

          cr->set_operator (Cairo::OPERATOR_ATOP);
          Gdk::Cairo::set_source_pixbuf (cr, m_source_icon, allocation.get_width () - 28, 12);
          cr->rectangle (allocation.get_width () - 28, 4, 20, 20);
          cr->fill ();
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
        draw_text (cr);
        draw_source_icon (cr);
        draw_spectrum (cr);

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
    ui_raise (DBusMPX * self,
              GError **      error);

    static gboolean
    startup (DBusMPX * self,
             int            no_network,
             GError **      error);

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
#define mpx_ui_raise    ui_raise

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
	Player::DBusMPX::ui_raise (DBusMPX* self, GError** error)
	{
	  return TRUE;
	}

	gboolean
	Player::DBusMPX::startup  (DBusMPX*   self,
						 int      no_network, 
						 GError** error)
	{
	  return TRUE;
	}

	void
	Player::DBusMPX::startup_complete (DBusMPX* self)
	{
	  g_signal_emit (self, signals[SIGNAL_STARTUP_COMPLETE], 0);
	}

	void
	Player::DBusMPX::shutdown_complete (DBusMPX* self)
	{
	  g_signal_emit (self, signals[SIGNAL_SHUTDOWN_COMPLETE], 0);
	}

	void
	Player::DBusMPX::quit (DBusMPX *self)
	{
	  g_signal_emit (self, signals[SIGNAL_QUIT], 0);
	}


    Player::Player(const Glib::RefPtr<Gnome::Glade::Xml>& xml,
                   MPX::Library & obj_library,
                   MPX::Amazon::Covers & obj_amazon)
    : WidgetLoader<Gtk::Window>(xml, "mpx")
    , m_ref_xml(xml)
	, m_SourceUI(0)
	, m_SourceCtr (0)
	, m_PageCtr(1)
    , m_Library (obj_library)
    , m_Covers (obj_amazon)
   {
		/*------------------------ Load Plugins -------------------------------------------------*/
		// This also initializes Python for us and hence it's of utmost importance it is kept
		// before loading the sources as they might use Python themselves
		m_PluginManager = new PluginManager(this);
		std::string const user_path = build_filename(build_filename(g_get_user_data_dir(), "mpx"),"scripts");
		if(file_test(user_path, FILE_TEST_EXISTS))
			m_PluginManager->append_search_path (user_path);
		m_PluginManager->append_search_path (build_filename(DATA_DIR,"scripts"));
		m_PluginManager->load_plugins();
		m_PluginManagerGUI = PluginManagerGUI::create(*m_PluginManager);

		/*------------------------ Connect Library -----------------------------------------------*/ 
        m_Library.signal_scan_start().connect( sigc::mem_fun( *this, &Player::on_library_scan_start ) );
        m_Library.signal_scan_run().connect( sigc::mem_fun( *this, &Player::on_library_scan_run ) );
        m_Library.signal_scan_end().connect( sigc::mem_fun( *this, &Player::on_library_scan_end ) );

		register_stock_icons();

		signals[PSIGNAL_NEW_TRACK] =
			g_signal_new ("new-track",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

		signals[PSIGNAL_TRACK_PLAYED] =
			g_signal_new ("track-played",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

		signals[PSIGNAL_INFOAREA_CLICK] =
			g_signal_new ("infoarea-click",
					  G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
					  GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
					  0,
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0); 

		m_DiscDefault = Gdk::Pixbuf::create_from_file(build_filename(DATA_DIR,
			build_filename("images","disc-default.png")))->scale_simple(64,64,Gdk::INTERP_BILINEAR);

		init_dbus ();
		DBusObjects.mpx = DBusMPX::create(*this, m_SessionBus);
	
        // Playback Engine
        m_Play = new Play;
		m_Play->signal_eos().connect
			  (sigc::mem_fun (*this, &MPX::Player::on_play_eos));
		m_Play->property_status().signal_changed().connect
			  (sigc::mem_fun (*this, &MPX::Player::on_play_status_changed));
		m_Play->signal_position().connect
			  (sigc::mem_fun (*this, &MPX::Player::on_play_position));
		m_Play->signal_metadata().connect
			  (sigc::mem_fun (*this, &MPX::Player::on_play_metadata));
#if 0
		m_Play->signal_buffering().connect
			  (sigc::mem_fun (*this, &MPX::Player::on_play_buffering));
		m_Play->signal_error().connect
			  (sigc::mem_fun (*this, &MPX::Player::on_play_error));
#endif

        IconTheme::get_default()->prepend_search_path(build_filename(DATA_DIR,"icons"));

		/*------------------------ Create the UI Manager, sources need it ------------------------*/
		m_ui_manager = UIManager::create ();

		/*------------------------ Load Sources --------------------------------------------------*/ 
        m_Sources = new Sources(xml);
        m_ref_xml->get_widget("statusbar", m_Statusbar);
		Util::dir_for_each_entry (build_filename(PLUGIN_DIR, "sources"), sigc::mem_fun(*this, &MPX::Player::load_source_plugin));  

		//----------------------- Volume ---------------------------------------------------------*/
        m_ref_xml->get_widget("volumebutton", m_Volume);
		m_Volume->signal_value_changed().connect( sigc::mem_fun( *this, &Player::on_volume_value_changed ) );
		std::vector<Glib::ustring> Icons;
		Icons.push_back("volume-knob");
		m_Volume->property_size() = Gtk::ICON_SIZE_SMALL_TOOLBAR;
		m_Volume->set_icons(Icons);

		//----------------------- Time Display ---------------------------------------------------*/
        m_ref_xml->get_widget("label-time", m_TimeLabel);

		//---------------------- Seek ------------------------------------------------------------*/
        m_ref_xml->get_widget("scale-seek", m_Seek);
		m_Seek->set_events (Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK);
		m_Seek->signal_event().connect( sigc::mem_fun( *this, &Player::on_seek_event ) );
		m_Seek->set_sensitive(false);
		g_atomic_int_set(&m_Seeking,0);

        //---------------------- Infoarea --------------------------------------------------------*/
        m_InfoArea = InfoArea::create(*m_Play, m_ref_xml);
		m_InfoArea->signal_cover_clicked().connect
		  ( sigc::mem_fun( *this, &Player::on_cover_clicked ));
		m_InfoArea->signal_uris_dropped().connect
		  ( sigc::mem_fun( *this, &MPX::Player::on_infoarea_uris ) );

        //---------------------- UIManager + Menus + Proxy Widgets --------------------------------*/
		m_actions = ActionGroup::create ("Actions_Player");

		m_actions->add (Action::create("MenuMusic", _("_Music")));
		m_actions->add (Action::create("MenuEdit", _("_Edit")));

		m_actions->add (Action::create ("action-play-files",
										Gtk::Stock::OPEN,
										_("_Play Files...")),
										sigc::mem_fun (*this, &Player::on_play_files));

		m_actions->add (Action::create ("action-import-folder",
										Gtk::Stock::HARDDISK,
										_("Import _Folder")),
										sigc::mem_fun (*this, &Player::on_import_folder));

		m_actions->add (Action::create ("action-vacuum-lib",
										_("Vacuum Library")),
										sigc::mem_fun (m_Library, &Library::vacuum));

		m_actions->add (Action::create ("action-import-share",
										Gtk::Stock::NETWORK,
										_("Import _Share")),
										sigc::mem_fun (*this, &Player::on_import_share));

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

		m_actions->add (Action::create (ACTION_PLUGINS,
										Gtk::StockID(MPX_STOCK_PLUGIN),
										_("Plugins...")),
										sigc::mem_fun (*this, &Player::on_show_plugins ));

		m_ui_manager->insert_action_group (m_actions);

		if(Util::ui_manager_add_ui(m_ui_manager, MenubarMain, *this, _("Main Menubar")))
		{
			dynamic_cast<Alignment*>(m_ref_xml->get_widget("alignment-menu"))->add (*(m_ui_manager->get_widget ("/MenubarMain")));
		}

        // Playback Proxies
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

		m_actions->get_action (ACTION_PLAY)->set_sensitive( 0 );
		m_actions->get_action (ACTION_PAUSE)->set_sensitive( 0 );
		m_actions->get_action (ACTION_PREV)->set_sensitive( 0 );
		m_actions->get_action (ACTION_NEXT)->set_sensitive( 0 );
		m_actions->get_action (ACTION_STOP)->set_sensitive( 0 );
        add_accel_group (m_ui_manager->get_accel_group());

#if 0
		dynamic_cast<Gtk::Image *>(m_ref_xml->get_widget ("image-button-repeat"))
		  ->set (render_icon (Gtk::StockID (MPX_STOCK_REPEAT), Gtk::ICON_SIZE_BUTTON));
		dynamic_cast<Gtk::Image *>(m_ref_xml->get_widget ("image-button-shuffle"))
		  ->set (render_icon (Gtk::StockID (MPX_STOCK_SHUFFLE), Gtk::ICON_SIZE_BUTTON));
		m_actions->get_action (ACTION_REPEAT)->connect_proxy
			  (*(dynamic_cast<ToggleButton *>(m_ref_xml->get_widget ("button-repeat"))));
		m_actions->get_action (ACTION_SHUFFLE)->connect_proxy
			  (*(dynamic_cast<ToggleButton *>(m_ref_xml->get_widget ("button-shuffle"))));
		mcs_bind->bind_toggle_action
		  (RefPtr<ToggleAction>::cast_static (m_actions->get_action (ACTION_SHUFFLE)), "mpx", "shuffle");
		mcs_bind->bind_toggle_action
		  (RefPtr<ToggleAction>::cast_static (m_actions->get_action (ACTION_REPEAT)), "mpx", "repeat");
#endif

#if 0
        // Tray icon
        {
          m_status_icon_image_default =
            Gdk::Pixbuf::create_from_file (MPX_TRAY_ICON_DIR G_DIR_SEPARATOR_S "tray-icon-default.png");
          m_status_icon_image_paused =
            Gdk::Pixbuf::create_from_file (MPX_TRAY_ICON_DIR G_DIR_SEPARATOR_S "tray-icon-paused.png");
          m_status_icon_image_playing =
            Gdk::Pixbuf::create_from_file (MPX_TRAY_ICON_DIR G_DIR_SEPARATOR_S "tray-icon-playing.png");

          m_status_icon = bmp_status_icon_new_from_pixbuf (m_status_icon_image_default->gobj());
          bmp_status_icon_set_visible (m_status_icon, TRUE);

          g_object_connect (G_OBJECT (m_status_icon),
                            "signal::click",
                            G_CALLBACK(Player::status_icon_click),
                            this,
                            "signal::popup-menu",
                            G_CALLBACK(Player::status_icon_popup_menu),
                            this,
                            "signal::scroll-up",
                            G_CALLBACK(Player::status_icon_scroll_up),
                            this,
                            "signal::scroll-down",
                            G_CALLBACK(Player::status_icon_scroll_down),
                            this,
                            NULL);

          GtkWidget *tray_icon = bmp_status_icon_get_tray_icon (m_status_icon);
          gtk_widget_realize (GTK_WIDGET (tray_icon));
          g_object_connect (G_OBJECT (tray_icon),
                            "signal::embedded",
                            G_CALLBACK (MPX::Player::on_tray_embedded),
                            this,
                            NULL);
          g_object_connect (G_OBJECT (tray_icon),
                            "signal::destroyed",
                            G_CALLBACK (MPX::Player::on_tray_destroyed),
                            this,
                            NULL);
        }
#endif

		m_Sources->sourceChanged().connect( sigc::mem_fun( *this, &Player::on_source_changed ));
		dynamic_cast<Gtk::ToggleButton*>(m_ref_xml->get_widget("sources-toggle"))->signal_toggled().connect( sigc::mem_fun( *this, &Player::on_sources_toggled ) );

		show_all ();
		DBusObjects.mpx->startup_complete(DBusObjects.mpx); 
    }

	void
	Player::install_source (int source, int tab)
	{
		m_SourceTabMapping.resize(source+1);
		m_SourceTabMapping[source] = tab;

		m_source_flags[source] = PlaybackSource::Flags(0);
		m_source_caps[source] = PlaybackSource::Caps(0); 

		m_SourceV[source]->send_caps();
		m_SourceV[source]->send_flags();

		m_SourceV[source]->signal_caps().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_caps), source));

		m_SourceV[source]->signal_flags().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_flags), source));

		m_SourceV[source]->signal_track_metadata().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_track_metadata), source));

		m_SourceV[source]->signal_playback_request().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_play_request), source));

		m_SourceV[source]->signal_segment().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_segment), source));

		m_SourceV[source]->signal_stop_request().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_stop), source));

		m_SourceV[source]->signal_next_request().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::on_source_next), source));

		m_SourceV[source]->signal_play_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::play_async_cb), source));

		m_SourceV[source]->signal_next_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::next_async_cb), source));

		m_SourceV[source]->signal_prev_async().connect
		  (sigc::bind (sigc::mem_fun (*this, &Player::prev_async_cb), source));

		UriSchemes v = m_SourceV[source]->Get_Schemes ();
		if(v.size())
		{
		  for(UriSchemes::const_iterator i = v.begin(); i != v.end(); ++i)
		  {
				if(m_UriMap.find(*i) != m_UriMap.end())
				{
				  g_warning("%s: Source '%s' tried to override URI handler for scheme '%s'!",
					  G_STRLOC,
					  m_SourceV[source]->get_name().c_str(),
					  i->c_str());	
				  continue;
				}
				g_message("%s: Source '%s' registers for URI scheme '%s'", G_STRLOC, m_SourceV[source]->get_name().c_str(), i->c_str()); 
				m_UriMap.insert (std::make_pair (std::string(*i), source));
		  }
		}
	}

	void
	Player::on_infoarea_uris (Util::FileList const& uris)
	{
		if( uris.size() ) 
		{
			try{
				URI u (uris[0]); //TODO/FIXME: We assume all URIs are of the same scheme
				UriSchemeMap::const_iterator i = m_UriMap.find (u.scheme);
				if( i != m_UriMap.end ())
				{
				  m_SourceV[i->second]->Process_URI_List (uris);
				}
			  }
			catch (URI::ParseError & cxe)
			  {
				g_warning (G_STRLOC ": Couldn't parse URI %s", uris[0].c_str());
			  }
			catch (bad_cast & cxe)
			  {
				g_critical (G_STRLOC ": Source isn't URI handler (dynamic_cast<> failed)");
			  }
		}
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
					  m_SourceV[i->second]->Process_URI_List (uris);
					}
				  }
				catch (URI::ParseError & cxe)
				  {
					g_warning (G_STRLOC ": Couldn't parse URI %s", uris[0].c_str());
				  }
				catch (bad_cast & cxe)
				  {
					g_critical (G_STRLOC ": Source isn't URI handler (dynamic_cast<> failed)");
				  }
			}
		}
	}

	void
	Player::on_show_plugins ()
	{
		m_PluginManagerGUI->present ();
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

	void
	Player::add_widget (Gtk::Widget *widget)
	{
		Gtk::VBox * box;
		m_ref_xml->get_widget("vbox3", box);
		box->pack_start(*widget);
	}

	void
	Player::remove_widget (Gtk::Widget *widget)
	{
		Gtk::VBox * box;
		m_ref_xml->get_widget("vbox3", box);
		box->remove(*widget);
	}

	void
	Player::get_object (PAccess<MPX::Library> & pa)
	{
		pa = PAccess<MPX::Library>(m_Library);
	}

	void	
	Player::get_object (PAccess<MPX::Amazon::Covers> & pa)
	{
		pa = PAccess<MPX::Amazon::Covers>(m_Covers);
	}

	Metadata const&
	Player::get_metadata ()
	{
		return m_Metadata;
	}

    bool
    Player::load_source_plugin (std::string const& path)
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
        p->get_ui()->reparent(*a);
        a->show();
        m_ref_xml->get_widget("sourcepages", m_MainNotebook);
        m_MainNotebook->append_page(*a);
        m_Sources->addSource( p->get_name(), p->get_icon() );
		m_SourceV.push_back(p);
		install_source(m_SourceCtr++, /* tab # */ m_PageCtr++);

		return false;
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
		MPXGstMetadata const& m = m_Play->get_metadata();

		switch (field)
		{
		  case FIELD_IMAGE:
			m_Metadata.Image = m.m_image.get();
			m_InfoArea->set_image (m.m_image.get()->scale_simple (72, 72, Gdk::INTERP_HYPER));
			return;
	
		  case FIELD_TITLE:
			if(m_Metadata[ATTRIBUTE_TITLE])
			{
				std::string album = get<std::string>(m_Metadata[ATTRIBUTE_TITLE].get());
				if(album == m.m_title.get())
					return;
			}

			m_Metadata[ATTRIBUTE_TITLE] = std::string(m.m_title.get());
			reparse_metadata();
			return;

		  case FIELD_ALBUM:
			if(m_Metadata[ATTRIBUTE_ALBUM])
			{
				std::string album = get<std::string>(m_Metadata[ATTRIBUTE_ALBUM].get());
				if(album == m.m_album.get())
					return;
			}

			m_Metadata[ATTRIBUTE_ALBUM] = std::string(m.m_album.get());
			reparse_metadata();
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
#if 0
		if( m_popup )
		{
		  m_popup->set_position (position, duration);
		}
#endif
	  }
	}

    Player*
    Player::create (MPX::Library & obj_library, MPX::Amazon::Covers & obj_amazn)
    {
		const std::string path (build_filename(DATA_DIR, build_filename("glade","mpx.glade")));
		Player * p = new Player(Gnome::Glade::Xml::create (path), obj_library, obj_amazn); 
		return p;
    }

    Player::~Player()
    {
		DBusObjects.mpx->shutdown_complete(DBusObjects.mpx); 
		g_object_unref(G_OBJECT(DBusObjects.mpx));
		delete m_PluginManager;
    }

	void
	Player::on_cover_clicked ()
	{
		g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_INFOAREA_CLICK], 0);
	}

	void
	Player::on_source_caps (PlaybackSource::Caps caps, int source)
	{
	  Mutex::Lock L (m_SourceCFLock);

	  m_source_caps[source] = caps;

	  if( (source == m_Sources->getSource()) || (m_MainNotebook->get_current_page()-1 == source) )
	  {
		m_actions->get_action (ACTION_PREV)->set_sensitive (caps & PlaybackSource::C_CAN_GO_PREV);
		m_actions->get_action (ACTION_NEXT)->set_sensitive (caps & PlaybackSource::C_CAN_GO_NEXT);
		m_actions->get_action (ACTION_PLAY)->set_sensitive (caps & PlaybackSource::C_CAN_PLAY);
	  }

	  if( source == m_Sources->getSource() )
	  {
			if( m_Play->property_status().get_value() == PLAYSTATUS_PLAYING )
			{
				  m_actions->get_action (ACTION_PAUSE)->set_sensitive (caps & PlaybackSource::C_CAN_PAUSE);
			}
	  }
	}

	void
	Player::on_source_flags (PlaybackSource::Flags flags, int source)
	{
	  Mutex::Lock L (m_SourceCFLock);
	  m_source_flags[source] = flags;
	  //m_actions->get_action (ACTION_REPEAT)->set_sensitive (flags & PlaybackSource::F_USES_REPEAT);
	  //m_actions->get_action (ACTION_SHUFFLE)->set_sensitive (flags & PlaybackSource::F_USES_SHUFFLE);
	}

	void
	Player::reparse_metadata ()
	{
		if( !m_Metadata.Image )
		{
			m_Metadata.Image = m_DiscDefault;
		}

		m_InfoArea->set_image (m_Metadata.Image->scale_simple (72, 72, Gdk::INTERP_HYPER));

		ustring artist, album, title;
		parse_metadata (m_Metadata, artist, album, title);
		m_InfoArea->set_text (L_TITLE, title);
		m_InfoArea->set_text (L_ARTIST, artist);
		m_InfoArea->set_text (L_ALBUM, album);
	}

	void
	Player::on_source_track_metadata (Metadata const& metadata, int source)
	{
	  g_return_if_fail( m_Sources->getSource() == source);

	  m_Metadata = metadata;
	  if(!m_Metadata.Image)
	  {
	    if(m_Metadata[ATTRIBUTE_ASIN]) 
			m_Covers.fetch(get<std::string>(m_Metadata[ATTRIBUTE_ASIN].get()), m_Metadata.Image);
		else
			m_Metadata.Image = m_DiscDefault;
	  }

	  if(!m_Metadata[ATTRIBUTE_LOCATION])
		m_Metadata[ATTRIBUTE_LOCATION] = m_Play->property_stream().get_value();

	  reparse_metadata ();
	}

	void
	Player::on_source_play_request (int source_id)
	{
	  g_return_if_fail( m_Sources->getSource() == source_id);

	  if( (m_Sources->getSource() != SOURCE_NONE ) && (m_Play->property_status().get_value() != PLAYSTATUS_STOPPED))
	  {
			if( m_source_flags[m_Sources->getSource()] & PlaybackSource::F_HANDLE_LASTFM )
			{
					track_played ();	
			}

			if( m_Sources->getSource() != source_id)
			{
					m_SourceV[m_Sources->getSource()]->stop ();
					m_Play->request_status (PLAYSTATUS_STOPPED);
			}
	  }

	  PlaybackSource* source = m_SourceV[source_id];

	  if( m_source_flags[source_id] & PlaybackSource::F_ASYNC)
	  {
			m_actions->get_action( ACTION_STOP )->set_sensitive (true);
			source->play_async ();
	  }
	  else
	  {
			  safe_pause_unset();
			  if( source->play () )
			  {
					  m_Play->switch_stream (source->get_uri(), source->get_type());
					  source->play_post ();
					  play_post_internal (source_id);
			  }
			  else
			  {
				source->stop ();
				m_Play->request_status (PLAYSTATUS_STOPPED);
			  }
	  }
	}

	void
	Player::on_source_segment (int source_id)
	{
	  if( source_id == m_Sources->getSource() )
	  {
		m_InfoArea->reset ();
		m_SourceV[source_id]->segment ();
	  }
	  else
	  {
		g_warning (_("%s: Source '%s' requested segment, but is not the active source"), G_STRLOC, m_SourceV[source_id]->get_name().c_str());
	  }
	}

	void
	Player::on_source_stop (int source_id)
	{
	  g_return_if_fail( m_Sources->getSource() == source_id);

	  if( source_id == m_Sources->getSource() )
	  {
		stop ();
	  }
	  else
	  {
		g_warning (_("%s: Source '%s' requested stop, but is not the active source"), G_STRLOC, m_SourceV[source_id]->get_name().c_str());
	  }
	}

	void
	Player::on_source_next (int source_id)
	{
	  g_return_if_fail( m_Sources->getSource() == source_id);

	  if( source_id == m_Sources->getSource() )
	  {
		next ();
	  }
	  else
	  {
		g_warning (_("%s: Source '%s' requested next, but is not the active source"), G_STRLOC, m_SourceV[source_id]->get_name().c_str());
	  }
	}

	//////////////////////////////// Internal Playback Control

	void
	Player::track_played ()
	{
		if(!m_Metadata[ATTRIBUTE_MPX_TRACK_ID])
			return;

		g_message("Track Played; Seconds: %f, Duration: %f", m_TrackPlayedSeconds, m_TrackDuration); 

		if((m_TrackPlayedSeconds >= 240) || (m_TrackPlayedSeconds >= m_TrackDuration/2))
		{
			gint64 id = get<gint64>(m_Metadata[ATTRIBUTE_MPX_TRACK_ID].get());
			m_Library.trackPlayed(id, time(NULL));
			g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_TRACK_PLAYED], 0);
		}

		m_TrackPlayedSeconds = 0.;
	}

	void
	Player::play_async_cb (int source_id)
	{
	  PlaybackSource* source = m_SourceV[source_id];

	  m_Play->switch_stream (source->get_uri(), source->get_type());
	  play_post_internal (source_id);
	}

	void
	Player::play_post_internal (int source_id)
	{
	  PlaybackSource* source = m_SourceV[source_id];

	  source->send_caps ();
	  source->send_metadata ();

	  g_signal_emit (G_OBJECT(gobj()), signals[PSIGNAL_NEW_TRACK], 0);

#if 0
	  if( m_popup )
	  {
			m_popup->set_source_icon (m_source_icons_small[source_id]);
	  }

	  if( m_source_flags[source_id] & PlaybackSource::F_HANDLE_LASTFM )
	  {
			shell_lastfm_now_playing ();
	  }

	  m_player_dbus_obj->emit_track_change (m_player_dbus_obj);

	  if( m_ui_merge_id_context )
	  {
			m_ui_manager->remove_ui (m_ui_merge_id_context);
			m_ui_merge_id_context = boost::dynamic_pointer_cast <UiPart::Base> (source)->add_context_ui();
	  }

	  if( m_ui_merge_id_tray )
	  {
			m_ui_manager->remove_ui (m_ui_merge_id_tray);
			m_ui_merge_id_tray = boost::dynamic_pointer_cast <UiPart::Base> (source)->add_tray_ui();
	  }

	  if( m_ui_merge_id_lastfm )
	  {
			m_ui_manager->remove_ui (m_ui_merge_id_lastfm);
			m_ui_merge_id_lastfm = 0;
	  }

	  m_InfoArea->set_source (m_source_icons_small[source_id]->scale_simple (20, 20, Gdk::INTERP_BILINEAR));

	  if( (m_ui_merge_id_lastfm == 0) && LastFM::Scrobbler::Obj()->connected() && (m_source_flags[source_id] & PlaybackSource::F_HANDLE_LASTFM_ACTIONS) )
	  {
			m_ui_merge_id_lastfm = m_ui_manager->add_ui_from_string (ui_lastfm_actions);
	  }
	  else
	  {
			if (m_ui_merge_id_lastfm != 0)
			{
				  m_ui_manager->remove_ui (m_ui_merge_id_lastfm);
			}
	  }
#endif
	}

	void
	Player::play ()
	{
	  int source_id = int (m_MainNotebook->get_current_page()-1);
	  PlaybackSource::Caps c = m_source_caps[source_id];

	  if( c & PlaybackSource::C_CAN_PLAY )
	  {
		if( (m_Sources->getSource() != SOURCE_NONE ) && (m_Play->property_status().get_value() != PLAYSTATUS_STOPPED))
		{
			  track_played ();
			  m_SourceV[m_Sources->getSource()]->stop ();
			  if( m_Sources->getSource() != source_id)
			  {
					m_Play->request_status (PLAYSTATUS_STOPPED);
			  }
		}

		PlaybackSource* source = m_SourceV[source_id];
		
		if( m_source_flags[source_id] & PlaybackSource::F_ASYNC)
		{
				source->play_async ();
				m_actions->get_action( ACTION_STOP )->set_sensitive (true);
				return;
		}
		else
		{
				safe_pause_unset();
				if( source->play() )
				{
					  m_Play->switch_stream (source->get_uri(), source->get_type());
					  source->play_post ();
					  play_post_internal (source_id);
					  return;
				}
		}
	  }

	  stop ();
	}

	void
	Player::safe_pause_unset ()
	{
	    RefPtr<ToggleAction>::cast_static (m_actions->get_action(ACTION_PAUSE))->set_active(false);
	}

	void
	Player::pause ()
	{
	  bool paused = RefPtr<ToggleAction>::cast_static (m_actions->get_action(ACTION_PAUSE))->get_active();
		  
	  m_InfoArea->set_paused(paused);

	  PlaybackSource::Caps c = m_source_caps[m_Sources->getSource()];
	  if( paused && (c & PlaybackSource::C_CAN_PAUSE ))
		m_Play->request_status (PLAYSTATUS_PAUSED);
	  else
	  if( !paused && (m_Play->property_status() == PLAYSTATUS_PAUSED))
		m_Play->request_status (PLAYSTATUS_PLAYING);
	}

	void
	Player::prev_async_cb (int source_id)
	{
	  PlaybackSource* source = m_SourceV[source_id];
	  PlaybackSource::Flags f = m_source_flags[source_id];

	  if( (f & PlaybackSource::F_PHONY_PREV) == 0 )
	  {
			std::string type = source->get_type ();
			ustring uri = source->get_uri();
			m_Play->switch_stream (uri, type);
			source->prev_post ();
			play_post_internal (source_id);
			//m_player_dbus_obj->emit_track_change (m_player_dbus_obj);
	  }
	}

	void
	Player::prev ()
	{
	  int source_id = int (m_Sources->getSource());
	  PlaybackSource* source = m_SourceV[source_id];
	  PlaybackSource::Flags f = m_source_flags[source_id];
	  PlaybackSource::Caps c = m_source_caps[source_id];

	  if( c & PlaybackSource::C_CAN_GO_PREV )
	  {
			track_played();
			safe_pause_unset();
			if( f & PlaybackSource::F_ASYNC )
			{
					m_actions->get_action (ACTION_PREV)->set_sensitive( false );
					source->go_prev_async ();
					return;
			}
			else
			if( source->go_prev () )
			{
					m_Play->switch_stream (source->get_uri(), source->get_type());
					prev_async_cb (m_Sources->getSource());
					return;
			}
	  }
	
	  stop ();
	}

	void
	Player::next_async_cb (int source_id)
	{
	  PlaybackSource* source = m_SourceV[source_id];
	  PlaybackSource::Flags f = m_source_flags[source_id];

	  if( (f & PlaybackSource::F_PHONY_NEXT) == 0 )
	  {
			safe_pause_unset();
			m_Play->switch_stream (source->get_uri(), source->get_type());
		    source->next_post ();
		    play_post_internal (source_id);
		    //m_player_dbus_obj->emit_track_change (m_player_dbus_obj);
	  }
	}

	void
	Player::on_play_eos ()
	{
	  int source_id = int (m_Sources->getSource());
	  PlaybackSource* source = m_SourceV[source_id];
	  PlaybackSource::Flags f = m_source_flags[source_id];
	  PlaybackSource::Caps c = m_source_caps[source_id];

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
				next_async_cb (m_Sources->getSource());
				return;
			}
	  }

	  stop ();
	}

	void
	Player::next ()
	{
	  if( m_Sources->getSource() != SOURCE_NONE )
	  {
		m_SourceV[m_Sources->getSource()]->skipped();
	  }

	  on_play_eos ();
	}

	void
	Player::stop ()
	{
	  if( m_Sources->getSource() != SOURCE_NONE )
	  {
		track_played();
		int source_id = int( m_Sources->getSource() );
		PlaybackSource::Flags f = m_source_flags[source_id];
		if( f & PlaybackSource::F_ASYNC )
		{
		  m_SourceV[source_id]->stop ();
		}
		safe_pause_unset ();
		m_Play->request_status( PLAYSTATUS_STOPPED );
		m_SourceV[source_id]->send_caps();
	  }
	}

	void
	Player::on_sources_toggled ()
	{
		bool active = dynamic_cast<Gtk::ToggleButton*>(m_ref_xml->get_widget("sources-toggle"))->get_active();
		dynamic_cast<Gtk::Notebook*>(m_ref_xml->get_widget("sourcepages"))->set_current_page( active ? 0 : m_SourceTabMapping[m_Sources->getSource()] ); 
	}

	void
	Player::on_source_changed (int source_id)
	{
		if(m_SourceUI)
		{
			m_ui_manager->remove_ui (m_SourceUI);
		}

		dynamic_cast<Gtk::Notebook*>(m_ref_xml->get_widget("sourcepages"))->set_current_page( m_SourceTabMapping[source_id] );
		dynamic_cast<Gtk::ToggleButton*>(m_ref_xml->get_widget("sources-toggle"))->set_active(false);

		m_SourceUI = m_SourceV[source_id]->add_menu();
		g_message("%s: New merge iD: %u", G_STRLOC, m_SourceUI);
	}

	void
	Player::on_play_status_changed ()
	{
	  MPXPlaystatus status = MPXPlaystatus (m_Play->property_status().get_value());
	  MPX::URI u;

	  switch (status)
	  {
		case PLAYSTATUS_STOPPED:
		{
		  m_Seek->set_sensitive(false);
		  g_atomic_int_set(&m_Seeking, 0);

#if 0
		  if( NM::Obj()->Check_Status() )
		  {
			m_actions->get_action (ACTION_LASTFM_TAG)->set_sensitive( 0 );
			m_actions->get_action (ACTION_LASTFM_LOVE)->set_sensitive( 0 );
			m_actions->get_action (ACTION_LASTFM_RECM)->set_sensitive( 0 );
			m_actions->get_action (ACTION_LASTFM_PLAY)->set_sensitive( 0 );
		  }

		  if( m_ui_merge_id_context )
		  {
			m_ui_manager->remove_ui (m_ui_merge_id_context);
			m_ui_merge_id_context = 0;
		  }

		  if( m_ui_merge_id_tray )
		  {
			m_ui_manager->remove_ui (m_ui_merge_id_tray);
			m_ui_merge_id_tray = 0;
		  }

		  if( m_ui_merge_id_lastfm )
		  {
			m_ui_manager->remove_ui (m_ui_merge_id_lastfm);
			m_ui_merge_id_lastfm = 0;
		  }
#endif

		  break;
		}

		case PLAYSTATUS_PLAYING:
		{
		  g_atomic_int_set(&m_Seeking,0);
		  m_actions->get_action( ACTION_STOP )->set_sensitive (true);
		  break;
		}

		default: ;
	  }

	  switch (status)
	  {
		  case PLAYSTATUS_NONE:
			/* ... */
			break;

		  case PLAYSTATUS_SEEKING:
			/* ... */
			break;

		  case PLAYSTATUS_STOPPED:
		  {
			m_Metadata = Metadata();
#if 0
			mVideoWidget->property_playing() = false;
			RefPtr<RadioAction>::cast_static (m_actions->get_action(ACTION_VIDEO_ASPECT_AUTO))->set_current_value( 0 );
			m_actions->get_action("MenuVideo")->set_sensitive( false );
			mMainNotebook->set_current_page( 0 );
#endif

			if( m_Sources->getSource() != SOURCE_NONE )
			{
			  m_SourceV[m_Sources->getSource()]->stop ();
			  m_SourceV[m_Sources->getSource()]->send_caps ();
			}

			m_InfoArea->reset ();

			m_TimeLabel->set_text("      …      ");
			m_Seek->set_range(0., 1.);
			m_Seek->set_value(0.);
			m_Seek->set_sensitive(false);

#if 0
			if( m_popup )
			{
			  m_popup->hide ();
			}
			bmp_status_icon_set_from_pixbuf (m_status_icon, m_status_icon_image_default->gobj());
			m_actions->get_action (ACTION_LASTFM_RECM)->set_sensitive( 0 );
			m_actions->get_action (ACTION_TRACK_DETAILS)->set_sensitive( 0 );
			m_actions->get_action (ACTION_CREATE_BOOKMARK)->set_sensitive( 0 );
#endif

			m_actions->get_action (ACTION_STOP)->set_sensitive( false );
			m_actions->get_action (ACTION_NEXT)->set_sensitive( false );
			m_actions->get_action (ACTION_PREV)->set_sensitive( false );
			m_actions->get_action (ACTION_PAUSE)->set_sensitive( false );

			break;
		  }

		  case PLAYSTATUS_WAITING:
		  {
			m_TimeLabel->set_text("      …      ");
			m_Seek->set_range(0., 1.);
			m_Seek->set_value(0.);
			m_Seek->set_sensitive(false);
#if 0
			bmp_status_icon_set_from_pixbuf (m_status_icon, m_status_icon_image_default->gobj());
#endif
			break;
		  }

		  case PLAYSTATUS_PLAYING:
		  {
#if 0
			bmp_status_icon_set_from_pixbuf (m_status_icon, m_status_icon_image_playing->gobj());
			m_spm_paused = false;
#endif
			m_Seek->set_sensitive(true);
			break;
		  }

		  case PLAYSTATUS_PAUSED:
		  {
#if 0
			bmp_status_icon_set_from_pixbuf (m_status_icon, m_status_icon_image_paused->gobj());
			m_spm_paused = false;
#endif
			break;
		  }
	  }

#if 0
	  if( m_popup )
	  {
		m_popup->set_playstatus (status);
	  }
	  m_player_dbus_obj->emit_status_change (m_player_dbus_obj, int (status));
#endif
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
            m_Library.scanUri(uri);
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

                m_MountFile = g_vfs_get_file_for_uri(g_vfs_get_default(), m_Share.c_str());
                if(!G_IS_FILE(m_MountFile))
                {
                    MessageDialog dialog (*this, (boost::format (_("An Error occcured getting a handle for the share '%s'\n"
                        "Please veryify the share URI and credentials")) % m_Share.c_str()).str());
                    dialog.run();
                    goto rerun_import_share_dialog;
                }

                GMountOperation * m_MountOperation = g_mount_operation_new();
                if(!G_IS_MOUNT_OPERATION(m_MountOperation))
                {
                    MessageDialog dialog (*this, (boost::format (_("An Error occcured trying to mount the share '%s'")) % m_Share.c_str()).str());
                    dialog.run();
                    return; 
                }

                g_mount_operation_set_username(m_MountOperation, login.c_str());
                g_mount_operation_set_password(m_MountOperation, password.c_str()); 
                g_signal_connect(m_MountOperation, "ask_password", G_CALLBACK(Player::ask_password_cb), NULL);
                g_file_mount_mountable(m_MountFile, G_MOUNT_MOUNT_NONE, m_MountOperation, NULL, mount_ready_callback, this);
            }
    }

    gboolean
    Player::ask_password_cb (GMountOperation *op,
             const char      *message,
             const char      *default_user,
             const char      *default_domain,
             GAskPasswordFlags   flags)
    {
      Glib::ustring value;
      if (flags & G_ASK_PASSWORD_NEED_USERNAME)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Username:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED /*abort*/);
            return TRUE;
        }
        p->get_request_infos(value);
        g_mount_operation_set_username (op, value.c_str());
        delete p;
        value.clear();
      }

      if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Domain:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED /*abort*/);
            return TRUE;
        }
        p->get_request_infos(value);
        g_mount_operation_set_domain (op, value.c_str());
        delete p;
        value.clear();
      }

      if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Password:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED /*abort*/);
            return TRUE;
        }
        p->get_request_infos(value);
        g_mount_operation_set_password (op, value.c_str());
        delete p;
        value.clear();
      }

      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
      return TRUE;
    }

    void
    Player::mount_ready_callback (GObject *source_object,
                     GAsyncResult *res,
                     gpointer data)
    {
        Player & player = *(reinterpret_cast<Player*>(data));
        GError *error = 0;
        g_file_mount_mountable_finish(player.m_MountFile, res, &error);

        if(error)
        {
            if(error->code != G_IO_ERROR_ALREADY_MOUNTED)
            {
                MessageDialog dialog (player, (boost::format ("An Error occcured while mounting the share: %s") % error->message).str());
                dialog.run();
                return;
            }
            else
                g_warning("%s: Location '%s' is already mounted", G_STRLOC, player.m_Share.c_str());

            g_error_free(error);
        }

        //g_object_unref(player.m_MountOperation);
        //g_object_unref(player.m_MountFile);
     
        player.m_Library.scanUri( player.m_Share, player.m_ShareName );  
    }

    void
    Player::unmount_ready_callback (GObject *source_object,
                                    GAsyncResult *res,
                                    gpointer data)
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
}
