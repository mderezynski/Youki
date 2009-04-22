#include "config.h"

#include <clutter-gtk/clutter-gtk.h>
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

    const int WIDTH = 8 ;
    const int SPACING = 1 ;
    const int HEIGHT = 36;
    const int ALPHA = 200 ;
}

namespace MPX
{
    YoukiSpectrum::YoukiSpectrum(
    )
    : m_spectrum_data( SPECT_BANDS, 0 )
    , m_spectrum_peak( SPECT_BANDS, 0 )
    {
        add_events( Gdk::EventMask( Gdk::BUTTON_PRESS_MASK )) ;
        set_size_request( -1, 44 ) ;

        boost::shared_ptr<IYoukiThemeEngine> theme = services->get<IYoukiThemeEngine>("mpx-service-theme") ;
        const ThemeColor& c = theme->get_color( THEME_COLOR_BASE ) ;
        m_stage = get_stage() ;
        m_stage->set_color( Clutter::Color( c.r * 255, c.g * 255, c.b * 255 ) ) ;

        m_group_peaks = Clutter::Group::create() ;
        m_group_bars = Clutter::Group::create() ;

        m_stage->add_actor(m_group_peaks) ;
        m_stage->add_actor(m_group_bars) ;

        for( int n=0; n < SPECT_BANDS; n++ )
        {
            int   x = 0
                , y = 0
                , w = 0
                , h = 0 ;

            x = m_stage->get_width()/2 - ((WIDTH+SPACING)*SPECT_BANDS)/2 + (WIDTH+SPACING)*n ; 
            w = WIDTH ;

            //// PEAK 

            int  peak = m_spectrum_peak[n] / 2 ; 
            y = -peak ; 
            h =  peak + HEIGHT ;

            if( w && h ) 
            {
                Clutter::Color color(
                      255
                    , 255
                    , 255
                    , 25
                ) ;
                Glib::RefPtr<Clutter::Rectangle> rect = Clutter::Rectangle::create( color ) ;
                m_group_peaks->add_actor( rect ) ;
                rect->set_position( x, y + 4 ) ;
                rect->set_size( w, h ) ;
            }

            //// BAR 

            int   bar = m_spectrum_data[n] / 2 ;
            y = - bar ;
            h =   bar + HEIGHT ;

            if( w && h ) 
            {
                Clutter::Color color(
                      double(colors[n/6].r)
                    , double(colors[n/6].g)
                    , double(colors[n/6].b)
                    , ALPHA
                ) ;
                Glib::RefPtr<Clutter::Rectangle> rect = Clutter::Rectangle::create( color ) ;
                m_group_bars->add_actor( rect ) ;
                rect->set_position( x, y + 4 ) ;
                rect->set_size( w, h ) ;
            }
        }

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

    }

    void
    YoukiSpectrum::on_size_allocate(
          Gtk::Allocation& a
    )
    {
        Clutter::Gtk::Embed::on_size_allocate( a ) ;
        redraw() ;
    }

    void
    YoukiSpectrum::on_show(
    )
    {
        Clutter::Gtk::Embed::on_show() ;
        redraw() ;
    }

    bool
    YoukiSpectrum::on_button_press_event(
        GdkEventButton* G_GNUC_UNUSED 
    )
    {
        m_signal.emit() ;
        return true ;
    }

    void
    YoukiSpectrum::on_play_status_changed()
    {
        m_play_status = PlayStatus(m_play->property_status().get_value()) ;
        switch( m_play_status )
        {
            case PLAYSTATUS_PLAYING:
                if( !m_timeout.connected() )
                    m_timeout = Glib::signal_timeout().connect(
                        sigc::mem_fun(
                            *this
                          , &YoukiSpectrum::redraw_handler
                        )
                        , 1000./24.
                    ) ;
                break;
            case PLAYSTATUS_PAUSED:
                redraw() ;
                break;
            case PLAYSTATUS_STOPPED:
                reset() ;
                redraw() ;
                break ;
            default:
                redraw() ;
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

            for( int n = 0; n < SPECT_BANDS; ++n )
            {
                    if( m_spectrum_data[n] < m_spectrum_peak[n] ) 
                        m_spectrum_peak[n] = fmin( m_spectrum_peak[n] - 0.5, 0 ) ;
                    else if( m_spectrum_data[n] == m_spectrum_peak[n] )
                        m_spectrum_peak[n] = fmin( m_spectrum_peak[n] + 2, 72 ) ;
                    else
                        m_spectrum_peak[n] = m_spectrum_data[n];
            }

            redraw() ;
            return true ;
        }

        return false ;
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

    void
    YoukiSpectrum::redraw()
    {
        const Gtk::Allocation& a = get_allocation ();

        for( int n = 0; n < SPECT_BANDS; ++n ) 
        {
            int   w = 0
                , h = 0 ;

            w = WIDTH ;

            //// PEAK 

            int  peak = m_spectrum_peak[n] / 2 ; 
            h =  peak + HEIGHT ;

            if( w && h ) 
            {
                Glib::RefPtr<Clutter::Actor> rect = m_group_peaks->get_nth_child( n ) ;
                rect->set_size( w, h ) ;
                rect->set_position( a.get_width()/2 - ((WIDTH+SPACING)*SPECT_BANDS)/2 + (WIDTH+SPACING)*n, HEIGHT - h + 4 ) ;
            }

            //// BAR 

            int   bar = m_spectrum_data[n] / 2 ;
            h =   bar + HEIGHT ;

            if( w && h ) 
            {
                Glib::RefPtr<Clutter::Actor> rect = m_group_bars->get_nth_child( n ) ;
                rect->set_size( w, h ) ;
                rect->set_position( a.get_width()/2 - ((WIDTH+SPACING)*SPECT_BANDS)/2 + (WIDTH+SPACING)*n, HEIGHT - h + 4 ) ;
            }
        }
    }

    void
    YoukiSpectrum::reset ()
    {
        std::fill( m_spectrum_data.begin(), m_spectrum_data.end(), 0. ) ;
        std::fill( m_spectrum_peak.begin(), m_spectrum_peak.end(), 0. ) ;
        redraw() ;
    }
}
