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
#ifdef HAVE_TR1
#include <tr1/unordered_map>
#endif //HAVE_TR1
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <libglademm.h>
#include "mpx/types.hh"
#include "widgets/cell-renderer-cairo-surface.hh"
#include "widgets/gossip-cell-renderer-expander.h"
#include "library.hh"
#include "util-graphics.hh"
#include "widgetloader.h"
#include "source-musiclib.hh"
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
                Entries.push_back(TargetEntry("mpx-album"));
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

        enum AlbumRowType
        {
            ROW_ALBUM,
            ROW_TRACK,
        };

        struct AlbumColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
            Gtk::TreeModelColumn<AlbumRowType> RowType;
            Gtk::TreeModelColumn<bool> HasTracks;
            Gtk::TreeModelColumn<Cairo::RefPtr<Cairo::ImageSurface> > Image;
            Gtk::TreeModelColumn<Glib::ustring> Text;
            Gtk::TreeModelColumn<std::string> AlbumSort;
            Gtk::TreeModelColumn<std::string> ArtistSort;
            Gtk::TreeModelColumn<std::string> ASIN;
            Gtk::TreeModelColumn<gint64> Date;
            Gtk::TreeModelColumn<gint64> Id;
            Gtk::TreeModelColumn<Glib::ustring> TrackTitle;
            Gtk::TreeModelColumn<Glib::ustring> TrackArtist;
            Gtk::TreeModelColumn<gint64> TrackNumber;
            Gtk::TreeModelColumn<gint64> TrackLength;
            Gtk::TreeModelColumn<gint64> TrackId;
            AlbumColumnsT ()
            {
                add (RowType);
                add (HasTracks);
                add (Image);
                add (Text);
                add (AlbumSort);
                add (ArtistSort);
                add (ASIN);
                add (Date);
                add (Id);
                add (TrackTitle);
                add (TrackArtist);
                add (TrackNumber);
                add (TrackLength);
                add (TrackId);
            };
        };

        class AlbumTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              typedef std::set<TreeIter> IterSet;
              typedef std::map<std::string, IterSet> ASINIterMap;
              typedef std::map<gint64, TreeIter> IdIterMap; 

              AlbumColumnsT AlbumColumns;
              Glib::RefPtr<Gtk::TreeStore> TreeStore;
              MPX::Library &m_Lib;
              MPX::Amazon::Covers &m_AMZN;
              ASINIterMap m_ASINIterMap;
              Cairo::RefPtr<Cairo::ImageSurface> m_DiscDefault;
              Glib::RefPtr<Gdk::Pixbuf> m_DiscDefault_Pixbuf;
              sigc::connection conn_timeout_cover;
              std::string m_DragASIN;
              IdIterMap m_AlbumIterMap;

              virtual void
              on_row_expanded (const TreeIter &iter,
                               const TreePath &path) 
              {
                if(!(*iter)[AlbumColumns.HasTracks])
                {
                    GtkTreeIter children;
                    bool has_children = (gtk_tree_model_iter_children(GTK_TREE_MODEL(TreeStore->gobj()), &children, const_cast<GtkTreeIter*>(iter->gobj())));

                    SQL::RowV v;
                    m_Lib.getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track;") % gint64((*iter)[AlbumColumns.Id])).str());

                    for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                    {
                        SQL::Row & r = *i;
                        TreeIter child = TreeStore->append(iter->children());
                        (*child)[AlbumColumns.TrackTitle] = get<std::string>(r["title"]);
                        (*child)[AlbumColumns.TrackArtist] = get<std::string>(r["artist"]);
                        (*child)[AlbumColumns.TrackNumber] = get<gint64>(r["track"]);
                        (*child)[AlbumColumns.TrackLength] = get<gint64>(r["time"]);

                        (*child)[AlbumColumns.RowType] = ROW_TRACK; 
                    }

                    (*iter)[AlbumColumns.HasTracks] = true;

                    if(has_children)
                    {
                        gtk_tree_store_remove(GTK_TREE_STORE(TreeStore->gobj()), &children);
                    } 
                    else
                        g_warning("%s:%d : No placeholder row present, state seems corrupted.", __FILE__, __LINE__);

                }
              }

              virtual void
              on_drag_begin (const Glib::RefPtr<Gdk::DragContext>& context) 
              {
                Glib::RefPtr<Gdk::Pixbuf> Cover;
                if(!m_DragASIN.empty())
                {
                    m_AMZN.fetch(m_DragASIN, Cover);
                    drag_source_set_icon(Cover->scale_simple(128,128,Gdk::INTERP_BILINEAR));
                }
                else
                    drag_source_set_icon(m_DiscDefault_Pixbuf->scale_simple(128,128,Gdk::INTERP_BILINEAR));

              }

              virtual bool
              on_button_press_event (GdkEventButton* event)
              {
                int tree_x, tree_y, cell_x, cell_y ;
                TreePath path ;
                TreeViewColumn *col ;

                if(get_path_at_pos (event->x, event->y, path, col, cell_x, cell_y))
                {
                    TreeIter iter = TreeStore->get_iter(path);
                    m_DragASIN = (*iter)[AlbumColumns.ASIN];
                }
                TreeView::on_button_press_event(event);
                return false;
              }

              virtual bool
              on_motion_notify_event (GdkEventMotion* event)
              {
                conn_timeout_cover.disconnect();

                int x, y;
                int x_orig, y_orig;
                GdkModifierType state;

                if (event->is_hint)
                {
                  gdk_window_get_pointer (event->window, &x_orig, &y_orig, &state);
                }
                else
                {
                  x_orig = int (event->x);
                  y_orig = int (event->y);
                  state = GdkModifierType (event->state);
                }

                int tree_x, tree_y, cell_x, cell_y ;
                TreePath path ;
                TreeViewColumn *col ;

                if(get_path_at_pos (x_orig, y_orig, path, col, cell_x, cell_y))
                {
                    if(col == get_column(0))
                    {
                        TreeIter iter = TreeStore->get_iter(path);
                    }
                }
                return false;
              }

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
                TreeIter iter = TreeStore->append();
                m_AlbumIterMap[id] = iter;
                TreeStore->append(iter->children());

                (*iter)[AlbumColumns.RowType] = ROW_ALBUM; 
                (*iter)[AlbumColumns.HasTracks] = false; 
                (*iter)[AlbumColumns.Image] = m_DiscDefault; 
                (*iter)[AlbumColumns.ASIN] = asin; 
                (*iter)[AlbumColumns.Id] = id; 

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

                std::string date; 
                if(r.count("mb_release_date"))
                {
                    date = get<std::string>(r["mb_release_date"]);
                    gint64 date_int;
                    sscanf(date.c_str(), "%04lld", &date_int);
                    (*iter)[AlbumColumns.Date] = date_int; 
                }
                else
                    (*iter)[AlbumColumns.Date] = 0; 

                std::string ArtistSort;
                if(r.find("album_artist_sortname") != r.end())
                    ArtistSort = get<std::string>(r["album_artist_sortname"]);
                else
                    ArtistSort = get<std::string>(r["album_artist"]);

                (*iter)[AlbumColumns.Text] = (boost::format("<b>%s</b>\n%s\n<small>%s</small>") % Markup::escape_text(get<std::string>(r["album"])).c_str() % Markup::escape_text(ArtistSort).c_str() % date.substr(0,4)).str();
                (*iter)[AlbumColumns.AlbumSort] = ustring(get<std::string>(r["album"])).collate_key();
                (*iter)[AlbumColumns.ArtistSort] = ustring(ArtistSort).collate_key();
              }

              void
              on_new_track(Track & track, gint64 albumid)
              {
                if(m_AlbumIterMap.count(albumid))
                {
                    TreeIter iter = m_AlbumIterMap[albumid];
                    if (((*iter)[AlbumColumns.HasTracks]))
                    {
                        TreeIter child = TreeStore->append(iter->children());
                        if(track[ATTRIBUTE_TITLE])
                            (*child)[AlbumColumns.TrackTitle] = get<std::string>(track[ATTRIBUTE_TITLE].get());
                        if(track[ATTRIBUTE_ARTIST])
                            (*child)[AlbumColumns.TrackArtist] = get<std::string>(track[ATTRIBUTE_ARTIST].get());
                        if(track[ATTRIBUTE_TRACK])
                            (*child)[AlbumColumns.TrackNumber] = get<gint64>(track[ATTRIBUTE_TRACK].get());
                        if(track[ATTRIBUTE_TIME])
                            (*child)[AlbumColumns.TrackLength] = get<gint64>(track[ATTRIBUTE_TIME].get());

                        (*child)[AlbumColumns.RowType] = ROW_TRACK; 
                    }
                }
              }

              int
              slotSort(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[AlbumColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[AlbumColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[AlbumColumns.Date];
                  gint64 alb_b = (*iter_b)[AlbumColumns.Date];
                  std::string art_a = (*iter_a)[AlbumColumns.ArtistSort];
                  std::string art_b = (*iter_b)[AlbumColumns.ArtistSort];

                  if(art_a != art_b)
                      return art_a.compare(art_b);

                  return alb_a - alb_b;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[AlbumColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[AlbumColumns.TrackNumber];

                  return trk_a - trk_b;
                }
              }

              void
              cellDataFuncCover (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererCairoSurface *cell = dynamic_cast<CellRendererCairoSurface*>(basecell);
                if(path.get_depth() == 1)
                {
                    cell->property_visible() = true;
                    cell->property_surface() = (*iter)[AlbumColumns.Image]; 
                }
                else
                {
                    cell->property_visible() = false;
                }
              }

              void
              cellDataFuncText1 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == 1)
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = (*iter)[AlbumColumns.Text]; 
                }
                else
                {
                    cell->property_visible() = false; 
                }
              }

              void
              cellDataFuncText2 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == 1)
                {
                    cell->property_visible() = false; 
                }
                else
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = (boost::format("%lld") % (*iter)[AlbumColumns.TrackNumber]).str();
                }
              }

              void
              cellDataFuncText3 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == 1)
                {
                    cell->property_visible() = false; 
                }
                else
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = Markup::escape_text((*iter)[AlbumColumns.TrackTitle]);
                }
              }

              void
              cellDataFuncText4 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == 1)
                {
                    cell->property_visible() = false; 
                }
                else
                {
                    cell->property_visible() = true; 
                    gint64 Length = (*iter)[AlbumColumns.TrackLength];
                    cell->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
                }
              }

              void
              album_list_load ()
              {
                SQL::RowV v;
                m_Lib.getSQL(v, "SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id;");
                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                {
                    SQL::Row & r = *i; 

                    TreeIter iter = TreeStore->append();
                    TreeStore->append(iter->children());

                    (*iter)[AlbumColumns.RowType] = ROW_ALBUM; 
                    (*iter)[AlbumColumns.HasTracks] = false; 
                    (*iter)[AlbumColumns.Image] = m_DiscDefault; 
                    (*iter)[AlbumColumns.ASIN] = ""; 
                    (*iter)[AlbumColumns.Id] = get<gint64>(r["id"]); 
      
                    if(r.count("amazon_asin"))
                    {
                        std::string asin = get<std::string>(r["amazon_asin"]);
                        (*iter)[AlbumColumns.ASIN] = asin; 
                        IterSet & s = m_ASINIterMap[asin];
                        s.insert(iter);
                        m_AMZN.cache(asin);
                    }

                    std::string date;
                    if(r.count("mb_release_date"))
                    {
                        date = get<std::string>(r["mb_release_date"]);
                        gint64 date_int;
                        sscanf(date.c_str(), "%04lld", &date_int);
                        (*iter)[AlbumColumns.Date] = date_int; 
                    }
                    else
                        (*iter)[AlbumColumns.Date] = 0; 

                    std::string ArtistSort;
                    if(r.count("album_artist_sortname"))
                        ArtistSort = get<std::string>(r["album_artist_sortname"]);
                    else
                        ArtistSort = get<std::string>(r["album_artist"]);

                    (*iter)[AlbumColumns.Text] = (boost::format("<b>%s</b>\n%s\n<small>%s</small>") % Markup::escape_text(get<std::string>(r["album"])).c_str() % Markup::escape_text(ArtistSort).c_str() % date.substr(0,4)).str();
                    (*iter)[AlbumColumns.AlbumSort] = ustring(get<std::string>(r["album"])).collate_key();
                    (*iter)[AlbumColumns.ArtistSort] = ustring(ArtistSort).collate_key();
                }
              }

              static void
              rb_sourcelist_expander_cell_data_func (GtkTreeViewColumn *column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           data) 
              {
                  if (gtk_tree_model_iter_has_child (model, iter))
                  {
                      GtkTreePath *path;
                      gboolean     row_expanded;

                      path = gtk_tree_model_get_path (model, iter);
                      row_expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (column->tree_view), path);
                      gtk_tree_path_free (path);

                      g_object_set (cell,
                                "visible", TRUE,
                                "expander-style", row_expanded ? GTK_EXPANDER_EXPANDED : GTK_EXPANDER_COLLAPSED,
                                NULL);
                  } else {
                      g_object_set (cell, "visible", FALSE, NULL);
                  }
              }

            public:

              AlbumTreeView (Glib::RefPtr<Gnome::Glade::Xml> const& xml, MPX::Library &lib, MPX::Amazon::Covers &amzn)
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-albums")
              , m_Lib(lib)
              , m_AMZN(amzn)
              {
                m_Lib.signal_new_album().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_album ));
                m_Lib.signal_new_track().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_track ));
                m_AMZN.signal_got_cover().connect( sigc::mem_fun( *this, &AlbumTreeView::on_got_cover ));

                set_show_expanders( false );
                set_level_indentation( 32 );

                TreeViewColumn * col = manage (new TreeViewColumn());

                GtkCellRenderer * renderer = gossip_cell_renderer_expander_new ();
                gtk_tree_view_column_pack_start (col->gobj(), renderer, FALSE);
                gtk_tree_view_column_set_cell_data_func (col->gobj(),
                                     renderer,
                                     GtkTreeCellDataFunc(rb_sourcelist_expander_cell_data_func),
                                     this,
                                     NULL);

                CellRendererCairoSurface * cellcairo = manage (new CellRendererCairoSurface);
                col->pack_start(*cellcairo, false);
                col->set_cell_data_func(*cellcairo, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncCover ));
                cellcairo->property_xpad() = 4;
                cellcairo->property_ypad() = 2;
                cellcairo->property_yalign() = 0.;

                CellRendererText *celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncText1 ));
                celltext->property_yalign() = 0.;
                celltext->property_ypad() = 2;

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncText2 ));
                celltext->property_xalign() = 1.;

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncText3 ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncText4 ));

                append_column(*col);

                TreeStore = Gtk::TreeStore::create(AlbumColumns);
                set_model(TreeStore);
                TreeStore->set_default_sort_func(sigc::mem_fun( *this, &AlbumTreeView::slotSort ));
                TreeStore->set_sort_column(GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

                set_enable_tree_lines();

                m_DiscDefault_Pixbuf = Gdk::Pixbuf::create_from_file(build_filename(DATA_DIR, build_filename("images","disc-default.png")));
                m_DiscDefault = Util::cairo_image_surface_from_pixbuf(m_DiscDefault_Pixbuf->scale_simple(64,64,Gdk::INTERP_BILINEAR));

                album_list_load ();

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-album", TARGET_SAME_APP, 0x80));
                drag_source_set(Entries); 
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
