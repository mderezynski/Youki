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
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <libglademm.h>
#include "widgetloader.h"
#include "mpx/types.hh"
#include "library.hh"
using namespace Gtk;
using boost::get;

namespace MPX
{
    using namespace Glib;
    using namespace Gnome::Glade;

    struct MusicLibPrivate
    {
        Glib::RefPtr<Gnome::Glade::Xml> m_RefXml;
        Gtk::Widget * m_UI;

        struct PlaylistColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
            Gtk::TreeModelColumn<ustring> Artist;
            Gtk::TreeModelColumn<ustring> Album;
            Gtk::TreeModelColumn<guint64> Track;
            Gtk::TreeModelColumn<ustring> Title;
            Gtk::TreeModelColumn<guint64> Length;

            // These hidden columns are used for sorting
            // They don't contain sortnames, as one might think
            // but instead the MB IDs (if not available, then just the 
            // plain name)
            Gtk::TreeModelColumn<ustring> ArtistSort;
            Gtk::TreeModelColumn<ustring> AlbumSort;

            PlaylistColumnsT ()
            {
                add (Artist);
                add (Album);
                add (Track);
                add (Title);
                add (Length);
                add (ArtistSort);
                add (AlbumSort);
            };
        };

        class PlaylistTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              PlaylistColumnsT PlaylistColumns;
              Glib::RefPtr<Gtk::ListStore> ListStore;
              MPX::Library & m_Lib;

            public:

              enum Column
              {
                C_ARTIST,
                C_ALBUM,
                C_TRACK,
                C_TITLE,
                C_LENGTH
              };

              PlaylistTreeView (Glib::RefPtr<Gnome::Glade::Xml> const& xml, MPX::Library &lib)
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-playlist")
              , m_Lib(lib)
              {
                append_column(_("Artist"), PlaylistColumns.Artist);
                append_column(_("Album"), PlaylistColumns.Album);
                append_column(_("Track"), PlaylistColumns.Track);
                append_column(_("Title"), PlaylistColumns.Title);
                get_column(C_ARTIST)->set_sort_column_id(PlaylistColumns.Artist);
                get_column(C_ALBUM)->set_sort_column_id(PlaylistColumns.Album);
                get_column(C_TRACK)->set_sort_column_id(PlaylistColumns.Track);
                get_column(C_TITLE)->set_sort_column_id(PlaylistColumns.Title);

                TreeViewColumn * col = manage (new TreeViewColumn(_("Length")));
                CellRendererText * cell = manage (new CellRendererText);
                col->pack_start(*cell, false);
                col->set_cell_data_func(*cell, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFunc ));
                col->set_sort_column_id(PlaylistColumns.Length);
                append_column(*col);

                ListStore = Gtk::ListStore::create(PlaylistColumns);

                ListStore->set_sort_func(PlaylistColumns.Artist,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByArtist ));

                ListStore->set_sort_func(PlaylistColumns.Album,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByAlbum ));

                ListStore->set_sort_func(PlaylistColumns.Track,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByTrack ));

                set_headers_clickable();
                set_model(ListStore);

                std::vector<TargetEntry> Entries;
                drag_dest_set(Entries, DEST_DEFAULT_MOTION);
                drag_dest_add_uri_targets();
              }

              virtual void
              on_drag_data_received (const Glib::RefPtr<Gdk::DragContext>&, int, int,
                  const SelectionData& data, guint, guint)
              {
                typedef std::vector<ustring> UV;
                UV uris = data.get_uris();

                for(UV::const_iterator i = uris.begin(); i != uris.end(); ++i)
                {
                  Track track;
                  m_Lib.getMetadata(*i, track);
                  TreeIter iter = ListStore->append();

                  if(track[ATTRIBUTE_ARTIST])
                      (*iter)[PlaylistColumns.Artist] = get<std::string>(track[ATTRIBUTE_ARTIST].get()); 

                  if(track[ATTRIBUTE_ALBUM])
                      (*iter)[PlaylistColumns.Album] = get<std::string>(track[ATTRIBUTE_ALBUM].get()); 

                  if(track[ATTRIBUTE_TRACK])
                      (*iter)[PlaylistColumns.Track] = guint64(get<gint64>(track[ATTRIBUTE_TRACK].get()));

                  if(track[ATTRIBUTE_TITLE])
                      (*iter)[PlaylistColumns.Title] = get<std::string>(track[ATTRIBUTE_TITLE].get()); 

                  if(track[ATTRIBUTE_TIME])
                      (*iter)[PlaylistColumns.Length] = guint64(get<gint64>(track[ATTRIBUTE_TIME].get()));

                  if(track[ATTRIBUTE_MB_ARTIST_ID])
                      (*iter)[PlaylistColumns.ArtistSort] = get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get());

                  if(track[ATTRIBUTE_MB_ALBUM_ID])
                      (*iter)[PlaylistColumns.AlbumSort] = get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get());
                }
              }

              bool
              on_drag_motion (RefPtr<Gdk::DragContext> const& context, int x, int y, guint time)
              {
                return true;
              }

              void
              on_drag_leave (RefPtr<Gdk::DragContext> const& context, guint time)
              {
              }

              bool
              on_drag_drop (RefPtr<Gdk::DragContext> const& context, int x, int y, guint time)
              {
                ustring target (drag_dest_find_target (context));

                if( !target.empty() )
                {
                  drag_get_data (context, target, time);
                  context->drag_finish  (true, false, time);
                  return true;
                }
                else
                {
                  context->drag_finish  (false, false, time);
                  return false;
                }
              }

              void
              cellDataFunc (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                CellRendererText *cell_t = dynamic_cast<CellRendererText*>(basecell);
                guint64 Length = (*iter)[PlaylistColumns.Length]; 
                cell_t->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
              }

              int
              slotSortByTrack(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                guint64 trk_a = (*iter_a)[PlaylistColumns.Track];
                guint64 trk_b = (*iter_b)[PlaylistColumns.Track];
                ustring alb_a = (*iter_a)[PlaylistColumns.AlbumSort];
                ustring alb_b = (*iter_b)[PlaylistColumns.AlbumSort];

                if(alb_a != alb_b)
                    return 0;

                return (trk_a - trk_b);
              }

              int
              slotSortByAlbum(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                ustring alb_a = (*iter_a)[PlaylistColumns.AlbumSort];
                ustring alb_b = (*iter_b)[PlaylistColumns.AlbumSort];
                ustring albp_a = (*iter_a)[PlaylistColumns.Album];
                ustring albp_b = (*iter_b)[PlaylistColumns.Album];

                int cmp = 0;
                if(!alb_a.empty() && !alb_b.empty())
                    cmp = alb_a.compare(alb_b); 
                else
                    cmp = albp_a.compare(albp_b); 

                return cmp;
              }

              int
              slotSortByArtist(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                ustring art_a = (*iter_a)[PlaylistColumns.ArtistSort];
                ustring art_b = (*iter_b)[PlaylistColumns.ArtistSort];
                ustring artp_a = (*iter_a)[PlaylistColumns.Artist];
                ustring artp_b = (*iter_b)[PlaylistColumns.Artist];

                int cmp = 0;
                if(!art_a.empty() && !art_b.empty())
                    cmp = art_a.compare(art_b); 
                else
                    cmp = artp_a.compare(artp_b); 

                return cmp;
              }
        };

        PlaylistTreeView *m_TreeViewPlaylist;

        MusicLibPrivate (MPX::Library &lib)
        {
            const std::string path (build_filename(DATA_DIR, build_filename("glade","source-musiclib.glade")));
            m_RefXml = Gnome::Glade::Xml::create (path);
            m_UI = m_RefXml->get_widget("source-musiclib");
            m_TreeViewPlaylist = new PlaylistTreeView(m_RefXml, lib);
        }
    };
}

namespace MPX
{
    PlaybackSourceMusicLib::PlaybackSourceMusicLib (MPX::Library &lib)
    : PlaybackSource(_("Music"))
    {
      m_Private = new MusicLibPrivate(lib);
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
    {
        try{
            return IconTheme::get_default()->load_icon("audio-x-generic", 16, ICON_LOOKUP_NO_SVG);
        } catch(...) { return Glib::RefPtr<Gdk::Pixbuf>(0); }
    }

    Gtk::Widget*
    PlaybackSourceMusicLib::get_ui ()
    {
        return m_Private->m_UI;
    }

} // end namespace MPX 
