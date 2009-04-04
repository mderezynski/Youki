#include "config.h"
#include <glibmm/i18n.h>
#include <cmath>

#include "infoarea.hh"
#include "mpx/mpx-audio-types.hh"
#include "mpx/mpx-play.hh"

namespace MPX
{
    InfoArea::InfoArea(
    )
    : m_spectrum_data(SPECT_BANDS, 0)
    , m_spectrum_peak(SPECT_BANDS, 0)
    {
        add_events (Gdk::BUTTON_PRESS_MASK);
        set_size_request( -1, 36 ) ;
    }

    void
    InfoArea::reset ()
    {
            std::fill (m_spectrum_data.begin (), m_spectrum_data.end (), 0.);
            std::fill (m_spectrum_peak.begin (), m_spectrum_peak.end (), 0.);
    }

    void
    InfoArea::update_spectrum(
        const Spectrum& spectrum
    )
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
