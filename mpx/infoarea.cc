#include "config.h"
#include "infoarea.hh"
#include <glibmm/i18n.h>

namespace MPX
{
  InfoArea*
  InfoArea::create(MPX::Play & play, Glib::RefPtr<Gnome::Glade::Xml> & xml)
  {
       return new InfoArea(play,xml);
  } 

  InfoArea::InfoArea (MPX::Play & play,
            Glib::RefPtr<Gnome::Glade::Xml> const& xml)
  : Gnome::Glade::WidgetLoader<Gtk::EventBox>(xml, "infoarea")
  , m_Play(play)
  , m_spectrum_data(SPECT_BANDS, 0)
  , m_cover_alpha(0.)
  , m_layout_alpha(0.)
  , m_pressed(false)
  , cover_anim_area_width(72.0)
  , cover_anim_area_height(72.0)
  , cover_anim_area_x0(6.0)
  , cover_anim_area_y0(6.0)
  , cover_anim_area_x1(cover_anim_area_x0 + cover_anim_area_width)
  , cover_anim_area_y1(cover_anim_area_y0 + cover_anim_area_height)
  , cover_anim_initial_pos(6.0 - 72.0)
  , cover_anim_initial_velocity(323.3)
  , cover_anim_gravity(359.1)
  , cover_anim_wall(cover_anim_area_x1)
  , cover_anim_wall_elasticity(0.120) //0.074;
  , cover_anim_time_scale(1.0)
  , cover_anim_fps(25)
  , cover_anim_interval(1000 / cover_anim_fps)
  , cover_anim_dt(InfoArea::cover_anim_time_scale / InfoArea::cover_anim_fps)
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

  void
  InfoArea::reset ()
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
  InfoArea::set_info ( const Glib::ustring& text )
  {
    Mutex::Lock L (m_info_lock);

    m_info_text = text;
    queue_draw ();
  }

  void
  InfoArea::clear_info ()
  {
    Mutex::Lock L (m_info_lock);

    m_info_text.reset();
    queue_draw ();
  }

  void
  InfoArea::set_metadata( Metadata & metadata )
  {
    Mutex::Lock L (m_layout_lock);

    if( metadata.Image )
    {
        set_cover (metadata.Image->scale_simple (72, 72, Gdk::INTERP_BILINEAR));
    }
    else
    {
        clear_cover ();
    }

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
  InfoArea::set_cover (RefPtr<Gdk::Pixbuf> const& pixbuf)
  {
    set_cover (Util::cairo_image_surface_round(Util::cairo_image_surface_from_pixbuf (pixbuf), 6.));
  }

  void
  InfoArea::set_cover (Cairo::RefPtr<Cairo::ImageSurface> const& surface)
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
  InfoArea::clear_cover ()
  {
        m_cover_surface_new.reset();
        m_cover_anim_conn_fade.disconnect ();
        m_cover_anim_conn_fade = signal_timeout ().connect(
            sigc::mem_fun( *this, &InfoArea::fade_out_cover ), 10
        );
  }
}
