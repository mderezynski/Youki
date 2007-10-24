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
#include "source-musiclib.hh"
#include <gtkmm/gtkmm.h>

namespace MPX
{
    using namespace Glib;
    class MusicLibPrivate
    {
        typedef Gtk::TreeModelColumn Col;
        struct PlaylistColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
              Col<ustring> Artist;
              Col<ustring> Album;
              Col<guint64> Track;
              Col<ustring> Title;
              Col<guint64> Length;
        };

        PlaylistColumnsT PlaylistColumns;
        Glib::RefPtr<Gtk::ListStore> ListStore;
    };
}

namespace MPX
{
    PlaybackSourceMusicLib ()
    : PlaybackSource(_("Music"))
    {
      m_Private = new MusicLibPrivate;
    }

    Glib::ustring
    PlaybackSourceMusicLib::get_uri () 
    {}

    Glib::ustring
    PlaybackSourceMusicLib::get_type ()
    {}

    GHashTable *
    PlaybackSourceMusicLib::get_metadata ()
    {}

    bool
    PlaybackSourceMusicLib::play ()
    {}

    bool
    PlaybackSourceMusicLib::go_next ()
    {}

    bool
    PlaybackSourceMusicLib::go_prev ()
    {}

    void
    PlaybackSourceMusicLib::stop () 
    {}

    void
    PlaybackSourceMusicLib::play_post () 
    {}

    void
    PlaybackSourceMusicLib::next_post () 
    {}

    void
    PlaybackSourceMusicLib::prev_post ()
    {}

    void
    PlaybackSourceMusicLib::restore_context ()
    {}

    void
    PlaybackSourceMusicLib::skipped () 
    {}

    void
    PlaybackSourceMusicLib::segment () 
    {}

    void
    PlaybackSourceMusicLib::buffering_done () 
    {}

    Glib::RefPtr<Gdk::Pixbuf>
    PlaybackSourceMusicLib::get_icon ()
    {}

    Gtk::Widget*
    PlaybackSourceMusicLib::get_ui ()
    {}

} // end namespace MPX 
