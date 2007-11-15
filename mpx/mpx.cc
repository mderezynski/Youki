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
#include "config.h"
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <cairomm/cairomm.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gio/gvfs.h>
#include <glib/gstdio.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include "import-share.hh"
#include "import-folder.hh"
#include "mpx.hh"
#include "mpx-sources.hh"
#include "request-value.hh"
#include "util-graphics.hh"
#include "util-ui.hh"
#include "source-musiclib.hh"
using namespace Glib;
using namespace Gtk;
using namespace std;
using namespace Gnome::Glade;
using namespace MPX::Util;

namespace
{
  char const * MenubarMain =
  "<ui>"
  ""
  "<menubar name='MenubarMain'>"
  "   <menu action='MenuMusic'>"
  "         <menuitem action='action-import-folder'/>"
  "         <menuitem action='action-import-share'/>"
  "         <separator name='sep00'/>"
  "         <menuitem action='action-quit'/>"
  "   </menu>"
  "</menubar>"
  ""
  "</ui>"
  "";
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

    //FIXME: Encode size here as well
  };

  LayoutData const layout_info[] = {
    {-0.0, 1.0, 86,  8},
    {-0.4, 1.0, 86, 40},
    {-0.4, 0.8, 86, 58}
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

#ifdef APP_MAINTENANCE
  class AnimParams
    : public WidgetLoader<Gtk::Window>
  {
      Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;
      Gtk::Range * m_range_velocity;
      Gtk::Range * m_range_gravity;
      Gtk::Range * m_range_elasticity;

      double & m_velocity; 
      double & m_gravity;
      double & m_elasticity;

      enum RangeId
      {
        RANGE_VELOCITY,
        RANGE_GRAVITY,
        RANGE_ELASTICITY
      };

      void
      on_range_value_changed (RangeId id)
      {
        switch(id)
        {
          case RANGE_VELOCITY:
            m_velocity = m_range_velocity->get_value()/1000.;
            break;

          case RANGE_GRAVITY:
            m_gravity = m_range_gravity->get_value()/1000.;
            break;

          case RANGE_ELASTICITY:
            m_elasticity = m_range_elasticity->get_value()/1000.;
            break;
        } 
      }

    public:

      static AnimParams*
      create (double & velocity, double & gravity, double & elasticity)
      {
        const std::string path (build_filename(DATA_DIR, build_filename("glade","dialog-bounce-adjust.glade")));
        AnimParams *p = new AnimParams(Gnome::Glade::Xml::create (path), velocity, gravity, elasticity);
        return p;
      }

      AnimParams(const Glib::RefPtr<Gnome::Glade::Xml>& xml,
              double & velocity,
              double & gravity,
              double & elasticity)
      : WidgetLoader<Gtk::Window>(xml, "anim-params")
      , m_ref_xml(xml)
      , m_velocity(velocity)
      , m_gravity(gravity)
      , m_elasticity(elasticity)
      {
        xml->get_widget("velocity", m_range_velocity);
        xml->get_widget("gravity", m_range_gravity);
        xml->get_widget("elasticity", m_range_elasticity);

        m_range_velocity->set_value(velocity*1000.);
        m_range_gravity->set_value(gravity*1000.);
        m_range_elasticity->set_value(elasticity*1000.);

        m_range_velocity->signal_value_changed().connect(
          sigc::bind( sigc::mem_fun( *this, &AnimParams::on_range_value_changed ), RANGE_VELOCITY));

        m_range_gravity->signal_value_changed().connect(
          sigc::bind( sigc::mem_fun( *this, &AnimParams::on_range_value_changed ), RANGE_GRAVITY));

        m_range_elasticity->signal_value_changed().connect(
          sigc::bind( sigc::mem_fun( *this, &AnimParams::on_range_value_changed ), RANGE_ELASTICITY));
      }
  };
#endif // APP_MAINTENANCE
 
  class InfoArea
    : public EventBox
  {
    private:

#ifdef APP_MAINTENANCE
      AnimParams * m_ParamControl;
#endif // APP_MAINTENANCE

#if 0
      Spectrum m_spectrum_data;
      Spectrum m_spectrum_peak;
#endif

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
      bool                                m_compact;
      sigc::connection                    m_cover_anim_conn;
      sigc::connection                    m_conn_decay;

    public:

#if 0
      typedef sigc::signal<void, VUri const&> SignalUris;
      typedef sigc::signal<void> SignalCoverClicked;

      SignalUris &
      signal_uris_dropped()
      {
        return m_signal_uris_dropped;
      }

      SignalCoverClicked&
      signal_cover_clicked()
      {
        return m_signal_cover_clicked;
      }

    private:

      SignalUris         m_signal_uris_dropped;
      SignalCoverClicked m_signal_cover_clicked;
#endif

    protected:

#if 0
      bool
      on_button_press_event (GdkEventButton * event)
      {
        int x = int (event->x);
        int y = int (event->y);

        if ((x >= 6) && (x <= 78) && (y >= 3) && (y <= 75))
        {
          int status = Play::Obj()->property_status();
          if (status != PLAYSTATUS_STOPPED && status != PLAYSTATUS_WAITING)
          {
            m_signal_cover_clicked.emit ();
          }
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
          VUri u = data.get_uris();
          m_signal_uris_dropped.emit (u);
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

          VUri u;

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
            m_signal_uris_dropped.emit (u);
          }
        }
      }
#endif

    private:

#if 0
      void
      enable_drag_dest ()
      {
        disable_drag_dest ();

        DNDEntries target_entries;
        target_entries.push_back (TargetEntry ("text/plain"));

        drag_dest_set (target_entries, Gtk::DEST_DEFAULT_MOTION);
        drag_dest_add_uri_targets ();
      }

      void
      disable_drag_dest ()
      {
        drag_dest_unset ();
      }
#endif

    public:

      InfoArea (BaseObjectType                 * obj,
                RefPtr<Gnome::Glade::Xml> const& xml)
      : EventBox          (obj)
#if 0
      , m_spectrum_data   (SPECT_BANDS, 0)
      , m_spectrum_peak   (SPECT_BANDS, 0)
#endif
      , m_source_icon     (Glib::RefPtr<Gdk::Pixbuf> (0))
      , m_cover_alpha     (1.0)
      , m_compact         (false)
      {
        add_events (Gdk::BUTTON_PRESS_MASK);

        Gdk::Color const color ("#000000");
        modify_bg (Gtk::STATE_NORMAL, color);
        modify_base (Gtk::STATE_NORMAL, color);

#if 0
        Play::Obj ()->signal_spectrum ().connect (sigc::mem_fun (this, &InfoArea::play_update_spectrum));
        Play::Obj ()->property_status ().signal_changed ().connect (sigc::mem_fun (this, &InfoArea::play_status_changed));

        enable_drag_dest ();
#endif

#ifdef APP_MAINTENANCE
        m_ParamControl = AnimParams::create(cover_anim_initial_velocity, cover_anim_gravity, cover_anim_wall_elasticity);
#endif // APP_MAINTENANCE
      }

      ~InfoArea ()
      {}

      void
      set_source (Glib::RefPtr<Gdk::Pixbuf> const& source_icon)
      {
        m_source_icon = source_icon;
        queue_draw ();
      }

      void
      set_compact (bool compact)
      {
        m_compact = compact;
        queue_draw ();
      }

      void
      reset ()
      {
#if 0
        std::fill (m_spectrum_data.begin (), m_spectrum_data.end (), 0);
        std::fill (m_spectrum_peak.begin (), m_spectrum_peak.end (), 0);
#endif

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

#if 0
      bool
      decay_spectrum ()
      {
        for (int n = 0; n < SPECT_BANDS; ++n)
        {
          m_spectrum_data[n] = (((m_spectrum_data[n] - 6) < 0) ? 0 : (m_spectrum_data[n] - 6));
          m_spectrum_peak[n] = (((m_spectrum_peak[n] - 4) < 0) ? 0 : (m_spectrum_peak[n] - 4));
        }
        queue_draw ();
        return true;
      }
#endif

#if 0
      void
      play_status_changed ()
      {
        MPXPlaystatus status = MPXPlaystatus (Play::Obj ()->property_status ().get_value ());
        if( status == PLAYSTATUS_PAUSED )
        {
          m_conn_decay = Glib::signal_timeout ().connect (sigc::mem_fun (this, &InfoArea::decay_spectrum), 50);
        }
        else
        {
          m_conn_decay.disconnect ();
        }
      }

      void
      play_update_spectrum (Spectrum const& spectrum)
      {
        for (int n = 0; n < SPECT_BANDS; ++n)
        {
          if( spectrum[n] < m_spectrum_data[n] )
          {
            m_spectrum_data[n] = (((m_spectrum_data[n] - 6) < 0) ? 0 : (m_spectrum_data[n] - 6));
            m_spectrum_peak[n] = (((m_spectrum_peak[n] - 2) < 0) ? 0 : (m_spectrum_peak[n] - 2));
          }
          else
          {
            m_spectrum_data[n] = spectrum[n];
            m_spectrum_peak[n] = spectrum[n];
          }
        }

        queue_draw ();
      }
#endif

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

          cr->rectangle (cover_anim_area_x0, cover_anim_area_y0, cover_anim_area_width, cover_anim_area_height);
          cr->clip ();

          double y = (cover_anim_area_y0 + cover_anim_area_y1 - m_cover_surface->get_height ()) / 2;

          draw_cairo_image (cr, m_cover_surface, m_cover_pos, y, m_cover_alpha);

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
          p->layout->set_width ((allocation.get_width() - p->x - 210) * PANGO_SCALE);
          p->layout->set_wrap (Pango::WRAP_CHAR);

          pango_cairo_show_layout (cr->cobj(), p->layout->gobj());
        }
      }

      void
      draw_spectrum (Cairo::RefPtr<Cairo::Context> const& cr)
      {
#if 0
        Gtk::Allocation allocation = get_allocation ();

        for (int n = 0; n < SPECT_BANDS; ++n)
        {
          int x = 0, y = 0, w = 0, h = 0;

          // Bar
          x = allocation.get_width () - 200 + (n*12);
          y = 11 + (64 - m_spectrum_data[n]);

          w = 10;
          h = m_spectrum_data[n];

          cr->set_source_rgba (1.0, 1.0, 1.0, 0.3);
          cr->rectangle (x,y,w,h);
          cr->fill ();

          // Peak
          if( m_spectrum_peak[n] > 0 )
          {
            y = 11 + (64 - m_spectrum_peak[n]);

            h = 1;

            cr->set_source_rgba (1.0, 1.0, 1.0, 0.65);
            cr->rectangle (x, y-1, w, h);
            cr->fill ();
          }
        }
#endif
      }

      void
      draw_source_icon (Cairo::RefPtr<Cairo::Context> const& cr)
      {
        if( m_source_icon && m_compact )
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
  };
}



namespace MPX
{
    Player::Player(Glib::RefPtr<Gnome::Glade::Xml> const& xml,
                   MPX::Library & obj_library,
                   MPX::Amazon::Covers & obj_amazon)
    : WidgetLoader<Gtk::Window>(xml, "mpx")
    , m_ref_xml(xml)
    , m_Library (obj_library)
    , m_Covers (obj_amazon)
   {
        m_Library.signal_scan_start().connect( sigc::mem_fun( *this, &Player::on_library_scan_start ) );
        m_Library.signal_scan_run().connect( sigc::mem_fun( *this, &Player::on_library_scan_run ) );
        m_Library.signal_scan_end().connect( sigc::mem_fun( *this, &Player::on_library_scan_end ) );

        // m_ref_xml->get_widget_derived("sources", m_Sources); TODO: Remove old-style sources stuff
        m_ref_xml->get_widget("statusbar", m_Statusbar);

        IconTheme::get_default()->prepend_search_path(build_filename(DATA_DIR,"icons"));

        ///// TODO: THIS IS ONLY TEMPORARY
        PlaybackSourceMusicLib * p = new PlaybackSourceMusicLib(obj_library, obj_amazon);
        Gtk::Notebook * n;
        Gtk::Alignment * a = new Gtk::Alignment;
        p->get_ui()->reparent(*a);
        a->show();
        m_ref_xml->get_widget("sourcepages", n);
        n->append_page(*a);
        //m_Sources->addSource( _("Music"), p->get_icon() );
        ///// TODO: THIS IS ONLY TEMPORARY

        // Infoarea
        {
          m_ref_xml->get_widget_derived("infoarea", m_InfoArea);
      
#if 0
          m_infoarea->signal_uris_dropped().connect
            ( sigc::mem_fun( *this, &MPX::Player::on_uris_dropped ) );

          m_infoarea->signal_cover_clicked().connect
            ( sigc::mem_fun( *this, &MPX::Player::on_shell_display_track_details) );
#endif
        }

#if 0
        // Playback Engine Signals
        {
          Play::Obj()->property_status().signal_changed().connect
                (sigc::mem_fun (*this, &MPX::PlayerShell::on_play_status_changed));
          Play::Obj()->signal_position().connect
                (sigc::mem_fun (*this, &MPX::PlayerShell::on_play_position));
          Play::Obj()->signal_buffering().connect
                (sigc::mem_fun (*this, &MPX::PlayerShell::on_play_buffering));
          Play::Obj()->signal_error().connect
                (sigc::mem_fun (*this, &MPX::PlayerShell::on_play_error));
          Play::Obj()->signal_eos().connect
                (sigc::mem_fun (*this, &MPX::PlayerShell::on_play_eos));
          Play::Obj()->signal_metadata().connect
                (sigc::mem_fun (*this, &MPX::PlayerShell::on_gst_metadata));
        }
#endif

        // UIManager + Menus + Proxy Widgets
        {
            m_actions = ActionGroup::create ("Actions_Player");

            m_actions->add(Action::create("MenuMusic", _("_Music")));

            m_actions->add (Action::create ("action-import-folder",
                                            Gtk::Stock::HARDDISK,
                                            _("Import _Folder")),
                                            sigc::mem_fun (*this, &Player::on_import_folder));

            m_actions->add (Action::create ("action-import-share",
                                            Gtk::Stock::NETWORK,
                                            _("Import _Share")),
                                            sigc::mem_fun (*this, &Player::on_import_share));

            m_actions->add (Action::create ("action-quit",
                                            Gtk::Stock::QUIT,
                                            _("_Quit")),
                                            AccelKey("<ctrl>Q"),
                                            sigc::ptr_fun ( &Gtk::Main::quit ));

            m_ui_manager = UIManager::create ();
            m_ui_manager->insert_action_group (m_actions);

            if(Util::ui_manager_add_ui(m_ui_manager, MenubarMain, *this, _("Main Menubar")))
            {
                dynamic_cast<Alignment*>(m_ref_xml->get_widget("alignment-menu"))->add (*(m_ui_manager->get_widget ("/MenubarMain")));
            }
        }

#if 0
        // Playback Proxies
        {
          m_actions->get_action (PLAYER_SHELL_ACTION_PLAY)->connect_proxy
                (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("b_play"))));
          m_actions->get_action (PLAYER_SHELL_ACTION_PAUSE)->connect_proxy
                (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("b_pause"))));
          m_actions->get_action (PLAYER_SHELL_ACTION_PREV)->connect_proxy
                (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("b_prev"))));
          m_actions->get_action (PLAYER_SHELL_ACTION_NEXT)->connect_proxy
                (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("b_next"))));
          m_actions->get_action (PLAYER_SHELL_ACTION_STOP)->connect_proxy
                (*(dynamic_cast<Button *>(m_ref_xml->get_widget ("b_stop"))));

          dynamic_cast<Gtk::Image *>(m_ref_xml->get_widget ("image-button-repeat"))
            ->set (render_icon (Gtk::StockID (MPX_STOCK_REPEAT), Gtk::ICON_SIZE_BUTTON));
          dynamic_cast<Gtk::Image *>(m_ref_xml->get_widget ("image-button-shuffle"))
            ->set (render_icon (Gtk::StockID (MPX_STOCK_SHUFFLE), Gtk::ICON_SIZE_BUTTON));

          m_actions->get_action (PLAYER_SHELL_ACTION_REPEAT)->connect_proxy
                (*(dynamic_cast<ToggleButton *>(m_ref_xml->get_widget ("button-repeat"))));
          m_actions->get_action (PLAYER_SHELL_ACTION_SHUFFLE)->connect_proxy
                (*(dynamic_cast<ToggleButton *>(m_ref_xml->get_widget ("button-shuffle"))));

          mcs_bind->bind_toggle_action
            (RefPtr<ToggleAction>::cast_static (m_actions->get_action (PLAYER_SHELL_ACTION_SHUFFLE)), "bmp", "shuffle");
          mcs_bind->bind_toggle_action
            (RefPtr<ToggleAction>::cast_static (m_actions->get_action (PLAYER_SHELL_ACTION_REPEAT)), "bmp", "repeat");

          m_actions->get_action (PLAYER_SHELL_ACTION_PLAY)->set_sensitive( 0 );
          m_actions->get_action (PLAYER_SHELL_ACTION_PAUSE)->set_sensitive( 0 );
          m_actions->get_action (PLAYER_SHELL_ACTION_PREV)->set_sensitive( 0 );
          m_actions->get_action (PLAYER_SHELL_ACTION_NEXT)->set_sensitive( 0 );
          m_actions->get_action (PLAYER_SHELL_ACTION_STOP)->set_sensitive( 0 );
        }
#endif

        add_accel_group (m_ui_manager->get_accel_group());

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
                            G_CALLBACK(PlayerShell::status_icon_click),
                            this,
                            "signal::popup-menu",
                            G_CALLBACK(PlayerShell::status_icon_popup_menu),
                            this,
                            "signal::scroll-up",
                            G_CALLBACK(PlayerShell::status_icon_scroll_up),
                            this,
                            "signal::scroll-down",
                            G_CALLBACK(PlayerShell::status_icon_scroll_down),
                            this,
                            NULL);

          GtkWidget *tray_icon = bmp_status_icon_get_tray_icon (m_status_icon);
          gtk_widget_realize (GTK_WIDGET (tray_icon));
          g_object_connect (G_OBJECT (tray_icon),
                            "signal::embedded",
                            G_CALLBACK (MPX::PlayerShell::on_tray_embedded),
                            this,
                            NULL);
          g_object_connect (G_OBJECT (tray_icon),
                            "signal::destroyed",
                            G_CALLBACK (MPX::PlayerShell::on_tray_destroyed),
                            this,
                            NULL);
        }
#endif
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
                g_mount_for_location(m_MountFile, m_MountOperation, NULL, mount_ready_callback, this);
            }
    }

    gboolean
    Player::ask_password_cb (GMountOperation *op,
             const char      *message,
             const char      *default_user,
             const char      *default_domain,
             GPasswordFlags   flags)
    {
      Glib::ustring value;
      if (flags & G_PASSWORD_FLAGS_NEED_USERNAME)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Username:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            g_mount_operation_reply (op, TRUE /*abort*/);
            return TRUE;
        }
        p->get_request_infos(value);
        g_mount_operation_set_username (op, value.c_str());
        delete p;
        value.clear();
      }

      if (flags & G_PASSWORD_FLAGS_NEED_DOMAIN)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Domain:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            g_mount_operation_reply (op, TRUE /*abort*/);
            return TRUE;
        }
        p->get_request_infos(value);
        g_mount_operation_set_domain (op, value.c_str());
        delete p;
        value.clear();
      }

      if (flags & G_PASSWORD_FLAGS_NEED_PASSWORD)
      {
        RequestValue * p = RequestValue::create();
        p->set_question(_("Please Enter the Password:"));
        int reply = p->run();
        if(reply == GTK_RESPONSE_CANCEL)
        {
            g_mount_operation_reply (op, TRUE /*abort*/);
            return TRUE;
        }
        p->get_request_infos(value);
        g_mount_operation_set_password (op, value.c_str());
        delete p;
        value.clear();
      }

      g_mount_operation_reply (op, FALSE);
      return TRUE;
    }

    void
    Player::mount_ready_callback (GObject *source_object,
                     GAsyncResult *res,
                     gpointer data)
    {
        Player & player = *(reinterpret_cast<Player*>(data));
        GError *error = 0;
        g_mount_for_location_finish(player.m_MountFile, res, &error);

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
