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
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glibmm/i18n.h>
#include <glib/gstdio.h>
#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include "mpx.hh"
using namespace Glib;
using namespace Gtk;

#if 0
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
    double  alpha;
    double  target;
    int     x;
    int     y;

  //FIXME: Encode size here as well
  } layout_info[] = { 
    {-0.0, 1.0, 116, 10},
    {-0.4, 1.0, 116, 42},
    {-0.4, 0.8, 116, 60}
  };

  class InfoArea
    : public EventBox
  {
    private:

      Spectrum        m_spectrum_data;
      Spectrum        m_spectrum_peak;

      struct Text
      {
        Text (Gtk::Widget & w, ustring const& text, LayoutID id)
        : m_alpha   (layout_info[id].alpha)
        , m_target  (layout_info[id].target)
        , m_x       (layout_info[id].x)
        , m_y       (layout_info[id].y)
        {
          m_layout = w.create_pango_layout ("");
          m_layout->set_markup (text);
        }

        ~Text () {}

        RefPtr<Pango::Layout> m_layout;
        double                m_alpha;
        double                m_target;
        int                   m_x, m_y;
        sigc::connection      m_conn;
      };
  
      typedef boost::shared_ptr <Text>    TextP;
      typedef boost::shared_ptr <Mutex>   MutexP;

      typedef std::map <LayoutID, TextP>    Layouts;
      typedef std::map <LayoutID, MutexP>   LayoutsLocks;
    
      Layouts         m_layouts;
      LayoutsLocks    m_locks;
      Mutex           m_layouts_lock;
      ustring         m_text[N_LAYOUTS];

      RefPtr<Pango::Layout> m_layout_message;

      Cairo::RefPtr<Cairo::ImageSurface>  m_surface;
      Cairo::RefPtr<Cairo::ImageSurface>  m_surface_frame;
      RefPixbuf                           m_source_icon;

      sigc::connection                    m_surface_conn;
      sigc::connection                    m_conn_decay;

      double    m_surface_alpha;
      bool      m_frame;
      bool      m_compact;
      Mutex     m_surface_lock;
  
    public:

      bool      m_showing_message; // FIXME: Make private

      typedef sigc::signal<void, VUri const&> SignalUris;
      typedef sigc::signal<void> SignalCoverClicked;

      SignalUris &
      signal_uris_dropped()
      {
        return signal_uris_dropped_;
      }

      SignalCoverClicked&
      signal_cover_clicked()
      {
        return signal_cover_clicked_;
      }

    private:

      SignalUris signal_uris_dropped_;
      SignalCoverClicked signal_cover_clicked_;

    protected:

      bool
      on_button_press_event (GdkEventButton * event) 
      {
        int x = int (event->x);
        int y = int (event->y);

        if ((x >= 34) && (x <=98) && (y >= 8) && (y <= 88))
        {
          int status = Play::Obj()->property_status();
          if (status != PLAYSTATUS_STOPPED && status != PLAYSTATUS_WAITING)
          {
            signal_cover_clicked_.emit ();
          }
        }

        return false;
      }

      bool
      on_drag_drop (RefPtr<Gdk::DragContext> const& context, int x, int y, guint time)
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
      on_drag_data_received (RefPtr<Gdk::DragContext> const& context, int x, int y,
                             Gtk::SelectionData const& data, guint info, guint time)
      {
        if( data.get_data_type() == "text/uri-list")
        {
          VUri u = data.get_uris();
          signal_uris_dropped_.emit (u);
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
            signal_uris_dropped_.emit (u);
          }
        }
      }

    private:

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

    public:

      InfoArea (BaseObjectType                 * obj,
                RefPtr<Gnome::Glade::Xml> const& xml)
      : EventBox          (obj)
      , m_source_icon     (RefPixbuf(0))
      , m_frame           (false)
      , m_compact         (false)
      , m_showing_message (false)
      {
        gtk_widget_add_events (GTK_WIDGET (gobj()), GDK_BUTTON_PRESS_MASK);

        modify_bg (Gtk::STATE_NORMAL, Gdk::Color ("#000000"));
        modify_base (Gtk::STATE_NORMAL, Gdk::Color ("#000000"));

        for (int n = 0; n < SPECT_BANDS; ++n)
        {
          m_spectrum_data.push_back (0);
          m_spectrum_peak.push_back (0);
        }

        Play::Obj()->signal_spectrum().connect (sigc::mem_fun (*this, &MPX::InfoArea::play_update_spectrum));
        Play::Obj()->property_status().signal_changed().connect (sigc::mem_fun (*this, &MPX::InfoArea::play_status_changed));

        m_locks.insert (make_pair (L_TITLE, MutexP (new Mutex())));
        m_locks.insert (make_pair (L_ARTIST, MutexP (new Mutex())));
        m_locks.insert (make_pair (L_ALBUM, MutexP (new Mutex())));

        m_surface_frame = Util::cairo_image_surface_from_pixbuf (Gdk::Pixbuf::create_from_file (build_filename (MPX_IMAGE_DIR, "cover-frame.png")));
  
        enable_drag_dest ();
      }

      ~InfoArea ()
      {}

      void
      set_source (Glib::RefPtr<Gdk::Pixbuf> source_icon)
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
        Mutex::Lock L1 (m_layouts_lock);
        Mutex::Lock L2 (m_surface_lock);

        for (int n = 0; n < SPECT_BANDS; m_spectrum_data[n++] = 0); 
        for (int n = 0; n < SPECT_BANDS; m_spectrum_peak[n++] = 0); 

        remove_layout_if_exists (L_ARTIST);
        remove_layout_if_exists (L_ALBUM);
        remove_layout_if_exists (L_TITLE);

        m_surface_conn.disconnect ();
        m_surface = Cairo::RefPtr<Cairo::ImageSurface>(0);

        m_text[0] = ustring ();
        m_text[1] = ustring ();
        m_text[2] = ustring ();

        // Not calling message_clear as it locks
        m_showing_message = false;
        m_layout_message = RefPtr<Pango::Layout>(0);

        set_source (RefPixbuf(0));
        queue_draw ();
      }

      void
      message_set (ustring const& text)
      {
        Mutex::Lock L (m_layouts_lock);
        m_showing_message = true;
        m_layout_message = create_pango_layout ("");
        m_layout_message->set_markup ((boost::format ("%s") % text.c_str()).str());
        queue_draw ();
      }

      void
      message_clear ()
      {
        Mutex::Lock L (m_layouts_lock);
        m_showing_message = false;
        m_layout_message = RefPtr<Pango::Layout>(0);
        queue_draw ();
      }

      void
      set_text (LayoutID id, ustring const& text)
      {
        Mutex::Lock L (m_layouts_lock);
        if( text != m_text[id] )
        {
          m_text[id] = text;
          TextP p = TextP (new Text (*this, text, id));

          remove_layout_if_exists (id);
          insert_layout_and_connect (id, p);
        } 
      }

      void
      set_frame (bool frame)
      {
        Mutex::Lock L (m_surface_lock); 
        m_frame = frame;
        queue_draw ();
      }

      void
      set_image (RefPtr<Gdk::Pixbuf> pixbuf, bool draw_frame)
      {
        Mutex::Lock L (m_surface_lock); 
        m_frame = draw_frame;
        m_surface = Util::cairo_image_surface_from_pixbuf (pixbuf);
        m_surface_alpha = -0.5;
        m_surface_conn.disconnect ();
        m_surface_conn = signal_timeout().connect (sigc::mem_fun (*this, &MPX::InfoArea::fade_in_surface), 20);
      }

      void
      set_image (Cairo::RefPtr<Cairo::ImageSurface> surface, bool draw_frame)
      {
        Mutex::Lock L (m_surface_lock); 
        m_frame = draw_frame;
        m_surface = surface; 
        m_surface_alpha = -0.5;
        m_surface_conn.disconnect ();
        m_surface_conn = signal_timeout().connect (sigc::mem_fun (*this, &MPX::InfoArea::fade_in_surface), 20);
      }

    private:

      void
      insert_layout_and_connect (LayoutID id, TextP & p)
      {
        m_layouts.insert (make_pair (id, p)); 
        p->m_conn = signal_timeout().connect (sigc::bind (sigc::mem_fun (*this, &MPX::InfoArea::fade_in), id), 20);
      }

      void
      remove_layout_if_exists (LayoutID id)
      {
        Layouts::iterator i = m_layouts.find (id);
        if( i != m_layouts.end() )
        {
          TextP & p = i->second;
          m_locks.find (id)->second->lock ();
          p->m_conn.disconnect ();
          m_layouts.erase (i);
          m_locks.find (id)->second->unlock ();
        }
      }

      bool 
      fade_in (LayoutID id)
      {
        Mutex::Lock L (*(m_locks.find (id)->second.get()));
        TextP & p (m_layouts.find (id)->second);
        p->m_alpha += 0.1;
        bool r = p->m_alpha < p->m_target;
        queue_draw ();
        return r;
      }

      bool 
      fade_in_surface ()
      {
        Mutex::Lock L (m_surface_lock);
        m_surface_alpha += 0.1;
        bool r = m_surface_alpha < 1.;
        queue_draw ();
        return r;
      }

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

      void
      play_status_changed ()
      {
        MPXPlaystatus status = MPXPlaystatus (Play::Obj()->property_status().get_value());
        if( status == PLAYSTATUS_PAUSED )
        {
          m_conn_decay = Glib::signal_timeout().connect (sigc::mem_fun (*this, &InfoArea::decay_spectrum), 50); 
        }
        else
        {
          m_conn_decay.disconnect();
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

    protected:

      virtual bool
      on_expose_event (GdkEventExpose * event)
      {
        Widget::on_expose_event (event);
        Cairo::RefPtr<Cairo::Context> cr = get_window ()->create_cairo_context ();

        if( !m_showing_message )
        {
          m_layouts_lock.lock ();  
          for (Layouts::const_iterator i = m_layouts.begin(); i != m_layouts.end(); ++i)
          {
            Mutex::Lock L (*(m_locks.find (i->first)->second.get()));
            TextP const& p (i->second);
            if( p->m_alpha < 0 )
              continue;
            cr->set_source_rgba (1., 1., 1., p->m_alpha);
            cr->set_operator (Cairo::OPERATOR_ATOP);
            cr->move_to (p->m_x, p->m_y);
            p->m_layout->set_single_paragraph_mode (true);
            p->m_layout->set_ellipsize (Pango::ELLIPSIZE_END);
            p->m_layout->set_width ((get_allocation().get_width() - p->m_x - 210) * PANGO_SCALE);
            p->m_layout->set_wrap (Pango::WRAP_CHAR);
            pango_cairo_show_layout (cr->cobj(), p->m_layout->gobj());
          }
          m_layouts_lock.unlock ();  

          m_surface_lock.lock ();
          if( m_surface && (m_surface_alpha >= 0) )
          {
            int offset = (m_frame ? 0 : 4);
    
            cr->set_operator (Cairo::OPERATOR_SOURCE);
            cr->set_source (m_surface, 38 - offset, 12 - offset); 
            cr->rectangle (38 - offset, 12 - offset, 64 + (offset*2), 64 + (offset*2)); 
            cr->save (); 
            cr->clip ();
            cr->paint_with_alpha (m_surface_alpha);
            cr->restore ();

            if( m_frame )
            {
              cr->set_operator (Cairo::OPERATOR_ATOP);
              cr->set_source (m_surface_frame, 34, 8);
              cr->rectangle (34, 8, 72, 72); 
              cr->save (); 
              cr->clip ();
              cr->paint_with_alpha (m_surface_alpha);
              cr->restore ();
            }
          }
          m_surface_lock.unlock ();
        }
        else
        {
          m_layouts_lock.lock ();  
          Pango::Rectangle r = m_layout_message->get_ink_extents();
          cr->set_source_rgba (1., 1., 1., 1.); 
          cr->set_operator (Cairo::OPERATOR_ATOP);
          cr->move_to ( 72, 12 ); 
          pango_cairo_show_layout (cr->cobj(), m_layout_message->gobj());
          m_layouts_lock.unlock ();  
        }

        for (int n = 0; n < SPECT_BANDS; ++n)
        {
          int x = 0, y = 0, w = 0, h = 0;

          // Bar
          x = (get_allocation().get_width ()) - 200 + (n*12);
          y = 10 + (64 - m_spectrum_data[n]); 
          w = 10;
          h = m_spectrum_data[n];

          cr->set_source_rgba (1., 1., 1., 0.3);
          cr->rectangle (x,y,w,h);
          cr->fill ();

          // Peak
          if( m_spectrum_peak[n] > 0 )
          {
            y = 10 + (64 - m_spectrum_peak[n]); 
            h = 1;
            cr->set_source_rgba (1., 1., 1., 0.65);
            cr->rectangle (x, y-1, w, h);
            cr->fill ();
          }
        }

        if( m_source_icon && m_compact )
        {
          cr->set_operator (Cairo::OPERATOR_ATOP);
          Gdk::Cairo::set_source_pixbuf (cr, m_source_icon, get_allocation().get_width() - 28, 12); 
          cr->rectangle (get_allocation().get_width() - 28, 12, 20, 20);
          cr->fill ();
        }

        return true;
      }
  };
}
#endif

namespace MPX
{
    Player::Player(BaseObjectType                 *  obj,
                   RefPtr<Gnome::Glade::Xml> const&  xml)
    : Window      (obj)
    , m_ref_xml   (xml)
    {
  #if 0
      // Infoarea
      {
        m_ref_xml->get_widget_derived ("infoarea", m_infoarea);
    
        m_infoarea->signal_uris_dropped().connect
          ( sigc::mem_fun( *this, &MPX::Player::on_uris_dropped ) );

        m_infoarea->signal_cover_clicked().connect
          ( sigc::mem_fun( *this, &MPX::Player::on_shell_display_track_details) );
      }
  #endif

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
        m_ui_manager = UIManager::create ();
        m_actions = ActionGroup::create ("Actions_Player");
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
    Player::create ()
    {
      const std::string path (build_filename(DATA_DIR, build_filename("glade","mpx.glade")));
      RefPtr<Gnome::Glade::Xml> glade_xml = Gnome::Glade::Xml::create (path);
      Player * p = 0;
      glade_xml->get_widget_derived ("mpx", p);
      return p;
    }

    Player::~Player()
    {
  #if 0
      mmkeys_deactivate ();
      g_object_unref (m_status_icon);
  #endif
    }

    bool
    Player::on_delete_event (GdkEventAny* G_GNUC_UNUSED)
    {
      Gtk::Main::quit();
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
}
