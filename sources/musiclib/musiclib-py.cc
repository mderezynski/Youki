//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
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
//  The MPX project hereby grants permission for non-GPlaylist compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPlaylist license
//  MPX is covered by.
#include "config.h"
#include "source-musiclib.hh"

#include "mpx/mpx-main.hh"
#include "mcs/base.h"

#define NO_IMPORT
#include <pygtk/pygtk.h>
#include <pygobject.h>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include "mpx/mpx-python.hh"
using namespace boost::python;

//#if PY_VERSION_HEX < 0x02050000
//typedef int Py_ssize_t;
//#endif

using namespace Glib;

BOOST_PYTHON_MODULE(mpx_musiclib)
{
    class_<MPX::Source::PlaybackSourceMusicLib, boost::noncopyable>("MusicLib", boost::python::no_init)
            .def("play_tracks", &MPX::Source::PlaybackSourceMusicLib::play_tracks)
            .def("play_album", &MPX::Source::PlaybackSourceMusicLib::play_album)
            .def("append_tracks", &MPX::Source::PlaybackSourceMusicLib::append_tracks)
            .def("get_playlist_model", &MPX::Source::PlaybackSourceMusicLib::get_playlist_model)
            .def("get_playlist_current_iter", &MPX::Source::PlaybackSourceMusicLib::get_playlist_current_iter)
            .def("set_sensitive", &MPX::Source::PlaybackSourceMusicLib::plist_sensitive)
            .def("gobj", &mpxpy::get_gobject<MPX::Source::PlaybackSourceMusicLib>)
    ;
}

namespace MPX
{
    void
    mpx_musiclib_py_init ()
    {
            initmpx_musiclib();
    }
}
