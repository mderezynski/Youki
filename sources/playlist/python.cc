#include <boost/python.hpp>
#include <Python.h>
#include "mpx/i-playbacksource.hh"
#include "mpx/python.hh"
#include "source-playlist.hh"

using namespace boost::python;

BOOST_PYTHON_MODULE(mpx_playlist)
{
    class_<MPX::Source::PlaybackSourcePlaylist, bases<MPX::PlaybackSource>, boost::noncopyable>("Playlist", boost::python::no_init)
            .def("play_tracks", &MPX::Source::PlaybackSourcePlaylist::play_tracks)
            .def("play_album", &MPX::Source::PlaybackSourcePlaylist::play_album)
            .def("append_tracks", &MPX::Source::PlaybackSourcePlaylist::append_tracks)
            .def("get_playlist_model", &MPX::Source::PlaybackSourcePlaylist::get_playlist_model)
            .def("get_playlist_current_iter", &MPX::Source::PlaybackSourcePlaylist::get_playlist_current_iter)
            .def("gobj", &mpxpy::get_gobject<MPX::Source::PlaybackSourcePlaylist>)
    ;
}

namespace MPX
{
    void
    mpx_playlist_py_init ()
    {
        try {
            g_message("%s: initializing MPX Playlist Py", G_STRLOC);
            initmpx_playlist();
        } catch( error_already_set ) {
            g_warning("%s; Python Error:", G_STRFUNC);
            PyErr_Print();
        }
    }
}
