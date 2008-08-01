#include "config.h"
#include "infoarea.hh"
#include <glibmm.h>

using namespace Glib;
using boost::get;

namespace
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
}

namespace MPX
{
  void
  InfoArea::parse_metadata (MPX::Metadata & metadata,
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
          set.Album = Util::gprintf(
            text_album_artist_f,
            Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM].get())).c_str(),
            Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM_ARTIST].get())).c_str()
          );
        }
        else
        {
          set.Album = Util::gprintf(
            text_artist_f,
            Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM_ARTIST].get())).c_str()
          );
        }
    }
    else
    {
        if( metadata[ATTRIBUTE_ALBUM] )
        {
          set.Album = Util::gprintf(
            text_album_f,
            Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ALBUM].get())).c_str()
          );
        }
    }

    if ((metadata[ATTRIBUTE_ARTIST_SORTNAME] && metadata[ATTRIBUTE_ARTIST]) &&
        (metadata[ATTRIBUTE_ARTIST_SORTNAME] != metadata[ATTRIBUTE_ARTIST])) /* let's display the artist if it's not identical to the sortname */
    {
      set.Artist = Util::gprintf(
        text_b_f2,
        Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST_SORTNAME].get())).c_str(),
        Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST].get())).c_str()
      );
    }
    else
    if( metadata[ATTRIBUTE_ARTIST] )
    {
      set.Artist = Util::gprintf (text_b_f, Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_ARTIST].get())).c_str());
    }

    if( metadata[ATTRIBUTE_TITLE] )
    {
      set.Title = Util::gprintf (text_big_f, Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_TITLE].get())).c_str());
    }

    if( metadata[ATTRIBUTE_GENRE] )
    {
      set.Genre = Util::gprintf (text_i_f, Markup::escape_text (get<std::string>(metadata[ATTRIBUTE_GENRE].get())).c_str());
    }
  }

  void
  InfoArea::enable_drag_dest ()
  {
      disable_drag_dest ();
      std::vector<Gtk::TargetEntry> target_entries;
      target_entries.push_back (Gtk::TargetEntry ("text/plain"));
      drag_dest_set (target_entries, Gtk::DEST_DEFAULT_MOTION);
      drag_dest_add_uri_targets ();
  }

  void
  InfoArea::disable_drag_dest ()
  {
      drag_dest_unset ();
  }

  bool
  InfoArea::fade_out_layout ()
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
  InfoArea::fade_out_cover ()
  {
    Mutex::Lock L (m_surface_lock);

    m_cover_alpha = fmax(m_cover_alpha - 0.1, 0.);
    bool cond = m_cover_alpha >= 0.2;
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
  InfoArea::slide_in_cover ()
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
        m_cover_velocity *= - (cover_anim_wall_elasticity + g_random_double_range(0.0, 0.08));

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
  InfoArea::draw_background (Cairo::RefPtr<Cairo::Context> & cr)
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
  InfoArea::draw_cover (Cairo::RefPtr<Cairo::Context> & cr)
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
          Util::draw_cairo_image (cr, m_cover_surface_new.get(), m_cover_pos + (m_pressed?1:0), y + (m_pressed?1:0), 1.);
          cr->restore ();
        }

        cr->save ();

        double wh = 72 - (15*m_cover_alpha);

        cr->save ();
        cr->rectangle (cover_anim_area_x0+(m_pressed?1:0),
                       cover_anim_area_y0+(m_pressed?1:0), cover_anim_area_width+(m_pressed?1:0),
                                                           cover_anim_area_height+(m_pressed?1:0));
        cr->clip ();

        //cr->rotate((2*M_PI)*(1. - m_cover_alpha));
        cr->scale(wh/72., wh/72.);
        Util::draw_cairo_image(
            cr,
            m_cover_surface_cur.get(),
            (cover_anim_area_x0 + ((72.-wh)/2.)) + pow((1./m_cover_alpha),9),
            (cover_anim_area_y0 + ((72.-wh)/2.)),
            m_cover_alpha/2.
        );
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

          Util::draw_cairo_image (cr, m_cover_surface_cur.get(), m_cover_pos + (m_pressed?1:0), y + (m_pressed?1:0), m_cover_alpha);

          cr->restore ();
    }
  }

  void
  InfoArea::draw_text (Cairo::RefPtr<Cairo::Context> const& cr)
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
  InfoArea::draw_spectrum (Cairo::RefPtr<Cairo::Context> & cr)
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
  InfoArea::draw_info (Cairo::RefPtr<Cairo::Context> & cr)
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
}
