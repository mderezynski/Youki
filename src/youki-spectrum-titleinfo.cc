#include "config.h"

#include <glibmm/i18n.h>
#include <cmath>

#include "mpx/util-graphics.hh"
#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/i-youki-play.hh"
#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-main.hh"

#include "youki-spectrum-titleinfo.hh"

namespace
{
    // Spectrum Constants

    struct Color
    {
          guint8 r;
          guint8 g;
          guint8 b;
    };

    Color colors[] =
    {
          { 0xff, 0xb1, 0x6f },
          { 0xff, 0xcf, 0x7e },
          { 0xf6, 0xe6, 0x99 },
          { 0xf1, 0xfc, 0xd4 },
          { 0xbd, 0xd8, 0xab },
          { 0xcd, 0xe6, 0xd0 },
          { 0xce, 0xe0, 0xea },
          { 0xd5, 0xdd, 0xea },
          { 0xee, 0xc1, 0xc8 },
          { 0xee, 0xaa, 0xb7 },
    };

    const int       WIDTH = 10 ;
    const int       SPACING = 1 ;
    const int       HEIGHT = 36;
    const double    ALPHA = 1. ; 

    // Text Animation Settings

    int const animation_fps = 24;
    int const animation_frame_period_ms = 1000 / animation_fps;

    int const    text_size_px           = 14 ;
    double const text_fade_in_time      = 0.2 ;
    double const text_fade_out_time     = 0.05 ;
    double const text_hold_time         = 5. ; 
    double const text_time              = text_fade_in_time + text_fade_out_time + text_hold_time;
    double const text_full_alpha        = 0.90 ;
    double const initial_delay          = 0.0 ;

}

namespace MPX
{
    inline double
    YoukiSpectrumTitleinfo::cos_smooth (double x)
    {
        return (1.0 - std::cos (x * G_PI)) / 2.0;
    }

    std::string
    YoukiSpectrumTitleinfo::get_text_at_time ()
    {
        if( !m_info.empty() )
        {
            unsigned int line = std::fmod( ( m_current_time / text_time ), m_info.size() ) ;
            return m_info[line];
        }
        else
        {
            return "" ;
        }
    }

    double
    YoukiSpectrumTitleinfo::get_text_alpha_at_time ()
    {
        {
            double offset = m_tmod ; 

            if (offset < text_fade_in_time)
            {
                return text_full_alpha * cos_smooth (offset / text_fade_in_time);
            }
            else if (offset < text_fade_in_time + text_hold_time)
            {
                return text_full_alpha;
            }
            else
            {
                return text_full_alpha * cos_smooth (1.0 - (offset - text_fade_in_time - text_hold_time) / text_fade_out_time);
            }
        }
    }

    void
    YoukiSpectrumTitleinfo::clear ()
    {
        m_info.clear() ;
        m_timer.stop () ;
        m_timer.reset () ;
        m_cover.reset () ;
        m_update_connection.disconnect () ;

        queue_draw () ;
    }

    void
    YoukiSpectrumTitleinfo::set_info(
          const std::vector<std::string>&   i
        , Glib::RefPtr<Gdk::Pixbuf>         cover
    )
    {
        m_info = i ;
        m_cover = cover->scale_simple( 66, 66, Gdk::INTERP_BILINEAR ) ;

        total_animation_time    = m_info.size() * text_time;
        start_time              = initial_delay;
        end_time                = start_time + total_animation_time;

        m_timer.reset () ;

        if( !m_update_connection )
        {
            m_timer.start ();
            m_update_connection = Glib::signal_timeout().connect(
                sigc::mem_fun(
                      *this
                    , &YoukiSpectrumTitleinfo::update_frame
                    )
                , animation_frame_period_ms
            ) ;
        }
    }

    bool
    YoukiSpectrumTitleinfo::update_frame ()
    {
        queue_draw ();
        return true;
    }

    ///////////////////////////////////

    YoukiSpectrumTitleinfo::YoukiSpectrumTitleinfo(
    )
    : m_spectrum_data( SPECT_BANDS, 0 )
    , m_spectrum_peak( SPECT_BANDS, 0 )
    , m_mode( SPECTRUM_MODE_VOCODER )
    , m_tmod( m_current_time, text_time )
    {
        add_events( Gdk::BUTTON_PRESS_MASK ) ;
        set_size_request( -1, 78 ) ;

        m_timer.stop ();
        m_timer.reset ();

        m_theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;

        m_play = services->get<IPlay>("mpx-service-play") ; 

        m_play_status = PlayStatus(m_play->property_status().get_value()) ;

        m_play->property_status().signal_changed().connect(
                sigc::mem_fun(
                      *this
                    , &YoukiSpectrumTitleinfo::on_play_status_changed
        )) ;

        m_play->signal_spectrum().connect(
            sigc::mem_fun(
                  *this
                , &YoukiSpectrumTitleinfo::update_spectrum
        )) ;

        Glib::signal_timeout().connect(
            sigc::mem_fun(
                  *this
                , &YoukiSpectrumTitleinfo::redraw_handler
            )
            , 1000./24.
        ) ;
    }

    void
    YoukiSpectrumTitleinfo::on_play_status_changed()
    {
        m_play_status = PlayStatus( m_play->property_status().get_value() ) ;

        if( m_play_status == PLAYSTATUS_STOPPED )
        {
            reset() ;
            queue_draw() ;
        }
    }

    bool
    YoukiSpectrumTitleinfo::redraw_handler()
    {
        if( m_play_status == PLAYSTATUS_PLAYING || m_play_status == PLAYSTATUS_PAUSED )
        {
            if( m_play_status == PLAYSTATUS_PAUSED )
            {
                for( int n = 0; n < SPECT_BANDS; ++n )
                {
                    m_spectrum_data[n] = fmax( m_spectrum_data[n] - 2, -72 ) ; 
                }
            }

            queue_draw () ;
        }

        return true ;
    }

    void
    YoukiSpectrumTitleinfo::reset ()
    {
        std::fill( m_spectrum_data.begin(), m_spectrum_data.end(), 0. ) ;
        std::fill( m_spectrum_peak.begin(), m_spectrum_peak.end(), 0. ) ;
    }

    void
    YoukiSpectrumTitleinfo::update_spectrum(
        const Spectrum& spectrum
    )
    {
        if( m_play_status == PLAYSTATUS_PLAYING )
        {
            for( int n = 0; n < SPECT_BANDS; ++n )
            {
                if( fabs(spectrum[n] - m_spectrum_data[n]) <= 2)
                {
                        /* do nothing */
                }

                else if( spectrum[n] > m_spectrum_data[n] )
                        m_spectrum_data[n] = spectrum[n] ;
                else
                        m_spectrum_data[n] = fmin( m_spectrum_data[n] - 2, 0 ) ;
            }
        }
    }

    bool
    YoukiSpectrumTitleinfo::on_button_press_event(
        GdkEventButton* G_GNUC_UNUSED 
    )
    {
        m_mode = Mode( !m_mode ) ; 

        return false ;
    }

    bool
    YoukiSpectrumTitleinfo::on_expose_event(
        GdkEventExpose* G_GNUC_UNUSED 
    )
    {
        Cairo::RefPtr<Cairo::Context> cairo = get_window ()->create_cairo_context ();

        draw_background( cairo ) ;
        draw_spectrum( cairo ) ;
        draw_titleinfo( cairo ) ;
        draw_cover( cairo ) ;

        return true;
    }

    void
    YoukiSpectrumTitleinfo::draw_background(
          Cairo::RefPtr<Cairo::Context>&                cairo
    )
    {
        cairo->save() ;

        const Gtk::Allocation& a = get_allocation ();

        const ThemeColor& c_base = m_theme->get_color( THEME_COLOR_BACKGROUND ) ; // all hail to the C-Base!
        const ThemeColor& c_info = m_theme->get_color( THEME_COLOR_INFO_AREA ) ; 

        GdkRectangle r ;
        r.x = 1 ;
        r.y = 3 ;
        r.width = a.get_width() - 2 ;
        r.height = a.get_height() - 2 - 4 ;

        cairo->set_operator(Cairo::OPERATOR_SOURCE) ;
        cairo->set_source_rgba(
              c_base.r
            , c_base.g
            , c_base.b
            , c_base.a
        ) ;
        cairo->paint () ;

        cairo->set_operator( Cairo::OPERATOR_OVER ) ;
        cairo->set_source_rgba(
              c_info.r 
            , c_info.g
            , c_info.b
            , c_info.a
        ) ;
        RoundedRectangle(
              cairo
            , r.x 
            , r.y
            , r.width 
            , r.height 
            , 4. 
        ) ;
        cairo->fill () ;

        Gdk::Color cgdk ;
        cgdk.set_rgb_p( 0.25, 0.25, 0.25 ) ; 

        Cairo::RefPtr<Cairo::LinearGradient> gradient = Cairo::LinearGradient::create(
              r.x + r.width / 2
            , r.y  
            , r.x + r.width / 2
            , r.y + r.height
        ) ;

        double h, s, b ;
    
        double alpha = 1. ; 
        
        Util::color_to_hsb( cgdk, h, s, b ) ;
        b *= 1.05 ; 
        s *= 0.55 ; 
        Gdk::Color c1 = Util::color_from_hsb( h, s, b ) ;

        Util::color_to_hsb( cgdk, h, s, b ) ;
        s *= 0.55 ; 
        Gdk::Color c2 = Util::color_from_hsb( h, s, b ) ;

        Util::color_to_hsb( cgdk, h, s, b ) ;
        b *= 0.9 ; 
        s *= 0.60 ;
        Gdk::Color c3 = Util::color_from_hsb( h, s, b ) ;

        gradient->add_color_stop_rgba(
              0
            , c1.get_red_p()
            , c1.get_green_p()
            , c1.get_blue_p()
            , alpha / 1.05
        ) ;
        gradient->add_color_stop_rgba(
              .20
            , c2.get_red_p()
            , c2.get_green_p()
            , c2.get_blue_p()
            , alpha / 1.05
        ) ;
        gradient->add_color_stop_rgba(
              1. 
            , c3.get_red_p()
            , c3.get_green_p()
            , c3.get_blue_p()
            , alpha
        ) ;
        cairo->set_source( gradient ) ;
        cairo->set_operator( Cairo::OPERATOR_OVER ) ;
        RoundedRectangle(
              cairo
            , r.x 
            , r.y 
            , r.width 
            , r.height 
            , 4. 
        ) ;
        cairo->fill(); 
/*
        std::valarray<double> dashes ( 3 ) ;
        dashes[0] = 0. ;
        dashes[1] = 1. ;
        dashes[2] = 0. ;
        cairo->set_line_width(
              .75
        ) ;
        cairo->set_dash(
              dashes
            , 0 
        ) ;
        cairo->move_to( r.x + 6 + 66 , 30 ) ;
        cairo->line_to( r.width - 4, 30 ) ;
        cairo->set_source_rgba( 1., 1., 1., 0.75 ) ; 
        cairo->stroke(); 
        cairo->unset_dash() ; 
*/

        gradient = Cairo::LinearGradient::create(
              r.x + r.width / 2
            , r.y + 3 
            , r.x + r.width / 2
            , 24
        ) ;

        gradient->add_color_stop_rgba(
              0
            , .45 
            , .45
            , .45
            , 1. 
        ) ;
        gradient->add_color_stop_rgba(
              .25 
            , .40 
            , .40
            , .40
            , 1. 
        ) ;
        gradient->add_color_stop_rgba(
              1.
            , .35 
            , .35
            , .35
            , 1. 
        ) ;
        cairo->set_source( gradient ) ;

        RoundedRectangle(
              cairo
            , r.x + 74 
            , r.y + 3
            , r.width - 76 
            , 24
            , 4. 
        ) ;
        cairo->fill_preserve() ;
        cairo->set_source_rgba( 0.35, 0.35, 0.35, 1. ) ; 
        cairo->set_line_width( 0.75 ) ; 
        cairo->stroke() ;

        RoundedRectangle(
              cairo
            , r.x 
            , r.y 
            , r.width 
            , r.height 
            , 4. 
        ) ;
        cairo->set_source_rgba( 0.20, 0.20, 0.20, 1. ) ; 
        cairo->set_line_width( 1. ) ; 
        cairo->stroke() ;

        cairo->restore() ;
    }

    void
    YoukiSpectrumTitleinfo::draw_titleinfo(
          Cairo::RefPtr<Cairo::Context>&                cairo
    )
    {
        cairo->save() ;

        const Gtk::Allocation& a = get_allocation ();

        m_current_time = m_timer.elapsed () ;

        int text_size_pt = static_cast<int>( (text_size_px * 72) / Util::screen_get_y_resolution( Gdk::Screen::get_default() )) ;

        Pango::FontDescription font_desc = get_style()->get_font() ;
        font_desc.set_size( text_size_pt * PANGO_SCALE ) ;
        font_desc.set_weight( Pango::WEIGHT_BOLD ) ;

        std::string text  = get_text_at_time() ;
        double      alpha = get_text_alpha_at_time() ;

        Glib::RefPtr<Pango::Layout> layout = Glib::wrap( pango_cairo_create_layout( cairo->cobj() )) ;

        layout->set_font_description( font_desc ) ;
        layout->set_text( text ) ;

        int width, height;
        layout->get_pixel_size( width, height ) ;

        cairo->set_operator( Cairo::OPERATOR_OVER ) ;

        cairo->move_to(
              (a.get_width() - width) / 2
            , 7 
        ) ;

        const ThemeColor& c_text = m_theme->get_color( THEME_COLOR_TEXT_SELECTED ) ; 

        Gdk::Color cgdk ;
        cgdk.set_rgb_p( c_text.r, c_text.g, c_text.b ) ;

        pango_cairo_layout_path( cairo->cobj(), layout->gobj() ) ;

        cairo->set_source_rgba(
              c_text.r 
            , c_text.g 
            , c_text.b 
            , alpha
        ) ; 
        cairo->fill_preserve() ;

        double h,s,b ;
        Util::color_to_hsb( cgdk, h, s, b ) ;
        b *= 0.7 ;
        s *= 0.75 ;
        Gdk::Color c1 = Util::color_from_hsb( h, s, b ) ;

        cairo->set_source_rgba(
              c1.get_red_p() 
            , c1.get_green_p() 
            , c1.get_blue_p() 
            , alpha
        ) ; 
        cairo->set_line_width( 0.5 ) ;
        cairo->stroke() ;

        cairo->restore() ;
    }

    void
    YoukiSpectrumTitleinfo::draw_spectrum(
          Cairo::RefPtr<Cairo::Context>&                cairo
    )
    {
        cairo->save() ;

        const Gtk::Allocation& a = get_allocation ();

        for( int n = 0; n < 56 ; ++n ) 
        {
            int   x = 0
                , y = 0
                , w = 0
                , h = 0 ;

            x = a.get_width()/2 - ((WIDTH+SPACING)*56)/2 + (WIDTH+SPACING)*n ; 
            w = WIDTH ;

            //// BAR 
        
            if( m_mode == SPECTRUM_MODE_VOCODER )
            {
                int   bar = HEIGHT + (m_spectrum_data[n] / 2) ;
                bar = (bar < 2) ? 2 : bar ;
                y   = (( HEIGHT + bar ) / 2) ; 
                h   = - bar ; 

                if( w && h ) 
                {
                    cairo->set_source_rgba(
                          double(colors[n/6].r)/255.
                        , double(colors[n/6].g)/255.
                        , double(colors[n/6].b)/255.
                        , ALPHA - (0.65 - ((0.65 * (1.0 - std::cos ((bar/36.) * G_PI)) / 1.5)))
                    ) ;
/*
                    RoundedRectangle(
                          cairo
                        , x - 50
                        , (HEIGHT-y) + 7 
                        , w
                        , - h
                        , 1.
                    ) ;
                    cairo->fill ();
*/

/*
                    cairo->set_source_rgba(
                          1. 
                        , 1. 
                        , 1. 
                        , ALPHA - (0.65 - ((0.65 * (1.0 - std::cos ((bar/36.) * G_PI)) / 1.5)))
                    ) ;
*/
                    RoundedRectangle(
                          cairo
                        , x
                        , (HEIGHT-y) + 7 + 28 
                        , w
                        , - h
                        , 1.
                    ) ;
                    cairo->set_line_width( 0.75 ) ;
                    cairo->stroke ();
                }
            }
            else
            {
                int   bar = m_spectrum_data[n] / 2 ;
                y = - bar ;
                h =   bar + HEIGHT ;

                if( w && h ) 
                {
                    cairo->set_source_rgba(
                          double(colors[n/6].r)/255.
                        , double(colors[n/6].g)/255.
                        , double(colors[n/6].b)/255.
                        , ALPHA * 0.9
                    ) ;
                    RoundedRectangle(
                          cairo
                        , x
                        , y + 7 + 28 
                        , w
                        , h
                        , 1.
                    ) ;
                    cairo->fill ();
                }
            }
        }

        cairo->restore() ;
    }

    void
    YoukiSpectrumTitleinfo::draw_cover(
          Cairo::RefPtr<Cairo::Context>&                cairo
    )
    {
        cairo->save() ;

        if( m_cover )
        {
            GdkRectangle r ;

            r.x = 4 ; 
            r.y = 6 ;
            r.width = 66 ; 
            r.height = 66 ; 

            Gdk::Cairo::set_source_pixbuf(
                  cairo
                , m_cover
                , r.x
                , r.y
            ) ;

            RoundedRectangle(
                  cairo
                , r.x 
                , r.y 
                , r.width 
                , r.height 
                , 4. 
            ) ;

            cairo->clip() ;
            cairo->paint_with_alpha( 0.90 ) ;
            cairo->reset_clip() ;

            cairo->set_line_width( 1. ) ; 

            RoundedRectangle(
                  cairo
                , r.x 
                , r.y 
                , r.width 
                , r.height 
                , 4. 
            ) ;

            cairo->set_source_rgba( 0.35, 0.35, 0.35, 1. ) ; 
            cairo->stroke(); 

            RoundedRectangle(
                  cairo
                , r.x + 1
                , r.y + 1
                , r.width - 2 
                , r.height - 2
                , 4. 
            ) ;

            cairo->set_source_rgba( 0.40, 0.40, 0.40, 1. ) ; 
            cairo->stroke(); 
        }

        cairo->restore() ;
    }
}
