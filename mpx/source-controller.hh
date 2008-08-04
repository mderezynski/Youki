//
//  mpx-source-handling.cc
//
//  Authors:
//      Milosz Derezynski <milosz@backtrace.info>
//
//  (C); 2008 MPX Project
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option); any later version.
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

#ifndef MPX_SOURCE_CONTROLLER_HH
#define MPX_SOURCE_CONTROLLER_HH

#include <boost/python.hpp>
#include <boost/optional.hpp>
#include "config.h"
#include <glibmm.h>
#include <Python.h>
#include <pygobject.h>
#include "mpx/util-string.hh"
#include "mpx/i-playbacksource.hh"

namespace MPX
{
    class Player;
    class Play;
    class SourceController
    {
        private:

                struct SourcePlugin
                {
                    PlaybackSource* (*get_instance)       (const Glib::RefPtr<Gtk::UIManager>&, MPX::Player&);
                    void            (*del_instance)       (MPX::PlaybackSource*);
                };

                typedef boost::shared_ptr<SourcePlugin>          SourcePluginPtr;
                typedef std::vector<SourcePluginPtr>             SourcePluginsKeeper;

                SourcePluginsKeeper             m_SourcePlugins;

                MPX::Play   & m_Play;
                MPX::Player & m_Player;
        public:

                SourceController (MPX::Play&, MPX::Player&);
                ~SourceController ();
        private:
                bool
                source_load (std::string const& path);

                void
                source_install (ItemKey const& source_id);

                void
                on_source_items(gint64 count, ItemKey const& key);

                PyObject*
                get_source(std::string const& uuid);

                PyObject*
                get_sources_by_class(std::string const& uuid);

                void
                add_subsource(PlaybackSource* p, ItemKey const& parent, gint64 id);

                void
                on_source_caps (Caps caps, ItemKey const& source_id);

                void
                on_source_flags (Flags flags, ItemKey const& source_id);

                void
                on_source_track_metadata (Metadata const& metadata, ItemKey const& source_id);

                void
                on_source_play_request (ItemKey const& source_id);

                void
                on_source_segment (ItemKey const& source_id);

                void
                on_source_stop (ItemKey const& source_id);

                void
                on_source_changed (ItemKey const& source_id);

                void
                play_async_cb (ItemKey const& source_id);

                void
                next_async_cb (ItemKey const& source_id);

                void
                prev_async_cb (ItemKey const& source_id);
    };
}

#endif
