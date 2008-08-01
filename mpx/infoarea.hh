#ifndef MPX_INFOAREA_HH
#define MPX_INFOAREA_HH

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include "mpx/widgets/widgetloader.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-string.hh"
#include "mpx/i-playbacksource.hh"
#include "mpx/mpx-types.hh"
#include "play.hh"

namespace MPX
{
  struct TextSet
  {
    std::string     Artist;
    std::string     Album;
    std::string     Title;
    std::string     Genre;
  };

  class InfoArea
    : public Gnome::Glade::WidgetLoader<Gtk::EventBox>
  {
    private:

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

          double cover_anim_area_width;
          double cover_anim_area_height;
          double cover_anim_area_x0;
          double cover_anim_area_y0;
          double cover_anim_area_x1;
          double cover_anim_area_y1;

          double cover_anim_initial_pos;
          double cover_anim_initial_velocity;
          double cover_anim_gravity;
          double cover_anim_wall;
          double cover_anim_wall_elasticity;
          double cover_anim_time_scale;

          int    cover_anim_fps;
          int    cover_anim_interval;
          double cover_anim_dt;

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

          InfoArea(Glib::RefPtr<Gnome::Glade::Xml> const& xml, std::string const&);
          ~InfoArea ()
          {}

    protected:

          bool
          on_button_press_event (GdkEventButton * event);

          bool
          on_button_release_event (GdkEventButton * event);

          bool
          on_drag_drop (Glib::RefPtr<Gdk::DragContext> const& context,
                        int                                   x,
                        int                                   y,
                        guint                                 time);

          void
          on_drag_data_received (Glib::RefPtr<Gdk::DragContext> const& context,
                                 int                                   x,
                                 int                                   y,
                                 Gtk::SelectionData const&             data,
                                 guint                                 info,
                                 guint                                 time);

    private:

          void
          parse_metadata (MPX::Metadata & metadata,
                          TextSet       & set);
          void
          enable_drag_dest ();

          void
          disable_drag_dest ();

          void
          play_status_changed ();

	public:

          void
          reset ();

          void
          set_info ( const Glib::ustring& text );
    
          void
          clear_info ();

          void
          set_metadata( Metadata & metadata );

          void
          set_cover (RefPtr<Gdk::Pixbuf> const& pixbuf);

          void
          set_cover (Cairo::RefPtr<Cairo::ImageSurface> const& surface);

          void
          clear_cover ();

          void
          update_spectrum (Spectrum const& spectrum);

    private:

          bool
          fade_out_layout ();

          bool
          fade_out_cover ();

          bool
          slide_in_cover ();

          void
          draw_background (Cairo::RefPtr<Cairo::Context> & cr);

          void
          draw_cover (Cairo::RefPtr<Cairo::Context> & cr);

          void
          draw_text (Cairo::RefPtr<Cairo::Context> const& cr);

          void
          draw_spectrum (Cairo::RefPtr<Cairo::Context> & cr);

          void
          draw_info (Cairo::RefPtr<Cairo::Context> & cr);

    protected:

          virtual bool
          on_expose_event (GdkEventExpose * event);

    public:

          static InfoArea*
          create(Glib::RefPtr<Gnome::Glade::Xml> & xml, std::string const&);
  };
}


#endif // MPX_INFOAREA_HH
