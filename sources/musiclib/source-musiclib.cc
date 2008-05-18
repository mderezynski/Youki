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
#include <glib.h>
#include <libglademm.h>
#include <Python.h>
#define NO_IMPORT
#include <pygobject.h>
#include <boost/format.hpp>

#include "mpx/hal.hh"
#include "mpx/library.hh"
#include "mpx/types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgetloader.h"
#include "mpx/playlistparser-xspf.hh"
#include "mpx/widgets/cell-renderer-cairo-surface.hh"
#include "mpx/widgets/cell-renderer-vbox.hh"
#include "mpx/widgets/gossip-cell-renderer-expander.h"

#include "source-musiclib.hh"

using namespace Gtk;
using namespace Glib;
using namespace Gnome::Glade;
using namespace MPX;
using boost::get;

namespace
{
    const char ACTION_CLEAR [] = "action-clear";
    const char ACTION_REMOVE_ITEMS [] = "action-remove-items";
    const char ACTION_REMOVE_REMAINING [] = "action-remove-remaining";
    const char ACTION_PLAY [] = "action-play";
    const char ACTION_GO_TO_ALBUM [] = "action-go-to-album";
    
    const char ui_playlist_popup [] =
    "<ui>"
    ""
    "<menubar name='popup-playlist-list'>"
    ""
    "   <menu action='dummy' name='menu-playlist-list'>"
    "       <menuitem action='action-play' name='action-play'/>"
    "       <menuitem action='action-remove-remaining'/>"
    "     <separator/>"
    "       <menuitem action='action-go-to-album'/>"
    "     <separator/>"
    "       <menuitem action='action-remove-items'/>"
    "       <menuitem action='action-clear'/>"
    "   </menu>"
    ""
    "</menubar>"
    ""
    "</ui>";

    char const * ui_source =
    "<ui>"
    ""
    "<menubar name='MenubarMain'>"
    "   <placeholder name='placeholder-source'>"
    "     <menu action='menu-source-musiclib'>"
    "         <menuitem action='musiclib-sort-by-name'/>"
    "         <menuitem action='musiclib-sort-by-date'/>"
    "         <menuitem action='musiclib-sort-by-rating'/>"
    "         <separator/>"
    "         <menuitem action='musiclib-new-playlist'/>"
    "     </menu>"
    "   </placeholder>"
    "</menubar>"
    ""
    "</ui>"
    "";

    std::string
    get_timestr_from_time_t (time_t atime)
    {
      struct tm atm, btm;
      localtime_r (&atime, &atm);
      time_t curtime = time(NULL);
      localtime_r (&curtime, &btm);

      static boost::format date_f ("%s, %s");

      if (atm.tm_year == btm.tm_year &&
      atm.tm_yday == btm.tm_yday)
      {
          char btime[64];
          strftime (btime, 64, "%H:00", &atm); // we just ASSUME that no podcast updates more than once an hour, for cleaner readbility

              return locale_to_utf8 ((date_f % _("Today") % btime).str());
      }
      else
      {
          char bdate[64];
          strftime (bdate, 64, "%d %b %Y", &atm);

          char btime[64];
          strftime (btime, 64, "%H:00", &atm); // we just ASSUME that no podcast updates more than once an hour, for cleaner readbility

              return locale_to_utf8 ((date_f % bdate % btime).str());
      }
    }

    enum Order
    {
        NO_ORDER,
        ORDER
    };
}
 
namespace MPX
{
    typedef std::vector<Gtk::TreePath> PathV;
    typedef std::vector<Gtk::TreeRowReference> ReferenceV;
    typedef ReferenceV::iterator ReferenceVIter;

    class ReferenceCollect
    {
      public:

        ReferenceCollect (Glib::RefPtr<Gtk::TreeModel> const& model,
                          ReferenceV & references)
        : m_model       (model)
        , m_references  (references)
        {} 

        void
        operator()(Gtk::TreePath const& p)
        {
          m_references.push_back (TreeRowReference (m_model, p));
        }

      private:
          
        Glib::RefPtr<Gtk::TreeModel> const& m_model; 
        ReferenceV & m_references;
    };  

    struct MusicLibPrivate
    {
        Glib::RefPtr<Gnome::Glade::Xml> m_RefXml;
        Gtk::Widget * m_UI;

        enum AlbumRowType
        {
            ROW_ALBUM   =   1,
            ROW_TRACK   =   2,
        };

        struct AlbumColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
            Gtk::TreeModelColumn<AlbumRowType>                          RowType;
            Gtk::TreeModelColumn<Cairo::RefPtr<Cairo::ImageSurface> >   Image;
            Gtk::TreeModelColumn<Glib::ustring>                         Text;

            Gtk::TreeModelColumn<bool>                                  HasTracks;

            Gtk::TreeModelColumn<std::string>                           AlbumSort;
            Gtk::TreeModelColumn<std::string>                           ArtistSort;

            Gtk::TreeModelColumn<gint64>                                Id;
            Gtk::TreeModelColumn<std::string>                           MBID;

            Gtk::TreeModelColumn<gint64>                                Date;
            Gtk::TreeModelColumn<gint64>                                InsertDate;
            Gtk::TreeModelColumn<gint64>                                Rating;

            Gtk::TreeModelColumn<Glib::ustring>                         TrackTitle;
            Gtk::TreeModelColumn<Glib::ustring>                         TrackArtist;
            Gtk::TreeModelColumn<gint64>                                TrackNumber;
            Gtk::TreeModelColumn<gint64>                                TrackLength;
            Gtk::TreeModelColumn<gint64>                                TrackId;

            AlbumColumnsT ()
            {
                add (RowType);
                add (HasTracks);
                add (Image);
                add (Text);
                add (AlbumSort);
                add (ArtistSort);
                add (MBID);
                add (Date);
                add (InsertDate);
                add (Id);
                add (Rating);
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
              typedef std::map<std::string, IterSet> MBIDIterMap;
              typedef std::map<gint64, TreeIter> IdIterMap; 

            public:

              Glib::RefPtr<Gtk::TreeStore> TreeStore;
              Glib::RefPtr<Gtk::TreeModelFilter> TreeStoreFilter;

            private:

              AlbumColumnsT AlbumColumns;
              PAccess<MPX::Library> m_Lib;
              PAccess<MPX::Covers> m_Covers;
              MPX::Source::PlaybackSourceMusicLib & m_MLib;

              MBIDIterMap m_MBIDIterMap;
              IdIterMap m_AlbumIterMap;

              Cairo::RefPtr<Cairo::ImageSurface> m_DiscDefault;
              Glib::RefPtr<Gdk::Pixbuf> m_DiscDefault_Pixbuf;
              Glib::RefPtr<Gdk::Pixbuf> m_Stars[6];

              boost::optional<std::string> m_DragMBID;
              boost::optional<gint64> m_DragAlbumId;
              boost::optional<gint64> m_DragTrackId;
              TreePath m_PathButtonPress;
              bool m_ButtonPressed;

              Gtk::Entry *m_FilterEntry;



              virtual void
              on_row_activated (const TreeModel::Path& path, TreeViewColumn* column)
              {
                TreeIter iter = TreeStore->get_iter (TreeStoreFilter->convert_path_to_child_path(path));
                if(path.get_depth() == ROW_ALBUM)
                {
                        gint64 id = (*iter)[AlbumColumns.Id];
                }
                else
                {
                        gint64 id = (*iter)[AlbumColumns.TrackId];
                        IdV v;
                        v.push_back(id);
                }
              }

              virtual void
              on_row_expanded (const TreeIter &iter_filter,
                               const TreePath &path) 
              {
                TreeIter iter = TreeStoreFilter->convert_iter_to_child_iter(iter_filter);
                if(!(*iter)[AlbumColumns.HasTracks])
                {
                    GtkTreeIter children;
                    bool has_children = (gtk_tree_model_iter_children(GTK_TREE_MODEL(TreeStore->gobj()), &children, const_cast<GtkTreeIter*>(iter->gobj())));

                    SQL::RowV v;
                    m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track;") % gint64((*iter)[AlbumColumns.Id])).str());

                    for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                    {
                        SQL::Row & r = *i;
                        TreeIter child = TreeStore->append(iter->children());
                        (*child)[AlbumColumns.TrackTitle] = get<std::string>(r["title"]);
                        (*child)[AlbumColumns.TrackArtist] = get<std::string>(r["artist"]);
                        (*child)[AlbumColumns.TrackNumber] = get<gint64>(r["track"]);
                        (*child)[AlbumColumns.TrackLength] = get<gint64>(r["time"]);
                        (*child)[AlbumColumns.TrackId] = get<gint64>(r["id"]);
                        (*child)[AlbumColumns.RowType] = ROW_TRACK; 
                    }

                    if(v.size())
                    {
                        (*iter)[AlbumColumns.HasTracks] = true;
                        if(has_children)
                        {
                            gtk_tree_store_remove(GTK_TREE_STORE(TreeStore->gobj()), &children);
                        } 
                        else
                            g_warning("%s:%d : No placeholder row present, state seems corrupted.", __FILE__, __LINE__);
                    }

                }
                scroll_to_row (path, 0.);
              }

              virtual void
              on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, SelectionData& selection_data, guint info, guint time)
              {
                if(m_DragAlbumId)
                {
                    gint64 * id = new gint64(m_DragAlbumId.get());
                    selection_data.set("mpx-album", 8, reinterpret_cast<const guint8*>(id), 8);
                }
                else if(m_DragTrackId)
                {
                    gint64 * id = new gint64(m_DragTrackId.get());
                    selection_data.set("mpx-track", 8, reinterpret_cast<const guint8*>(id), 8);
                }

                m_DragMBID.reset();
                m_DragAlbumId.reset();
                m_DragTrackId.reset();
              }

              virtual void
              on_drag_begin (const Glib::RefPtr<Gdk::DragContext>& context) 
              {
                if(m_DragAlbumId)
                {
                    if(m_DragMBID)
                    {
                        Cairo::RefPtr<Cairo::ImageSurface> CoverCairo;
                        m_Covers.get().fetch(m_DragMBID.get(), CoverCairo, COVER_SIZE_DEFAULT);
                        if(CoverCairo)
                        {
                            CoverCairo = Util::cairo_image_surface_round(CoverCairo, 21.3); 
                            Glib::RefPtr<Gdk::Pixbuf> CoverPixbuf = Util::cairo_image_surface_to_pixbuf(CoverCairo);
                            drag_source_set_icon(CoverPixbuf->scale_simple(128,128,Gdk::INTERP_BILINEAR));
                            return;
                        }
                    }

                    drag_source_set_icon(m_DiscDefault_Pixbuf->scale_simple(128,128,Gdk::INTERP_BILINEAR));
                }
                else
                {
                    Glib::RefPtr<Gdk::Pixmap> pix = create_row_drag_icon(m_PathButtonPress);
                    drag_source_set_icon(pix->get_colormap(), pix, Glib::RefPtr<Gdk::Bitmap>(0));
                }
              }

              virtual bool
              on_motion_notify_event (GdkEventMotion *event)
              {
                int x_orig, y_orig;
                GdkModifierType state;

                if(!g_atomic_int_get(&m_ButtonPressed))
                    return false;

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

                TreePath path;              
                TreeViewColumn *col;
                int cell_x, cell_y;
                if(get_path_at_pos (x_orig, y_orig, path, col, cell_x, cell_y))
                {
                    TreeIter iter = TreeStore->get_iter(TreeStoreFilter->convert_path_to_child_path(path));
                    
                    if( (cell_x >= 102) && (cell_x <= 162) && (cell_y >= 65) && (cell_y <=78))
                    {
                        int rating = ((cell_x - 102)+7) / 12;
                        (*iter)[AlbumColumns.Rating] = rating;  
                        m_Lib.get().albumRated(m_DragAlbumId.get(), rating);
                    }
                }

                return false;
             } 

              virtual bool
              on_button_release_event (GdkEventButton* event)
              {
                g_atomic_int_set(&m_ButtonPressed, 0);
                return false;
              }

              virtual bool
              on_button_press_event (GdkEventButton* event)
              {
                int cell_x, cell_y ;
                TreeViewColumn *col ;

                if(get_path_at_pos (event->x, event->y, m_PathButtonPress, col, cell_x, cell_y))
                {
                    TreeIter iter = TreeStore->get_iter(TreeStoreFilter->convert_path_to_child_path(m_PathButtonPress));
                    if(m_PathButtonPress.get_depth() == ROW_ALBUM)
                    {
                        m_DragMBID = (*iter)[AlbumColumns.MBID];
                        m_DragAlbumId = (*iter)[AlbumColumns.Id];
                        m_DragTrackId.reset(); 
                
                        g_atomic_int_set(&m_ButtonPressed, 1);

                        if( (cell_x >= 102) && (cell_x <= 162) && (cell_y >= 65) && (cell_y <=78))
                        {
                            int rating = ((cell_x - 102)+7) / 12;
                            (*iter)[AlbumColumns.Rating] = rating;  
                            m_Lib.get().albumRated(m_DragAlbumId.get(), rating);
                        }
                    }
                    else
                    if(m_PathButtonPress.get_depth() == ROW_TRACK)
                    {
                        m_DragMBID.reset(); 
                        m_DragAlbumId.reset();
                        m_DragTrackId = (*iter)[AlbumColumns.TrackId];
                    }
                }
                TreeView::on_button_press_event(event);
                return false;
              }

              void
              on_got_cover(const Glib::ustring& mbid)
              {
                Cairo::RefPtr<Cairo::ImageSurface> Cover;
                m_Covers.get().fetch(mbid, Cover, COVER_SIZE_ALBUM_LIST);
                Util::cairo_image_surface_border(Cover, 1.);
                Cover = Util::cairo_image_surface_round(Cover, 6.);
                IterSet & set = m_MBIDIterMap[mbid];
                for(IterSet::iterator i = set.begin(); i != set.end(); ++i)
                {
                    (*(*i))[AlbumColumns.Image] = Cover;
                }
              }

              void
              place_album (SQL::Row & r, gint64 id, bool acquire = false)
              {
                TreeIter iter = TreeStore->append();
                m_AlbumIterMap.insert(std::make_pair(id, iter));
                TreeStore->append(iter->children()); //create dummy/placeholder row for tracks

                (*iter)[AlbumColumns.RowType] = ROW_ALBUM; 
                (*iter)[AlbumColumns.HasTracks] = false; 
                (*iter)[AlbumColumns.Image] = m_DiscDefault; 
                (*iter)[AlbumColumns.Id] = id; 

                if(r.count("album_rating"))
                {
                    gint64 rating = get<gint64>(r["album_rating"]);
                    (*iter)[AlbumColumns.Rating] = rating;
                }

                gint64 idate = 0;
                if(r.count("album_insert_date"))
                {
                    idate = get<gint64>(r["album_insert_date"]);
                    (*iter)[AlbumColumns.InsertDate] = idate;
                }

                std::string asin;
                if(r.count("amazon_asin"))
                {
                    asin = get<std::string>(r["amazon_asin"]);
                }

                if(r.count("mb_album_id"))
                {
                    std::string mbid = get<std::string>(r["mb_album_id"]);
                    IterSet & s = m_MBIDIterMap[mbid];
                    s.insert(iter);
                    m_Covers.get().cache( mbid, "", asin, true ); 
                    (*iter)[AlbumColumns.MBID] = mbid; 
                }

                std::string date; 
                if(r.count("mb_release_date"))
                {
                    date = get<std::string>(r["mb_release_date"]);
                    if(date.size())
                    {
                        gint64 date_int;
                        sscanf(date.c_str(), "%04lld", &date_int);
                        (*iter)[AlbumColumns.Date] = date_int; 
                        date = date.substr(0,4) + ", ";
                    }
                }
                else
                    (*iter)[AlbumColumns.Date] = 0; 

                std::string ArtistSort;
                if(r.find("album_artist_sortname") != r.end())
                    ArtistSort = get<std::string>(r["album_artist_sortname"]);
                else
                    ArtistSort = get<std::string>(r["album_artist"]);

                (*iter)[AlbumColumns.Text] = (boost::format("<span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3%Added: %4%</span>") % Markup::escape_text(get<std::string>(r["album"])).c_str() % Markup::escape_text(ArtistSort).c_str() % date % get_timestr_from_time_t(idate)).str();
                (*iter)[AlbumColumns.AlbumSort] = ustring(get<std::string>(r["album"])).collate_key();
                (*iter)[AlbumColumns.ArtistSort] = ustring(ArtistSort).collate_key();

                gint count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(TreeStore->gobj()), NULL);
                m_MLib.Signals.Items.emit(count);
              } 
   
              void
              album_list_load ()
              {
                g_message("%s: Reloading.", G_STRLOC);

                TreeStore->clear ();
                m_MBIDIterMap.clear();
                m_AlbumIterMap.clear();

                SQL::RowV v;
                m_Lib.get().getSQL(v, "SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id;");

                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                {
                    SQL::Row & r = *i; 
                    place_album(r, get<gint64>(r["id"]));
                }
              }

              void
              on_new_album(gint64 id)
              {
                SQL::RowV v;
                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album.id = %lld;") % id).str());

                g_return_if_fail(!v.empty());

                SQL::Row & r = v[0];

                place_album (r, id, true); 
              }

              void
              on_new_track(Track & track, gint64 album_id, gint64 artist_id)
              {
                if(m_AlbumIterMap.count(album_id))
                {
                    TreeIter iter = m_AlbumIterMap[album_id];
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

                        (*child)[AlbumColumns.TrackId] = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get());
                        (*child)[AlbumColumns.RowType] = ROW_TRACK; 
                    }
                }
                else
                    g_warning("%s: Got new track without associated album! Consistency error!", G_STRLOC);
              }

              int
              slotSortRating(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[AlbumColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[AlbumColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[AlbumColumns.Rating];
                  gint64 alb_b = (*iter_b)[AlbumColumns.Rating];

                  return alb_b - alb_a;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[AlbumColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[AlbumColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
              }

              int
              slotSortAlpha(const TreeIter& iter_a, const TreeIter& iter_b)
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

                return 0;
              }

              int
              slotSortDate(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[AlbumColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[AlbumColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[AlbumColumns.InsertDate];
                  gint64 alb_b = (*iter_b)[AlbumColumns.InsertDate];

                  return alb_b - alb_a;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[AlbumColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[AlbumColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
              }


              void
              cellDataFuncCover (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererCairoSurface *cell = dynamic_cast<CellRendererCairoSurface*>(basecell);
                if(path.get_depth() == ROW_ALBUM)
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
                CellRendererVBox *cvbox = dynamic_cast<CellRendererVBox*>(basecell);
                CellRendererText *cell1 = dynamic_cast<CellRendererText*>(cvbox->property_renderer1().get_value());
                CellRendererPixbuf *cell2 = dynamic_cast<CellRendererPixbuf*>(cvbox->property_renderer2().get_value());
                if(path.get_depth() == ROW_ALBUM)
                {
                    cvbox->property_visible() = true; 

                    if(cell1)
                        cell1->property_markup() = (*iter)[AlbumColumns.Text]; 
                    if(cell2)
                    {
                        gint64 i = ((*iter)[AlbumColumns.Rating]);
                        g_return_if_fail((i >= 0) && (i <= 5));
                        cell2->property_pixbuf() = m_Stars[i];
                    }
                }
                else
                {
                    cvbox->property_visible() = false; 
                }
              }

              void
              cellDataFuncText2 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = (boost::format("%lld.") % (*iter)[AlbumColumns.TrackNumber]).str();
                }
                else
                {
                    cell->property_visible() = false; 
                }
              }

              void
              cellDataFuncText3 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = Markup::escape_text((*iter)[AlbumColumns.TrackTitle]);
                }
                else
                {
                    cell->property_visible() = false; 
                }
              }

              void
              cellDataFuncText4 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    gint64 Length = (*iter)[AlbumColumns.TrackLength];
                    cell->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
                }
                else
                {
                    cell->property_visible() = false; 
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

              bool
              albumVisibleFunc (TreeIter const& iter)
              {
                  ustring filter (ustring (m_FilterEntry->get_text()).lowercase());
                  TreePath path (TreeStore->get_path(iter));
                  if( filter.empty() || path.get_depth() > 1)
                    return true;
                  else
                    return (Util::match_keys (ustring((*iter)[AlbumColumns.Text]).lowercase(), filter)); 
              }

            public:

              void
              go_to_album(gint64 id)
              {
                if(m_AlbumIterMap.count(id))
                {
                    TreeIter iter = m_AlbumIterMap.find(id)->second;
                    scroll_to_row (TreeStore->get_path(iter), 0.);
                }
              }


              AlbumTreeView (
                    Glib::RefPtr<Gnome::Glade::Xml> const& xml,    
                    PAccess<MPX::Library>           const& lib,
                    PAccess<MPX::Covers>            const& amzn,
                    MPX::Source::PlaybackSourceMusicLib  & mlib
              )
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-albums")
              , m_Lib(lib)
              , m_Covers(amzn)
              , m_MLib(mlib)
              , m_ButtonPressed(false)
              {
                for(int n = 0; n < 6; ++n)
                {
                    m_Stars[n] =

                    Gdk::Pixbuf::create_from_file(
                        build_filename(
                            build_filename(
                                DATA_DIR,
                                "images"
                            ),
                            (boost::format("stars%d.png") % n).str()
                        )
                    );

                }

                m_Lib.get().signal_new_album().connect(
                    sigc::mem_fun(
                        *this, &AlbumTreeView::on_new_album
                ));

                m_Lib.get().signal_new_track().connect(
                    sigc::mem_fun(
                        *this, &AlbumTreeView::on_new_track
                ));

                m_Lib.get().signal_reload().connect(
                    sigc::mem_fun(
                        *this, &AlbumTreeView::album_list_load
                ));

                m_Covers.get().signal_got_cover().connect(
                    sigc::mem_fun(
                        *this, &AlbumTreeView::on_got_cover
                ));

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
                cellcairo->property_ypad() = 4;
                cellcairo->property_yalign() = 0.;
                cellcairo->property_xalign() = 0.;

                CellRendererVBox *cvbox = manage (new CellRendererVBox);

                CellRendererText *celltext = manage (new CellRendererText);
                celltext->property_yalign() = 0.;
                celltext->property_ypad() = 4;
                celltext->property_height() = 52;
                celltext->property_ellipsize() = Pango::ELLIPSIZE_MIDDLE;
                cvbox->property_renderer1() = celltext;

                CellRendererPixbuf *cellpixbuf = manage (new CellRendererPixbuf);
                cellpixbuf->property_xalign() = 0.;
                cellpixbuf->property_ypad() = 2;
                cellpixbuf->property_xpad() = 2;
                cvbox->property_renderer2() = cellpixbuf;

                col->pack_start(*cvbox, true);
                col->set_cell_data_func(*cvbox, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncText1 ));


                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncText2 ));
                celltext->property_xalign() = 1.;

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncText3 ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext,
                    sigc::mem_fun(
                        *this, &AlbumTreeView::cellDataFuncText4
                ));

                append_column(*col);

                TreeStore = Gtk::TreeStore::create(AlbumColumns);
                TreeStoreFilter = Gtk::TreeModelFilter::create(TreeStore);
                TreeStoreFilter->set_visible_func(
                    sigc::mem_fun(
                        *this, &AlbumTreeView::albumVisibleFunc
                ));

                set_model(TreeStoreFilter);
                TreeStore->set_sort_func(0, sigc::mem_fun( *this, &AlbumTreeView::slotSortAlpha ));
                TreeStore->set_sort_func(1, sigc::mem_fun( *this, &AlbumTreeView::slotSortDate ));
                TreeStore->set_sort_func(2, sigc::mem_fun( *this, &AlbumTreeView::slotSortRating ));
                TreeStore->set_sort_column(0, Gtk::SORT_ASCENDING);

                xml->get_widget("album-filter-entry", m_FilterEntry);
                m_FilterEntry->signal_changed().connect(
                    sigc::mem_fun(TreeStoreFilter.operator->(), &TreeModelFilter::refilter
                ));

                m_DiscDefault_Pixbuf = Gdk::Pixbuf::create_from_file(
                        build_filename(
                            DATA_DIR,
                            build_filename(
                                "images",
                                "disc-default.png"
                            )
                        )
                );

                m_DiscDefault = Util::cairo_image_surface_from_pixbuf(
                        m_DiscDefault_Pixbuf->scale_simple(
                            72,
                            72,
                            Gdk::INTERP_BILINEAR
                        )
                );

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-album", TARGET_SAME_APP, 0x80));
                Entries.push_back(TargetEntry("mpx-track", TARGET_SAME_APP, 0x81));
                drag_source_set(Entries); 

                album_list_load ();
              }
        };

        AlbumTreeView           * m_TreeViewAlbums;
        PAccess<MPX::Library>     m_Lib;
        PAccess<MPX::Covers>      m_Covers;
        PAccess<MPX::HAL>         m_HAL;

        MusicLibPrivate (MPX::Player & player, MPX::Source::PlaybackSourceMusicLib & mlib)
        {
            const std::string path (build_filename(DATA_DIR, build_filename("glade","source-musiclib.glade")));
            m_RefXml = Gnome::Glade::Xml::create (path);
            m_UI = m_RefXml->get_widget("source-musiclib");
            player.get_object(m_Lib);
            player.get_object(m_Covers);
            player.get_object(m_HAL);
            m_TreeViewAlbums = new AlbumTreeView(m_RefXml, m_Lib, m_Covers, mlib);
        }
    };
}

namespace MPX
{
namespace Source
{
    PlaybackSourceMusicLib::PlaybackSourceMusicLib (
            Glib::RefPtr<Gtk::UIManager>  const& ui_manager,
            MPX::Player                        & obj_player
    )
    : PlaybackSource(ui_manager, _("Music"), C_CAN_SEEK)
    , m_MainUIManager(ui_manager)
    , m_Player(obj_player)
    , m_PlaylistID(0)
    {
        m_Player.get_object(m_Lib);
        m_Player.get_object(m_HAL);
        m_Player.get_object(m_Covers);

        m_Private = new MusicLibPrivate(m_Player,*this);

        m_MainActionGroup = ActionGroup::create("ActionsMusicLib");
        m_MainActionGroup->add(Action::create("menu-source-musiclib", _("Music _Library")));

        Gtk::RadioButtonGroup group;

        m_MainActionGroup->add(

            RadioAction::create(
                group,
                "musiclib-sort-by-name",
                "Sort Albums By Artist/Album"
            ),

            sigc::mem_fun(
                *this, &PlaybackSourceMusicLib::on_sort_column_change
            )
        );

        m_MainActionGroup->add(

            RadioAction::create(
                group,
                "musiclib-sort-by-date",
                "Sort Albums By Date"
            ),

            sigc::mem_fun(
                *this, &PlaybackSourceMusicLib::on_sort_column_change
            )
        );

        m_MainActionGroup->add(

            RadioAction::create(
                group,
                "musiclib-sort-by-rating",
                "Sort Albums By Rating"
            ),

            sigc::mem_fun(
                *this, &PlaybackSourceMusicLib::on_sort_column_change
            )
        );

        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-sort-by-name"))->property_value() = 0;
        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-sort-by-date"))->property_value() = 1;
        RefPtr<Gtk::RadioAction>::cast_static(m_MainActionGroup->get_action("musiclib-sort-by-rating"))->property_value() = 2;

        m_MainActionGroup->add(

            Action::create(
                "musiclib-new-playlist",
                Gtk::Stock::NEW,
                _("New Playlist")
            ),

            sigc::mem_fun(
                *this, &PlaybackSourceMusicLib::on_new_playlist
            )
        );

        m_MainUIManager->insert_action_group(m_MainActionGroup);
    }

    void
    PlaybackSourceMusicLib::post_install ()
    {
    }

    PlaybackSourceMusicLib::~PlaybackSourceMusicLib ()
    {
        delete m_Private;
    }

    PyObject*
    PlaybackSourceMusicLib::get_py_obj ()
    {
        using namespace boost::python;
        object obj(boost::ref(this));
        Py_INCREF(obj.ptr());
        return obj.ptr();
    }

    std::string
    PlaybackSourceMusicLib::get_guid ()
    {
        return "8a98a167-3bf4-44af-859b-079efd6797b5";
    }

    std::string
    PlaybackSourceMusicLib::get_class_guid ()
    {
        return "ecd01fd1-9b4d-4fef-8e85-6c4b16965ef8";
    }

    guint
    PlaybackSourceMusicLib::add_menu ()
    {
        guint id = m_MainUIManager->add_ui_from_string(ui_source);
        return id;
    }

    void
    PlaybackSourceMusicLib::on_sort_column_change ()
    {
        int value = RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action ("musiclib-sort-by-name"))->get_current_value();
        m_Private->m_TreeViewAlbums->TreeStore->set_sort_column(value, Gtk::SORT_ASCENDING);    
    }

    void
    PlaybackSourceMusicLib::on_new_playlist ()
    {
        static boost::format format ("%s (%lld)"); 

        PlaybackSourcePlaylist * pl =
            new PlaybackSourcePlaylist(
                m_MainUIManager,
                m_Player,
                (format % (_("New Playlist")) % (m_PlaylistID+1)).str()
            );

        m_Playlists[m_PlaylistID] = pl;
        m_Player.add_subsource(pl, get_key(), m_PlaylistID);
        m_PlaylistID++;
    }

    std::string
    PlaybackSourceMusicLib::get_uri () 
    {
        return std::string();
    }

    std::string
    PlaybackSourceMusicLib::get_type ()
    {
        return std::string();
    }

    bool
    PlaybackSourceMusicLib::play ()
    {
        return false;
    }

    bool
    PlaybackSourceMusicLib::go_next ()
    {
        return false;
    }

    bool
    PlaybackSourceMusicLib::go_prev ()
    {
        return false;
    }

    void
    PlaybackSourceMusicLib::stop () 
    {
    }

    void
    PlaybackSourceMusicLib::check_caps ()
    {
    }

    void
    PlaybackSourceMusicLib::send_metadata ()
    {
    }

    void
    PlaybackSourceMusicLib::play_post () 
    {
    }

    void
    PlaybackSourceMusicLib::next_post () 
    { 
    }

    void
    PlaybackSourceMusicLib::prev_post ()
    {
    }

    void
    PlaybackSourceMusicLib::restore_context ()
    {}

    Glib::RefPtr<Gdk::Pixbuf>
    PlaybackSourceMusicLib::get_icon ()
    {
        try{
            return IconTheme::get_default()->load_icon("source-library", 64, ICON_LOOKUP_NO_SVG);
        } catch(...) { return Glib::RefPtr<Gdk::Pixbuf>(0); }
    }

    Gtk::Widget*
    PlaybackSourceMusicLib::get_ui ()
    {
        return m_Private->m_UI;
    }

    UriSchemes 
    PlaybackSourceMusicLib::get_uri_schemes ()
    {
        return UriSchemes();
    }

    void    
    PlaybackSourceMusicLib::process_uri_list (Util::FileList const& uris, bool play)
    {
    }

} // end namespace Source
} // end namespace MPX 
