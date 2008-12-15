#include "config.h"
#include "infoarea.hh"
#include <glibmm/i18n.h>
#include <cmath>

namespace MPX
{
        InfoArea*
                InfoArea::create(
                                const Glib::RefPtr<Gnome::Glade::Xml>&      xml,
                                const std::string&                          name
                                )
                {
                        return new InfoArea(xml, name);
                } 

        InfoArea::InfoArea(
                        const Glib::RefPtr<Gnome::Glade::Xml>&      xml,
                        const std::string&                          name
        )
        : Gnome::Glade::WidgetLoader<Gtk::EventBox>(xml, name)
        , m_spectrum_data(SPECT_BANDS, 0)
        , m_spectrum_peak(SPECT_BANDS, 0)
        , m_cover_alpha(0.)
        , m_pressed(false)
        , cover_anim_area_width(80.0)
        , cover_anim_area_height(80.0)
        , cover_anim_area_x0(2.0)
        , cover_anim_area_y0(2.0)
        , cover_anim_area_x1(cover_anim_area_x0 + cover_anim_area_width)
        , cover_anim_area_y1(cover_anim_area_y0 + cover_anim_area_height)
        , cover_anim_initial_pos(3.0 - 80.0)
        , cover_anim_initial_velocity(473.3)
        , cover_anim_gravity(359.1)
        , cover_anim_wall(cover_anim_area_x1)
        , cover_anim_wall_elasticity(0.11) //0.120;
        , cover_anim_time_scale(1.0)
        , cover_anim_fps(25)
        , cover_anim_interval(1000 / cover_anim_fps)
        , cover_anim_dt(InfoArea::cover_anim_time_scale / InfoArea::cover_anim_fps)
        {
            /*
            gtk_widget_realize(GTK_WIDGET(gobj()));

            m_layout_dot = create_pango_layout ("");
            m_layout_dot->set_markup("<small>●</small>");
            Pango::Rectangle Ink;
            m_layout_dot->get_pixel_extents(Ink, layout_extents[4]); 
            */

            m_layout_alpha[0] = 0;
            m_layout_alpha[1] = 0;
            m_layout_alpha[2] = 0;

            add_events (Gdk::BUTTON_PRESS_MASK);

            enable_drag_dest ();

            set_tooltip_text(_("Drag and drop files here to play them."));
        }

  void
          InfoArea::reset ()
          {
                  std::fill (m_spectrum_data.begin (), m_spectrum_data.end (), 0.);
                  std::fill (m_spectrum_peak.begin (), m_spectrum_peak.end (), 0.);

                  m_cover_surface_new.reset();
                  m_cover_surface_cur.reset();
                  m_text_new.reset();
                  m_text_cur.reset();

                  m_layout_alpha[0] = 1.;
                  m_layout_alpha[1] = 1.;
                  m_layout_alpha[2] = 1.;
                  m_cover_alpha = 1.;

                  m_fade_conn[0].disconnect();
                  m_fade_conn[1].disconnect();
                  m_fade_conn[2].disconnect();

                  m_decay_conn.disconnect();

                  m_cover_anim_conn_fade.disconnect ();
                  m_cover_anim_conn_slide.disconnect ();

                  m_info_text.reset();

                  queue_draw ();
          }

  void
          InfoArea::set_info ( const Glib::ustring& text )
          {
                  Glib::Mutex::Lock L (m_info_lock);

                  m_info_text = text;
                  queue_draw ();
          }

  void
          InfoArea::clear_info ()
          {
                  Glib::Mutex::Lock L (m_info_lock);

                  m_info_text.reset();
                  queue_draw ();
          }

  void
          InfoArea::set_metadata( Metadata & metadata, bool first )
          {
                  Glib::Mutex::Lock L (m_layout_lock);

                  if( metadata.Image )
                  {
                          set_cover (metadata.Image->scale_simple (80, 80, Gdk::INTERP_BILINEAR), first);
                  }
                  else if( first )
                  {
                          clear_cover ();
                  }

                  TextSet set (5);
                  parse_metadata( metadata, set ); 

                  if( m_text_cur )
                  {
                          m_fade_conn[0].disconnect();
                          m_fade_conn[1].disconnect();
                          m_fade_conn[2].disconnect();

                          m_text_new = set;

                          for( int n = 0; n < 3; ++n )
                          {
                                  if( m_text_new && m_text_cur && m_text_new.get()[n] != m_text_cur.get()[n] )

                                            m_fade_conn[n] = Glib::signal_timeout().connect(
                                                    sigc::bind(
                                                            sigc::mem_fun(  *this,
                                                                        &InfoArea::fade_out_layout
                                                            ),
                                                            n
                                                    ),
                                                    15
                                            );
                                  else
                                            m_layout_alpha[n] = 1.;
                          }
                  }
                  else
                  {
                          m_text_cur = set;
                          m_layout_alpha[0] = 1.;
                          m_layout_alpha[1] = 1.;
                          m_layout_alpha[2] = 1.;
                          queue_draw();
                  }
          }

  void
          InfoArea::set_cover(
                          const Glib::RefPtr<Gdk::Pixbuf>& pixbuf,
                          bool first
                          )
          {
                  if( pixbuf ) 
                  {
                          Gdk::Color c = get_style()->get_base(Gtk::STATE_SELECTED);
                          Cairo::RefPtr<Cairo::ImageSurface> surface = Util::cairo_image_surface_from_pixbuf (pixbuf);
                          //surface = Util::cairo_image_surface_round(surface, 3.5);
                          set_cover( surface, first );
                  }
                  else
                          if( first )
                          {
                                  clear_cover ();
                          }
          }

  void
          InfoArea::set_cover(
                          const Cairo::RefPtr<Cairo::ImageSurface>& surface,
                          bool first
                          )
          {
                  Glib::Mutex::Lock L (m_surface_lock);

                  if( !surface && !first )
                  {
                          return;
                  }

                  m_cover_pos           = cover_anim_initial_pos;
                  m_cover_velocity      = cover_anim_initial_velocity;
                  m_cover_accel         = cover_anim_gravity;
                  m_cover_alpha         = 1.;

                  if(!m_cover_surface_cur)
                  {
                          m_cover_surface_cur = surface; 
                          m_cover_anim_conn_slide.disconnect ();
                          m_cover_anim_conn_slide = Glib::signal_timeout ().connect(
                                          sigc::mem_fun( *this, &InfoArea::slide_in_cover ), cover_anim_interval
                                          );
                  }
                  else
                  {
                          m_cover_surface_new = surface;
                          m_cover_anim_conn_slide.disconnect ();

                          if( !m_cover_anim_conn_fade )
                          {
                                  m_cover_anim_conn_fade = Glib::signal_timeout ().connect(
                                                  sigc::mem_fun( *this, &InfoArea::fade_out_cover ), 25 
                                                  );
                          }
                  }
          }

  void
          InfoArea::clear_cover ()
          {
                  if( m_cover_surface_cur )
                  {
                          m_cover_surface_new.reset();
                          m_cover_anim_conn_fade.disconnect ();
                          m_cover_anim_conn_fade = Glib::signal_timeout ().connect(
                                          sigc::mem_fun( *this, &InfoArea::fade_out_cover ), 25
                                          );
                  }
          }

  void
          InfoArea::update_spectrum (Spectrum const& spectrum)
          {
                  for( int n = 0; n < SPECT_BANDS; ++n )
                  {
                          if( fabs(spectrum[n] - m_spectrum_data[n]) <= 2)
                          {
                                  /* do nothing */
                          }
                          else if( spectrum[n] > m_spectrum_data[n] )
                                  m_spectrum_data[n] = spectrum[n];
                          else
                                  m_spectrum_data[n] = fmin(m_spectrum_data[n] - 2, 0);
                  }

                  for( int n = 0; n < SPECT_BANDS; ++n )
                  {
                          if( spectrum[n] < m_spectrum_peak[n] ) 
                                  m_spectrum_peak[n] = fmin(m_spectrum_peak[n] - 0.5, 0);
                          else if( spectrum[n] == m_spectrum_peak[n] ) 
                                  m_spectrum_peak[n] = fmin(m_spectrum_peak[n] + 2.0, 72);
                          else
                                  m_spectrum_peak[n] = spectrum[n];
                  }
                  queue_draw ();
          }
}
