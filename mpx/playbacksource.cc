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
#include <glibmm/i18n.h>
#include <sigc++/signal.h>
#include <boost/format.hpp>

#include "mpx/i-playbacksource.hh"

using namespace Glib;

namespace MPX
{
		PlaybackSource::PlaybackSource (const Glib::RefPtr<Gtk::UIManager>&, const std::string& name, Caps caps, Flags flags)
        : m_Caps(caps)
        , m_Flags(flags)
        , m_Name(name)
        {
        }

        PlaybackSource::~PlaybackSource ()
        {
        }

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

		std::string
		PlaybackSource::get_type ()
		{
			return std::string();
		}

        void
        PlaybackSource::play_async ()
		{
		}

        void
        PlaybackSource::go_next_async ()
		{
		}

        void
        PlaybackSource::go_prev_async ()
		{
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
          Signals.Caps.emit (m_Caps);
        }

        void
        PlaybackSource::send_flags ()
        {
          Signals.Flags.emit (m_Flags);
        }

        void
        PlaybackSource::segment ()
        {
        }

        std::string
        PlaybackSource::get_name ()
        {
          return m_Name;
        }

        void
        PlaybackSource::buffering_done ()
        {
		}

        UriSchemes
        PlaybackSource::get_uri_schemes ()
        {
            return UriSchemes();
        }

        void
        PlaybackSource::process_uri_list (Util::FileList const&, bool play)
        {
        }

		guint
		PlaybackSource::add_menu ()
		{
			return 0;
		}

        PyObject*
        PlaybackSource::get_py_obj ()
        {
            Py_INCREF(Py_None);
            return Py_None;
        }

        ItemKey const&
        PlaybackSource::get_key ()
        {
            return m_OwnKey;
        }

        void
        PlaybackSource::set_key (ItemKey const& key)
        {
            m_OwnKey = key;
        }

        void
        PlaybackSource::post_install ()
        {
        }

} // end namespace MPX 
