//
// markov-analyzer-thread
//
// Authors:
//     Milosz Derezynski <milosz@backtrace.info>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
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
#ifndef MPX_MARKOV_ANALYZER_THREAD_HH
#define MPX_MARKOV_ANALYZER_THREAD_HH
#include <glibmm.h>
#include <sigx/sigx.h>
#include <sigx/signal_f.h>
#include <sigx/request_f.h>
#include "mpx/mpx-library.hh"
#include "mpx/mpx-types.hh"
#include <deque>

namespace MPX
{
	class MarkovAnalyzerThread : public sigx::glib_threadable
	{
        public:

            sigx::request_f<MPX::Track&>  append ;

        public:	

            MarkovAnalyzerThread (MPX::Library&) ;
            ~MarkovAnalyzerThread () ;

        protected:

            virtual void on_startup () ; 
            virtual void on_cleanup () ;

            void on_append (MPX::Track&) ;
            bool process_idle();

            void
            process_tracks(
                MPX::Track & track1,
                MPX::Track & track2
            );

        private:
    
            typedef std::deque<MPX::Track>  TrackQueue_t;

            struct ThreadData;

            MPX::Library                * m_Library ;
            Glib::Private<ThreadData>     m_ThreadData ;
            sigc::connection              m_idleConnection;
	};
}

#endif
