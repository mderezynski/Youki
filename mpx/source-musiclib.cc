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
#include "mpx/types.hh"
#include "widgets/cell-renderer-cairo-surface.hh"
#include "library.hh"
#include "util-graphics.hh"
#include "widgetloader.h"
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
                ustring albs_a = (*iter_a)[PlaylistColumns.AlbumSort];
                ustring albs_b = (*iter_b)[PlaylistColumns.AlbumSort];
                ustring alb_a = (*iter_a)[PlaylistColumns.Album];
                ustring alb_b = (*iter_b)[PlaylistColumns.Album];

                int cmp = 0;
                if(!albs_a.empty() && !albs_b.empty())
                    cmp = albs_a.compare(albs_b); 
                else
                    cmp = alb_a.compare(alb_b); 

                return cmp;
              }

              int
              slotSortByArtist(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                ustring arts_a = (*iter_a)[PlaylistColumns.ArtistSort];
                ustring arts_b = (*iter_b)[PlaylistColumns.ArtistSort];
                ustring art_a = (*iter_a)[PlaylistColumns.Artist];
                ustring art_b = (*iter_b)[PlaylistColumns.Artist];

                int cmp = 0;
                if(!arts_a.empty() && !arts_b.empty())
                    cmp = arts_a.compare(arts_b); 
                else
                    cmp = art_a.compare(art_b); 

                return cmp;
              }
        };

        struct AlbumColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
            Gtk::TreeModelColumn<Cairo::RefPtr<Cairo::ImageSurface> > Image;
            Gtk::TreeModelColumn<Glib::ustring> Text;
            Gtk::TreeModelColumn<std::string> AlbumSort;
            Gtk::TreeModelColumn<std::string> ArtistSort;
            AlbumColumnsT ()
            {
                add (Image);
                add (Text);
                add (AlbumSort);
                add (ArtistSort);
            };
        };

        class AlbumTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              typedef std::set<TreeIter> IterSet;
              typedef std::map<std::string, IterSet> ASINIterMap;

              AlbumColumnsT AlbumColumns;
              Glib::RefPtr<Gtk::ListStore> ListStore;
              MPX::Library &m_Lib;
              MPX::Amazon::Covers &m_AMZN;
              ASINIterMap m_ASINIterMap;
              Cairo::RefPtr<Cairo::ImageSurface> m_DiscDefault;

              void
              on_got_cover(const Glib::ustring& asin)
              {
                Cairo::RefPtr<Cairo::ImageSurface> Cover;
                m_AMZN.fetch(asin, Cover, COVER_SIZE_ALBUM_LIST);
                Util::cairo_image_surface_border(Cover, 1.);
                IterSet & set = m_ASINIterMap[asin];
                for(IterSet::iterator i = set.begin(); i != set.end(); ++i)
                {
                    (*(*i))[AlbumColumns.Image] = Cover;
                }
              }
    
              void
              on_new_album(const std::string& mbid, const std::string& asin, gint64 id)
              {
                TreeIter iter = ListStore->append();

                (*iter)[AlbumColumns.Image] = m_DiscDefault; 

                if(!asin.empty())
                {
                  IterSet & s = m_ASINIterMap[asin];
                  s.insert(iter);
                  m_AMZN.cache(asin);
                }

                SQL::RowV v;
                m_Lib.getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album.id = %lld;") % id).str());

                g_return_if_fail(!v.empty());

                SQL::Row & r = v[0];

                std::string ArtistSort;
                if(r.find("album_artist_sortname") != r.end())
                    ArtistSort = get<std::string>(r["album_artist_sortname"]);
                else
                    ArtistSort = get<std::string>(r["album_artist"]);

                (*iter)[AlbumColumns.Text] = (boost::format("<big><b>%s</b>\n%s</big>") % Markup::escape_text(get<std::string>(r["album"])).c_str() % Markup::escape_text(ArtistSort).c_str()).str();
                (*iter)[AlbumColumns.AlbumSort] = ustring(get<std::string>(r["album"])).collate_key();
                (*iter)[AlbumColumns.ArtistSort] = ustring(ArtistSort).collate_key();
              }

              int
              slotSort(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                std::string alb_a = (*iter_a)[AlbumColumns.AlbumSort];
                std::string alb_b = (*iter_b)[AlbumColumns.AlbumSort];
                std::string art_a = (*iter_a)[AlbumColumns.ArtistSort];
                std::string art_b = (*iter_b)[AlbumColumns.ArtistSort];

                if(art_a != art_b)
                    return art_a.compare(art_b);

                return alb_a.compare(alb_b); 
              }

            public:

              AlbumTreeView (Glib::RefPtr<Gnome::Glade::Xml> const& xml, MPX::Library &lib, MPX::Amazon::Covers &amzn)
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-albums")
              , m_Lib(lib)
              , m_AMZN(amzn)
              {
                m_Lib.signal_new_album().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_album ));
                m_AMZN.signal_got_cover().connect( sigc::mem_fun( *this, &AlbumTreeView::on_got_cover ));

                TreeViewColumn * col = manage (new TreeViewColumn());
                CellRendererCairoSurface * cellcairo = manage (new CellRendererCairoSurface);
                col->pack_start(*cellcairo);
                col->add_attribute(*cellcairo, "surface", AlbumColumns.Image);
                append_column(*col);

                col = manage (new TreeViewColumn());
                CellRendererText * celltext = manage (new CellRendererText);
                col->pack_start(*celltext);
                col->add_attribute(*celltext, "markup", AlbumColumns.Text);
                append_column(*col);

                CellRenderer * cell = 0;
                cell = get_column(0)->get_first_cell_renderer();
                cell->property_xpad() = 4;
                cell->property_ypad() = 2;
                cell->property_yalign() = 0.;
                cell = get_column(1)->get_first_cell_renderer();
                cell->property_yalign() = 0.;
                cell->property_ypad() = 2;

                ListStore = Gtk::ListStore::create(AlbumColumns);
                set_model(ListStore);
                ListStore->set_default_sort_func(sigc::mem_fun( *this, &AlbumTreeView::slotSort ));
                ListStore->set_sort_column(GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

                m_DiscDefault = Util::cairo_image_surface_from_pixbuf(Gdk::Pixbuf::create_from_file(build_filename(DATA_DIR, build_filename("images","disc-default.png")))->scale_simple(72, 72, Gdk::INTERP_BILINEAR));
              }
        };

        PlaylistTreeView *m_TreeViewPlaylist;
        AlbumTreeView *m_TreeViewAlbums;

        MusicLibPrivate (MPX::Library &lib, MPX::Amazon::Covers &amzn)
        {
            const std::string path (build_filename(DATA_DIR, build_filename("glade","source-musiclib.glade")));
            m_RefXml = Gnome::Glade::Xml::create (path);
            m_UI = m_RefXml->get_widget("source-musiclib");
            m_TreeViewPlaylist = new PlaylistTreeView(m_RefXml, lib);
            m_TreeViewAlbums = new AlbumTreeView(m_RefXml, lib, amzn);
        }
    };
}

namespace MPX
{
    PlaybackSourceMusicLib::PlaybackSourceMusicLib (MPX::Library &lib, MPX::Amazon::Covers& amzn)
    : PlaybackSource(_("Music"))
    {
      m_Private = new MusicLibPrivate(lib,amzn);
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
