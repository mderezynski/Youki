#ifndef MPX_INFOAREA_HH
#define MPX_INFOAREA_HH

#include "mpx/widgets/widgetloader.hh"
#include "mpx/util-file.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-string.hh"
#include "mpx/i-playbacksource.hh"
#include "mpx/mpx-types.hh"
#include "play.hh"

#include <boost/optional.hpp>
#include <gtkmm.h>
#include <cairomm/cairomm.h>

namespace MPX
{
  typedef std::vector<std::string> TextSet; 
  /*
  struct TextSet
  {
    std::string     Artist;
    std::string     Album;
    std::string     Title;
    std::string     Genre;
  };*/

  class InfoArea
    : public Gnome::Glade::WidgetLoader<Gtk::EventBox>
  {
    private:

          Spectrum                              m_spectrum_data, m_spectrum_peak;
          boost::optional<Glib::ustring>        m_info_text;
          boost::optional<TextSet>              m_text_cur;
          boost::optional<TextSet>              m_text_new;
          boost::optional<Cairo::RefPtr<Cairo::ImageSurface> >   m_cover_surface_cur;
          boost::optional<Cairo::RefPtr<Cairo::ImageSurface> >   m_cover_surface_new;
          double                                m_cover_pos;
          double                                m_cover_velocity;
          double                                m_cover_accel;
          double                                m_cover_alpha;
          double                                m_layout_alpha[3];
          sigc::connection                      m_cover_anim_conn_fade;
          sigc::connection                      m_cover_anim_conn_slide;
          sigc::connection                      m_decay_conn;
          sigc::connection                      m_fade_conn[3];

          bool                                  m_pressed;
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

          Pango::Rectangle layout_extents[3];

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
          set_info(
            const Glib::ustring&
          );

          void
          clear_info ();

          void
          set_metadata(
            Metadata&,
            bool
          );

          void
          set_cover(
            const Glib::RefPtr<Gdk::Pixbuf>&,
            bool
          );

          void
          set_cover(
            const Cairo::RefPtr<Cairo::ImageSurface>&,
            bool
          );

          void
          clear_cover ();

          void
          update_spectrum(
            const Spectrum&
          );

    private:

          bool
          fade_out_layout(int);

          bool
          fade_out_cover ();

          bool
          slide_in_cover ();

          void
          draw_background(Cairo::RefPtr<Cairo::Context>&);

          void
          draw_cover(Cairo::RefPtr<Cairo::Context>&);

          void
          draw_text(Cairo::RefPtr<Cairo::Context>&);

          void
          draw_spectrum(Cairo::RefPtr<Cairo::Context>&);

          void
          draw_info(Cairo::RefPtr<Cairo::Context>&);

    protected:

          virtual bool
          on_expose_event (GdkEventExpose * event);

    public:

          static InfoArea*
          create(const Glib::RefPtr<Gnome::Glade::Xml>&, const std::string&);
  };
}


#endif // MPX_INFOAREA_HH
