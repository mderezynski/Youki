#include "config.h"

#include <glibmm/i18n.h>
#include <cmath>

#include "mpx/widgets/cairo-extensions.hh"
#include "mpx/i-youki-play.hh"
#include "mpx/i-youki-theme-engine.hh"
#include "mpx/mpx-main.hh"

#include "youki-spectrum.hh"

namespace
{
    struct Color
    {
          guint8 r;
          guint8 g;
          guint8 b;
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

    const int WIDTH = 10 ;
    const int SPACING = 1 ;
    const int HEIGHT = 36;
    const double ALPHA = 0.8 ;
}

namespace MPX
{
    YoukiSpectrum::YoukiSpectrum(
    )
    : m_spectrum_data( SPECT_BANDS, 0 )
    , m_spectrum_peak( SPECT_BANDS, 0 )
    {
        add_events( Gdk::BUTTON_PRESS_MASK ) ;
        set_size_request( -1, 44 ) ;

        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
        const ThemeColor& c = theme->get_color( THEME_COLOR_BASE ) ;
        Gdk::Color cgdk ;
        cgdk.set_rgb_p( c.r, c.g, c.b ) ; 
        modify_bg( Gtk::STATE_NORMAL, cgdk ) ;
        modify_base( Gtk::STATE_NORMAL, cgdk ) ;

        m_play = services->get<IPlay>("mpx-service-play") ; 

        m_play_status = PlayStatus(m_play->property_status().get_value()) ;

        m_play->property_status().signal_changed().connect(
                sigc::mem_fun(
                      *this
                    , &YoukiSpectrum::on_play_status_changed
        )) ;

        m_play->signal_spectrum().connect(
            sigc::mem_fun(
                  *this
                , &YoukiSpectrum::update_spectrum
        )) ;

        Glib::signal_timeout().connect(
            sigc::mem_fun(
                  *this
                , &YoukiSpectrum::redraw_handler
            )
            , 1000./24.
        ) ;
    }

    void
    YoukiSpectrum::on_play_status_changed()
    {
        m_play_status = PlayStatus( m_play->property_status().get_value() ) ;

        if( m_play_status == PLAYSTATUS_STOPPED )
        {
            reset() ;
            queue_draw() ;
        }
    }

    bool
    YoukiSpectrum::redraw_handler()
    {
        if( m_play_status == PLAYSTATUS_PLAYING || m_play_status == PLAYSTATUS_PAUSED )
        {
            if( m_play_status == PLAYSTATUS_PAUSED )
            {
                for( int n = 0; n < SPECT_BANDS; ++n )
                {
                    m_spectrum_data[n] = fmax( m_spectrum_data[n] - 1, -72 ) ; 
                }
            }

            queue_draw () ;
        }

        return true ;
    }

    void
    YoukiSpectrum::reset ()
    {
        std::fill( m_spectrum_data.begin(), m_spectrum_data.end(), 0. ) ;
        std::fill( m_spectrum_peak.begin(), m_spectrum_peak.end(), 0. ) ;
    }

    void
    YoukiSpectrum::update_spectrum(
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
    YoukiSpectrum::on_button_press_event(
        GdkEventButton* G_GNUC_UNUSED 
    )
    {
        m_signal.emit() ;
        return true ;
    }

    bool
    YoukiSpectrum::on_expose_event(
        GdkEventExpose* G_GNUC_UNUSED 
    )
    {
        Cairo::RefPtr<Cairo::Context> cairo = get_window ()->create_cairo_context ();

        draw_spectrum( cairo ) ;

        return true;
    }

    void
    YoukiSpectrum::draw_spectrum(
          Cairo::RefPtr<Cairo::Context>&                cairo
    )
    {
        const Gtk::Allocation& a = get_allocation ();

        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
        const ThemeColor& c_base /* :) */ = theme->get_color( THEME_COLOR_BACKGROUND ) ; 

        cairo->set_operator(
              Cairo::OPERATOR_SOURCE
        ) ;

        cairo->set_source_rgba(
              c_base.r
            , c_base.g
            , c_base.b
            , c_base.a
        ) ;
        cairo->paint () ;

        cairo->set_operator(
              Cairo::OPERATOR_ATOP
        ) ;

        for( int n = 0; n < SPECT_BANDS; ++n ) 
        {
            int   x = 0
                , y = 0
                , w = 0
                , h = 0 ;

            x = a.get_width()/2 - ((WIDTH+SPACING)*SPECT_BANDS)/2 + (WIDTH+SPACING)*n ; 
            w = WIDTH ;

            //// BAR 

            int   bar = m_spectrum_data[n] / 2 ;
            y = - bar ;
            h =   bar + HEIGHT ;

            if( w && h ) 
            {
                cairo->set_source_rgba(
                      double(colors[n/6].r)/255.
                    , double(colors[n/6].g)/255.
                    , double(colors[n/6].b)/255.
                    , ALPHA
                ) ;
                RoundedRectangle(
                      cairo
                    , x
                    , y + 4
                    , w
                    , h
                    , 1.
                ) ;
                cairo->fill ();
            }
        }
    }
}
