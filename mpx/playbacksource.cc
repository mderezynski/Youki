//  MPX
//  Copyright (C) 2005-2007 MPX development.
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
#include "config.h" 
#include <exception>
#include <glib/ghash.h>
#include <glibmm/ustring.h>
#include <glibmm/i18n.h>
#include <sigc++/signal.h>
#include <boost/format.hpp>

#include "mpx/playbacksource.hh"

using namespace Glib;

namespace MPX
{
        PlaybackSource::SignalNextAsync&
        PlaybackSource::signal_next_async ()
        {
          return Signals.NextAsync;
        }

        PlaybackSource::SignalPrevAsync&
        PlaybackSource::signal_prev_async ()
        {
          return Signals.PrevAsync;
        }

        PlaybackSource::SignalPlayAsync&
        PlaybackSource::signal_play_async ()
        {
          return Signals.PlayAsync;
        }

        PlaybackSource::SignalCaps &
        PlaybackSource::signal_caps ()
        {
          return Signals.Caps;
        }

        PlaybackSource::SignalFlags &
        PlaybackSource::signal_flags ()
        {
          return Signals.Flags;
        }

        PlaybackSource::SignalTrackMetadata &
        PlaybackSource::signal_track_metadata ()
        {
          return Signals.Metadata;
        }

        PlaybackSource::SignalPlaybackRequest &
        PlaybackSource::signal_playback_request ()
        {
          return Signals.PlayRequest;
        }

        PlaybackSource::SignalStopRequest &
        PlaybackSource::signal_stop_request ()
        {
          return Signals.StopRequest;
        }

        PlaybackSource::SignalNextRequest &
        PlaybackSource::signal_next_request ()
        {
          return Signals.NextRequest;
        }

        PlaybackSource::SignalSegment &
        PlaybackSource::signal_segment ()
        {
          return Signals.Segment;
        }

        PlaybackSource::SignalMessage &
        PlaybackSource::signal_message ()
        {
          return Signals.Message;
        }

        PlaybackSource::SignalMessageClear &
        PlaybackSource::signal_message_clear ()
        {
          return Signals.MessageClear;
        }


        void
        PlaybackSource::next_post ()
        { 
        }

        void
        PlaybackSource::prev_post ()
        {
        }

        void
        PlaybackSource::skipped ()
        {
        }

        void
        PlaybackSource::send_caps ()
        {
          Signals.Caps.emit (m_caps);
        }

        void
        PlaybackSource::send_flags ()
        {
          Signals.Flags.emit (m_flags);
        }

        GHashTable*
        PlaybackSource::get_metadata ()
        {
          return 0;
        } 

        void
        PlaybackSource::segment ()
        {
        }

        ustring
        PlaybackSource::get_name ()
        {
          return m_name;
        }

        void
        PlaybackSource::buffering_done ()
        {};

} // end namespace MPX 
