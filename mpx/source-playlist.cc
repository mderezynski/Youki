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


#include "mpx/hal.hh"
#include "mpx/library.hh"
#include "mpx/playlistparser-xspf.hh"
#include "mpx/source-playlist.hh"
#include "mpx/types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgetloader.h"
#include "mpx/widgets/cell-renderer-cairo-surface.hh"
#include "mpx/widgets/cell-renderer-vbox.hh"
#include "mpx/widgets/gossip-cell-renderer-expander.h"

#include "glib-marshalers.h"

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
    
    const char ui_playlist_popup [] =
    "<ui>"
    ""
    "<menubar name='popup-playlist-list'>"
    ""
    "   <menu action='dummy' name='menu-playlist-list'>"
    "       <menuitem action='action-play' name='action-play'/>"
    "       <menuitem action='action-remove-remaining'/>"
    "     <separator/>"
    "       <menuitem action='action-remove-items'/>"
    "       <menuitem action='action-clear'/>"
    "   </menu>"
    ""
    "</menubar>"
    ""
    "</ui>";

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

    struct PlaylistPrivate
    {
        Gtk::ScrolledWindow * m_UI;

        class PlaylistTreeView
            :   public Gtk::TreeView
        {
              PAccess<MPX::Library> m_Lib;
              MPX::Source::PlaybackSourcePlaylist & m_Playlist;
              PAccess<MPX::HAL> m_HAL;

              Glib::RefPtr<Gdk::Pixbuf> m_Playing;
              gint64 m_RowId;

              TreePath m_PathButtonPress;
              int m_ButtonDepressed;

              Glib::RefPtr<Gtk::UIManager> m_UIManager;
              Glib::RefPtr<Gtk::ActionGroup> m_ActionGroup;

            public:

              boost::optional<Gtk::TreeIter> m_CurrentIter;
              boost::optional<Gtk::TreeIter> m_PlayInitIter;
              boost::optional<gint64> m_CurrentId;
              PlaylistColumnsT PlaylistColumns;
              Glib::RefPtr<Gtk::ListStore> ListStore;
              Glib::RefPtr<Gdk::Pixbuf> m_Stars[6];

              enum Column
              {
                C_PLAYING,
                C_TITLE,
                C_LENGTH,
                C_RATING,
                C_ARTIST,
                C_ALBUM,
                C_TRACK,
              };

              PlaylistTreeView(
                    PAccess<MPX::Library>           const& lib,
                    PAccess<MPX::HAL>               const& hal,
                    MPX::Source::PlaybackSourcePlaylist  & mlib
              )
              : ObjectBase("MPXPlaylistViewClass")
              , TreeView()
              , m_Lib(lib)
              , m_Playlist(mlib)
              , m_HAL(hal)
              , m_RowId(0)
              , m_ButtonDepressed(0)
              {
                set_has_tooltip();
                set_rules_hint();

                m_Playing = Gdk::Pixbuf::create_from_file(Glib::build_filename(Glib::build_filename(DATA_DIR,"images"),"play.png"));
                for(int n = 0; n < 6; ++n)
                {
                        m_Stars[n] = Gdk::Pixbuf::create_from_file(build_filename(build_filename(DATA_DIR,"images"), (boost::format("stars%d.png") % n).str()));
                }

                TreeViewColumn * col = manage (new TreeViewColumn(""));
                CellRendererPixbuf * cell = manage (new CellRendererPixbuf);
                col->pack_start(*cell, true);
                col->set_min_width(24);
                col->set_max_width(24);
                col->set_cell_data_func(*cell, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncPlaying ));
                append_column(*col);

                append_column(_("Title"), PlaylistColumns.Title);

                col = manage (new TreeViewColumn(_("Time")));
                CellRendererText * cell2 = manage (new CellRendererText);
                col->property_alignment() = 1.;
                col->pack_start(*cell2, true);
                col->set_cell_data_func(*cell2, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFunc ));
                col->set_sort_column_id(PlaylistColumns.Length);
                g_object_set(G_OBJECT(cell2->gobj()), "xalign", 1.0f, NULL);
                append_column(*col);

                col = manage (new TreeViewColumn(_("Rating")));
                cell = manage (new CellRendererPixbuf);
                col->pack_start(*cell, true);
                col->set_min_width(66);
                col->set_max_width(66);
                col->set_cell_data_func(*cell, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncRating ));
                append_column(*col);

                append_column(_("Artist"), PlaylistColumns.Artist);
                append_column(_("Album"), PlaylistColumns.Album);
                append_column(_("Track"), PlaylistColumns.Track);

                get_column(C_ARTIST)->set_sort_column_id(PlaylistColumns.Artist);
                get_column(C_TITLE)->set_sort_column_id(PlaylistColumns.Title);
                get_column(C_ALBUM)->set_sort_column_id(PlaylistColumns.Album);
                get_column(C_TRACK)->set_sort_column_id(PlaylistColumns.Track);


                for(int c = C_TITLE; c < C_TRACK; get_column(c++)->set_resizable(true));

                get_column(2)->set_resizable(false);

                ListStore = Gtk::ListStore::create(PlaylistColumns);
                ListStore->set_sort_func(PlaylistColumns.Artist,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByArtist ));
                ListStore->set_sort_func(PlaylistColumns.Album,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByAlbum ));
                ListStore->set_sort_func(PlaylistColumns.Track,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByTrack ));

                get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
                get_selection()->signal_changed().connect( sigc::mem_fun( *this, &PlaylistTreeView::on_selection_changed ) );

                /* FIXME: The problem here is it's the order of adding, not the
                 * order in the view */
                /*
                ListStore->set_default_sort_func(
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortById ));
                */

                set_headers_clickable();
                set_model(ListStore);

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-album"));
                Entries.push_back(TargetEntry("mpx-track"));
                drag_dest_set(Entries, DEST_DEFAULT_MOTION);
                drag_dest_add_uri_targets();

                m_UIManager = Gtk::UIManager::create();
                m_ActionGroup = Gtk::ActionGroup::create ("Actions_UiPartPlaylist-PlaylistList");

                m_ActionGroup->add  (Gtk::Action::create("dummy","dummy"));

                m_ActionGroup->add  (Gtk::Action::create (ACTION_PLAY,
                                    Gtk::StockID (GTK_STOCK_MEDIA_PLAY),
                                    _("Play")),
                                    sigc::mem_fun(mlib, &MPX::Source::PlaybackSourcePlaylist::action_cb_play));

                m_ActionGroup->add  (Gtk::Action::create (ACTION_CLEAR,
                                    Gtk::StockID (GTK_STOCK_CLEAR),
                                    _("Clear Playlist")),
                                    sigc::mem_fun( *this, &PlaylistTreeView::clear ));

                m_ActionGroup->add  (Gtk::Action::create (ACTION_REMOVE_ITEMS,
                                Gtk::StockID (GTK_STOCK_REMOVE),
                                _("Remove selected Tracks")),
                                sigc::mem_fun( *this, &PlaylistTreeView::action_cb_playlist_remove_items ));

                m_ActionGroup->add  (Gtk::Action::create (ACTION_REMOVE_REMAINING,
                                _("Remove remaining Tracks")),
                                sigc::mem_fun( *this, &PlaylistTreeView::action_cb_playlist_remove_remaining ));

                m_UIManager->insert_action_group (m_ActionGroup);
                m_UIManager->add_ui_from_string (ui_playlist_popup);

                Gtk::Widget * item = m_UIManager->get_widget("/ui/popup-playlist-list/menu-playlist-list/action-play");
                Gtk::Label * label = dynamic_cast<Gtk::Label*>(dynamic_cast<Gtk::Bin*>(item)->get_child());
                label->set_markup(_("<b>Play</b>"));

				//set_tooltip_text(_("Drag and drop albums, tracks and files here to add them to the playlist."));
              }

              virtual void
              scroll_to_track ()
              {
                if(m_CurrentIter)
                {
                    TreePath path = ListStore->get_path(m_CurrentIter.get());
                    scroll_to_row(path, 0.5);
                }
              }

              virtual void
              clear ()  
              {
                  ListStore->clear ();
                  m_CurrentIter.reset ();
                  m_Playlist.check_caps();
                  m_Playlist.send_caps ();
                  columns_autosize();
              }

              virtual void
              action_cb_playlist_remove_items ()
              {
                  PathV p = get_selection()->get_selected_rows();
                  ReferenceV v;
                  std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, v));
                  ReferenceVIter i;
                  for (i = v.begin() ; !v.empty() ; )
                  {
                    TreeIter iter = ListStore->get_iter (i->get_path());
                    if(m_CurrentIter && m_CurrentIter.get() == iter)
                    {
                      m_CurrentIter.reset();
                    }
                    ListStore->erase (iter);
                    i = v.erase (i);
                  }

                  m_Playlist.check_caps();
                  m_Playlist.send_caps ();
              }

              virtual void
              action_cb_playlist_remove_remaining ()
              {
                  PathV p; 
                  TreePath path = ListStore->get_path(m_CurrentIter.get());
                  path.next();
        
                  while( path.get_indices().data()[0] < ListStore->children().size() )
                  {
                    p.push_back(path);
                    path.next ();
                  }

                  ReferenceV v;
                  std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, v));

                  ReferenceVIter i;
                  for (i = v.begin() ; !v.empty() ; )
                  {
                    TreeIter iter = ListStore->get_iter (i->get_path());
                    ListStore->erase (iter);
                    i = v.erase (i);
                  }

                  m_Playlist.check_caps();
                  m_Playlist.send_caps ();
              }

              void
              on_selection_changed ()
              {
                  if(get_selection()->count_selected_rows() == 1)
                      m_Playlist.set_play();
                  else
                      m_Playlist.clear_play();
              }


              void
              place_track(SQL::Row & r, Gtk::TreeIter const& iter)
              {

                  if(r.count("artist"))
                      (*iter)[PlaylistColumns.Artist] = get<std::string>(r["artist"]); 
                  if(r.count("album"))
                      (*iter)[PlaylistColumns.Album] = get<std::string>(r["album"]); 
                  if(r.count("track"))
                      (*iter)[PlaylistColumns.Track] = guint64(get<gint64>(r["track"]));
                  if(r.count("title"))
                      (*iter)[PlaylistColumns.Title] = get<std::string>(r["title"]);
                  if(r.count("time"))
                      (*iter)[PlaylistColumns.Length] = guint64(get<gint64>(r["time"]));
                  if(r.count("mb_artist_id"))
                      (*iter)[PlaylistColumns.ArtistSort] = get<std::string>(r["mb_artist_id"]);
                  if(r.count("mb_album_id"))
                      (*iter)[PlaylistColumns.AlbumSort] = get<std::string>(r["mb_album_id"]);

                  if(r.count("id"))
                  {
                      (*iter)[PlaylistColumns.RowId] = get<gint64>(r["id"]); 
                      if(!m_CurrentIter && m_CurrentId && get<gint64>(r["id"]) == m_CurrentId.get())
                      {
                          m_CurrentIter = iter; 
                          queue_draw();
                      }
                  }

                  if(r.count("rating"))
                      (*iter)[PlaylistColumns.Rating] = get<gint64>(r["rating"]);

                  (*iter)[PlaylistColumns.Location] = get<std::string>(r["location"]); 
                  (*iter)[PlaylistColumns.MPXTrack] = m_Lib.get().sqlToTrack(r); 
                  (*iter)[PlaylistColumns.IsMPXTrack] = true; 
              }

              void
              place_track(MPX::Track & track, Gtk::TreeIter const& iter)
              {
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

                  if(track[ATTRIBUTE_RATING])
                      (*iter)[PlaylistColumns.Rating] = get<gint64>(track[ATTRIBUTE_RATING].get());

                  if(track[ATTRIBUTE_MPX_TRACK_ID])
                  {
                      (*iter)[PlaylistColumns.RowId] = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()); 
                      if(!m_CurrentIter && m_CurrentId && get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()) == m_CurrentId.get())
                      {
                            m_CurrentIter = iter; 
                            queue_draw();
                      }
                  }

                  if(track[ATTRIBUTE_LOCATION])
                      (*iter)[PlaylistColumns.Location] = get<std::string>(track[ATTRIBUTE_LOCATION].get());
                  else
                      g_warning("Warning, no location given for track; this is totally wrong and should never happen.");

                  (*iter)[PlaylistColumns.MPXTrack] = track; 
                  (*iter)[PlaylistColumns.IsMPXTrack] = track[ATTRIBUTE_MPX_TRACK_ID] ? true : false; 
              }

              void
              append_album (gint64 id)
              {
                  SQL::RowV v;
                  m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = '%lld'") % id).str()); 
                  TreeIter iter = ListStore->append();
                  for(SQL::RowV::const_iterator i = v.begin(); i != v.end(); ++i)
                  {
                      SQL::Row r = *i;
                      if(i != v.begin())
                        iter = ListStore->insert_after(iter);
                      place_track(r, iter); 
                  }
              }

              void          
              append_tracks (IdV const& tid_v, Order order = NO_ORDER)
              {
                  if(tid_v.empty())
                      return;

                  std::stringstream numbers;
                  IdV::const_iterator end = tid_v.end();
                  end--;

                  for(IdV::const_iterator i = tid_v.begin(); i != tid_v.end(); ++i)
                  {
                      numbers << *i;
                      if(i != end)
                          numbers << ",";
                  }

                  SQL::RowV v;
                  m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE id IN (%s) %s") % numbers.str() % ((order == ORDER) ? "ORDER BY track_view.track" : "")).str()); 

                  TreeIter iter = ListStore->append();

                  if(!m_CurrentIter)
                    m_PlayInitIter = iter;

                  for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                  {
                          SQL::Row & r = *i;
                          
                          if(i != v.begin())
                              iter = ListStore->insert_after(iter);

                          place_track(r, iter);
                  }
              } 

              void
              append_uris (const Util::FileList& uris, Gtk::TreeIter & iter, bool & begin)
              {
                  Util::FileList dirs;

                  for(Util::FileList::const_iterator i = uris.begin(); i != uris.end(); ++i)
                  {
                      if(file_test(filename_from_uri(*i), FILE_TEST_IS_DIR))
                      {
                          dirs.push_back(*i);
                          continue;
                      }

                      // Check for playlist types
                      if(Util::str_has_suffix_nocase(*i, "xspf"))
                      {
                          PlaylistParserXSPF p;
                          Util::FileList xspf_uris;
                          p.read(*i, xspf_uris); 
                          append_uris(xspf_uris, iter, begin);
                      }


                      Track track;
                      m_Lib.get().getMetadata(*i, track);

                      if(!begin)
                        iter = ListStore->insert_after(iter);

                      begin = false;

                      place_track(track, iter);
                  }

                  if(!dirs.empty())
                      for(Util::FileList::const_iterator i = dirs.begin(); i != dirs.end(); ++i)
                      {
                                              Util::FileList files;
                                              Util::collect_audio_paths(*i, files);
                                              append_uris(files,iter,begin);
                      }
              }

              virtual void
              on_drag_data_received (const Glib::RefPtr<Gdk::DragContext>&, int x, int y,
                  const SelectionData& data, guint, guint)
              {
                  ListStore->set_sort_column(GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

                  TreePath path;
                  TreeViewDropPosition pos;
                  TreeIter iter;

                  if( !get_dest_row_at_pos (x, y, path, pos) )
                  {
                    int cell_x, cell_y;
                    TreeViewColumn * col;
                    if(get_path_at_pos (x, y, path, col, cell_x, cell_y))
                      iter = ListStore->insert (ListStore->get_iter (path));
                    else
                      iter = ListStore->append ();
                  }
                  else
                  {
                    switch (pos)
                    {
                      case TREE_VIEW_DROP_BEFORE:
                      case TREE_VIEW_DROP_INTO_OR_BEFORE:
                        iter = ListStore->insert (ListStore->get_iter (path));
                        break;

                      case TREE_VIEW_DROP_AFTER:
                      case TREE_VIEW_DROP_INTO_OR_AFTER:
                        iter = ListStore->insert_after (ListStore->get_iter (path));
                        break;
                    }
                  }

                  if(data.get_data_type() == "mpx-album")
                  {
                      gint64 id = *(reinterpret_cast<const gint64*>(data.get_data()));

                      SQL::RowV v;
                      m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track;") % id).str()); 
                      for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                      {
                          SQL::Row & r = *i;
                          
                          if(i != v.begin())
                              iter = ListStore->insert_after(iter);

                          place_track(r, iter);
                     }
                  }
                  if(data.get_data_type() == "mpx-track")
                  {
                      gint64 id = *(reinterpret_cast<const gint64*>(data.get_data()));

                      SQL::RowV v;
                      m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE id = %lld;") % id).str()); 
                      for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                      {
                          SQL::Row & r = *i;

                          if(i != v.begin())
                              iter = ListStore->insert_after(iter);

                          place_track(r, iter);
                      }
                  }
                  else
                  {
                      bool begin = true;
                      Util::FileList uris = data.get_uris();
                      append_uris (uris, iter, begin);
                  }

                  m_Playlist.check_caps ();
                  m_Playlist.send_caps ();
              }

              virtual bool
              on_button_press_event (GdkEventButton* event)
              {
                  int cell_x, cell_y ;
                  TreeViewColumn *col ;
                  TreePath path;

                  if(get_path_at_pos (event->x, event->y, path, col, cell_x, cell_y))
                  {
                      if(col == get_column(3))
                      {
                          int rating = (cell_x + 7) / 12;
                          g_return_val_if_fail(((rating >= 0) && (rating <= 5)), false);
                          TreeIter iter = ListStore->get_iter(path);
                          (*iter)[PlaylistColumns.Rating] = rating;   
                          m_Lib.get().trackRated(gint64((*iter)[PlaylistColumns.RowId]), rating);
                      }
                  }
                  TreeView::on_button_press_event(event);
                  return false;
              }

              bool
              on_drag_motion (RefPtr<Gdk::DragContext> const& context, int x, int y, guint time)
              {
                  TreeModel::Path path;
                  TreeViewDropPosition pos;

                  if( get_dest_row_at_pos (x, y, path, pos) )
                  {
                    switch (pos)
                    {
                        case TREE_VIEW_DROP_INTO_OR_BEFORE:
                        case TREE_VIEW_DROP_BEFORE:
                          set_drag_dest_row (path, TREE_VIEW_DROP_BEFORE);
                          break;

                        case TREE_VIEW_DROP_INTO_OR_AFTER:
                        case TREE_VIEW_DROP_AFTER:
                          set_drag_dest_row (path, TREE_VIEW_DROP_AFTER);
                          break;
                    }
                  }
                  
                  return true;
              }

              virtual bool
              on_event (GdkEvent * ev)
              {
                  if ((ev->type == GDK_BUTTON_RELEASE) ||
                      (ev->type == GDK_2BUTTON_PRESS) ||
                      (ev->type == GDK_3BUTTON_PRESS))
                  {
                    m_ButtonDepressed = false;
                    return false;
                  }
                  else
                  if( ev->type == GDK_BUTTON_PRESS )
                  {
                    GdkEventButton * event = reinterpret_cast <GdkEventButton *> (ev);

                    if( event->button == 1 && (event->window == get_bin_window()->gobj()))
                    {
                      TreeViewColumn      * column;
                      int                   cell_x,
                                            cell_y;
                      int                   tree_x,
                                            tree_y;
                      TreePath              path;

                      tree_x = event->x; tree_y = event->y;

                      m_ButtonDepressed  = get_path_at_pos (tree_x, tree_y, path, column, cell_x, cell_y) ;
                      if(m_ButtonDepressed)
                      {
                          m_PathButtonPress = path;  
                      }
                    }
                    else
                    if( event->button == 3 )
                    {
                      m_ActionGroup->get_action (ACTION_CLEAR)->set_sensitive
                        (ListStore->children().size());

                      m_ActionGroup->get_action (ACTION_REMOVE_ITEMS)->set_sensitive
                        (get_selection()->count_selected_rows());

                      m_ActionGroup->get_action (ACTION_REMOVE_REMAINING)->set_sensitive
                        (m_CurrentIter);

                      m_ActionGroup->get_action (ACTION_PLAY)->set_sensitive
                        (get_selection()->count_selected_rows());

                      Gtk::Menu * menu = dynamic_cast < Gtk::Menu* > (Util::get_popup (m_UIManager, "/popup-playlist-list/menu-playlist-list"));
                      if (menu) // better safe than screwed
                      {
                        menu->popup (event->button, event->time);
                      }
                      return true;
                    }
                  }
                  return false;
              }

              virtual bool
              on_motion_notify_event (GdkEventMotion * event)
              {
                  TreeView::on_motion_notify_event (event);

                  if( m_ButtonDepressed && event->window == get_bin_window()->gobj())
                  {
                    TreeViewColumn    * column;
                    TreeModel::Path     path;

                    int                 cell_x,
                                        cell_y;

                    int                 tree_x,
                                        tree_y;

                    tree_x = event->x;
                    tree_y = event->y;
                    if( get_path_at_pos (tree_x, tree_y, path, column, cell_x, cell_y) )
                    {
                      if( path != m_PathButtonPress )
                      {
                        if( path < m_PathButtonPress )
                          move_selected_rows_up ();
                        else
                        if( path > m_PathButtonPress )
                          move_selected_rows_down ();

                        m_PathButtonPress = path;
                      }
                    }
                  }
                  return false;
              }

              void
              move_selected_rows_up ()
              {
                  PathV p = get_selection()->get_selected_rows();

                  if( p.empty() )
                    return;

                  if( p[0].get_indices().data()[0] < 1 )
                    return; // we don't move rows if the uppermost would be pushed out of the list

                  ReferenceV r;
                  std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, r));
                  ReferenceVIter i = r.begin();

                  ListStore->set_sort_column(GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

                  for ( ; !r.empty() ; )
                  {
                    TreeModel::Path path_a (i->get_path());
                    TreeModel::Path path_b (i->get_path());
                    path_a.prev();
                    path_b.next();
                    TreeModel::iterator iter_a (ListStore->get_iter (path_a));
                    TreeModel::iterator iter_b (ListStore->get_iter (path_b));

                    if( G_UNLIKELY( TreeNodeChildren::size_type( path_b.get_indices().data()[0] ) == ListStore->children().size()) )
                    {
                      ListStore->iter_swap (iter_a, ListStore->get_iter (i->get_path()));
                    }
                    else
                    {
                      ListStore->move (iter_a, iter_b);
                    }

                    i = r.erase (i);
                  }
              }

              void
              move_selected_rows_down ()
              {
                  PathV p = get_selection()->get_selected_rows();
                
                  if( p.empty() )
                    return;

                  if( TreeNodeChildren::size_type( p[p.size()-1].get_indices().data()[0] + 1 ) == (ListStore->children().size()) )
                    return; // we don't move rows if the LOWER-most would be pushed out of the list, either

                  ReferenceV r;
                  std::for_each (p.begin(), p.end(), ReferenceCollect (ListStore, r));
                  ReferenceVIter i = r.begin();

                  ListStore->set_sort_column(GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

                  for ( ; !r.empty() ; )
                  {
                    TreeModel::Path path_a (i->get_path());
                    TreeModel::Path path_b (i->get_path());
                    path_b.next();
                    TreeModel::iterator iter_a (ListStore->get_iter (path_a));
                    TreeModel::iterator iter_b (ListStore->get_iter (path_b));

                    ListStore->move (iter_b, iter_a);
                    i = r.erase (i);
                  }
              }

              void
              on_drag_leave (RefPtr<Gdk::DragContext> const& context, guint time)
              {
                  gtk_tree_view_set_drag_dest_row (gobj(), NULL, GTK_TREE_VIEW_DROP_BEFORE);
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
                  g_object_set(G_OBJECT(cell_t->gobj()), "xalign", 1.0f, NULL);
                  cell_t->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
              }

              void
              cellDataFuncPlaying (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                  CellRendererPixbuf *cell_p = dynamic_cast<CellRendererPixbuf*>(basecell);
                  if(m_CurrentIter && m_CurrentIter.get() == iter)
                      cell_p->property_pixbuf() = m_Playing;
                  else
                      cell_p->property_pixbuf() = Glib::RefPtr<Gdk::Pixbuf>(0);
              }

              void
              cellDataFuncRating (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                  CellRendererPixbuf *cell_p = dynamic_cast<CellRendererPixbuf*>(basecell);
                  if(!(*iter)[PlaylistColumns.IsMPXTrack])
                  {
                      cell_p->property_sensitive() = false; 
                      cell_p->property_pixbuf() = Glib::RefPtr<Gdk::Pixbuf>(0);
                  }
                  else
                  {
                      cell_p->property_sensitive() = true; 
                      gint64 i = ((*iter)[PlaylistColumns.Rating]);
                      g_return_if_fail((i >= 0) && (i <= 5));
                      cell_p->property_pixbuf() = m_Stars[i];
                  }
              }

              int
              slotSortById(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  guint64 id_a = (*iter_a)[PlaylistColumns.RowId];
                  guint64 id_b = (*iter_b)[PlaylistColumns.RowId];
      
                  return (id_a - id_b); // FIXME: int overflow
              }

              int
              slotSortByTrack(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  ustring alb_a = (*iter_a)[PlaylistColumns.AlbumSort];
                  ustring alb_b = (*iter_b)[PlaylistColumns.AlbumSort];
                  guint64 trk_a = (*iter_a)[PlaylistColumns.Track];
                  guint64 trk_b = (*iter_b)[PlaylistColumns.Track];

                  if(alb_a != alb_b)
                      return 0;

                  return (trk_a - trk_b); // FIXME: int overflow
              }

              int
              slotSortByAlbum(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  ustring arts_a = (*iter_a)[PlaylistColumns.ArtistSort];
                  ustring arts_b = (*iter_b)[PlaylistColumns.ArtistSort];
                  ustring alb_a = (*iter_a)[PlaylistColumns.Album];
                  ustring alb_b = (*iter_b)[PlaylistColumns.Album];

                  return alb_a.compare(alb_b); 
              }

              int
              slotSortByArtist(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  ustring art_a = (*iter_a)[PlaylistColumns.Artist];
                  ustring art_b = (*iter_b)[PlaylistColumns.Artist];

                  return art_a.compare(art_b); 
              }

              void
              prepare_uris (Util::FileList const& uris, bool init_iter)
              {
                  bool begin = true;
                  TreeIter iter = ListStore->append();
                  if(init_iter)
                      m_PlayInitIter = iter;
                  append_uris (uris, iter, begin);
                  m_Playlist.check_caps();
                  m_Playlist.send_caps ();
              }

        };

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

        PlaylistTreeView           * m_TreeViewPlaylist;
        PAccess<MPX::Library>        m_Lib;
        PAccess<MPX::HAL>            m_HAL;

        PlaylistPrivate (MPX::Player & player, MPX::Source::PlaybackSourcePlaylist & mlib)
        {
            player.get_object(m_Lib);
            player.get_object(m_HAL);
            m_TreeViewPlaylist = new PlaylistTreeView(m_Lib, m_HAL, mlib);
            m_UI = manage(new Gtk::ScrolledWindow); 
            m_UI->add(*m_TreeViewPlaylist);
            m_UI->set_shadow_type(SHADOW_IN);
            m_UI->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
            m_UI->show_all();
        }
    };
}

namespace MPX
{
namespace Source
{
    bool PlaybackSourcePlaylist::m_signals_installed = false;

    PlaybackSourcePlaylist::PlaybackSourcePlaylist(
            Glib::RefPtr<Gtk::UIManager>    const& ui_manager,
            MPX::Player                          & player,
            std::string                     const& name
    )
    : PlaybackSource(ui_manager, name, C_CAN_SEEK)
    {
        if(!m_signals_installed)
        {
            signals[PSM_SIGNAL_PLAYLIST_TOOLTIP] =
              g_signal_new ("playlist-tooltip",
                            G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                            GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                            0,
                            NULL, NULL,
                            psm_glib_marshal_OBJECT__OBJECT_BOXED, G_TYPE_OBJECT, 2, G_TYPE_OBJECT, GTK_TYPE_TREE_ITER);

            signals[PSM_SIGNAL_PLAYLIST_END] = 
              g_signal_new ("playlist-end",
                            G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (G_OBJECT_GET_CLASS(G_OBJECT(gobj())))),
                            GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
                            0,
                            NULL, NULL,
                            g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

            m_signals_installed = true;
        }
        else
        {
            signals[PSM_SIGNAL_PLAYLIST_TOOLTIP] = g_signal_lookup("playlist-tooltip", Glib::Object::get_type() );
            signals[PSM_SIGNAL_PLAYLIST_END] = g_signal_lookup("playlist-end", Glib::Object::get_type() );
        }

        player.get_object(m_Lib);
        player.get_object(m_HAL);

        m_Private = new PlaylistPrivate(player,*this);
        m_Private->m_TreeViewPlaylist->signal_row_activated().connect( sigc::mem_fun( *this, &PlaybackSourcePlaylist::on_plist_row_activated ) );
        m_Private->m_TreeViewPlaylist->signal_query_tooltip().connect( sigc::mem_fun( *this, &PlaybackSourcePlaylist::on_plist_query_tooltip ) );
    }

    PlaybackSourcePlaylist::~PlaybackSourcePlaylist ()
    {
        delete m_Private;
    }

    PyObject*
    PlaybackSourcePlaylist::get_py_obj ()
    {
        using namespace boost::python;
        object obj(boost::ref(this));
        Py_INCREF(obj.ptr());
        return obj.ptr();
    }

    std::string
    PlaybackSourcePlaylist::get_guid ()
    {
        return "";
    }

    std::string
    PlaybackSourcePlaylist::get_class_guid ()
    {
        return "b20e4d5f-ebc5-4db7-be8a-cfacce153d64";
    }

    guint
    PlaybackSourcePlaylist::add_menu ()
    {
        return 0;
    }

    void
    PlaybackSourcePlaylist::on_plist_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
    {
        m_Private->m_TreeViewPlaylist->m_CurrentIter = m_Private->m_TreeViewPlaylist->ListStore->get_iter( path );
        m_Private->m_TreeViewPlaylist->m_CurrentId = (*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.RowId];
        m_Private->m_TreeViewPlaylist->queue_draw();
        Signals.PlayRequest.emit();
    }

    bool
    PlaybackSourcePlaylist::on_plist_query_tooltip(int x,int y, bool kbd, const Glib::RefPtr<Gtk::Tooltip> & tooltip)
    {
        GtkWidget * tooltip_widget = 0;
        int cell_x, cell_y;
        TreePath path;
        TreeViewColumn *col;
        int x2, y2;
        m_Private->m_TreeViewPlaylist->convert_widget_to_bin_window_coords(x, y, x2, y2);
        if(m_Private->m_TreeViewPlaylist->get_path_at_pos (x2, y2, path, col, cell_x, cell_y))
        {
            TreeIter iter = m_Private->m_TreeViewPlaylist->ListStore->get_iter(path);
            GtkTreeIter const* iter_c = iter->gobj();
            g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_TOOLTIP], 0, G_OBJECT(m_Private->m_TreeViewPlaylist->ListStore->gobj()), iter_c, &tooltip_widget);
            if( tooltip_widget )
            {
                g_object_ref(G_OBJECT(tooltip_widget));
                gtk_widget_show_all(GTK_WIDGET(tooltip_widget));
                tooltip->set_custom(*Glib::wrap(tooltip_widget, false)); 
                return true;
            }
        }

        return false;        
    }

#if 0
    PyObject*
    PlaybackSourcePlaylist::get_playlist_model ()
    {
        GtkListStore * store = GTK_LIST_STORE(m_Private->m_TreeViewPlaylist->ListStore->gobj());
        return pygobject_new((GObject*)store);
    }
#endif

    Glib::RefPtr<Gtk::ListStore>
    PlaybackSourcePlaylist::get_playlist_model ()
    {
        return m_Private->m_TreeViewPlaylist->ListStore;
    }

    PyObject*
    PlaybackSourcePlaylist::get_playlist_current_iter ()
    {
        if(!m_Private->m_TreeViewPlaylist->m_CurrentIter)
        {
            Py_INCREF(Py_None);
            return Py_None;
        }

        GtkTreeIter  * iter = m_Private->m_TreeViewPlaylist->m_CurrentIter.get().gobj();
        return pyg_boxed_new(GTK_TYPE_TREE_ITER, iter, TRUE, FALSE);
    }

    void
    PlaybackSourcePlaylist::play_album(gint64 id)
    {
        m_Private->m_TreeViewPlaylist->clear();
        m_Private->m_TreeViewPlaylist->append_album(id);
        Signals.PlayRequest.emit();
    }

    void
    PlaybackSourcePlaylist::append_album(gint64 id)
    {
        m_Private->m_TreeViewPlaylist->append_album(id);
    }

    void
    PlaybackSourcePlaylist::play_tracks(IdV const& idv)
    {
        m_Private->m_TreeViewPlaylist->clear();
        m_Private->m_TreeViewPlaylist->append_tracks(idv, NO_ORDER);
        Signals.PlayRequest.emit();
    }

    void
    PlaybackSourcePlaylist::append_tracks(IdV const& idv)
    {
        m_Private->m_TreeViewPlaylist->append_tracks(idv, NO_ORDER);
    }

    void
    PlaybackSourcePlaylist::action_cb_play ()
    {
        g_message(G_STRFUNC);
        m_Private->m_TreeViewPlaylist->m_CurrentIter = boost::optional<Gtk::TreeIter>(); 
        m_Private->m_TreeViewPlaylist->m_CurrentId = boost::optional<gint64>();
        Signals.PlayRequest.emit();
    }

    std::string
    PlaybackSourcePlaylist::get_uri () 
    {
        g_return_val_if_fail(m_Private->m_TreeViewPlaylist->m_CurrentIter, std::string());
        
#ifdef HAVE_HAL
        try{
            MPX::Track t ((*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.MPXTrack]);
            std::string volume_udi = get<std::string>(t[ATTRIBUTE_HAL_VOLUME_UDI].get());
            std::string device_udi = get<std::string>(t[ATTRIBUTE_HAL_DEVICE_UDI].get());
            std::string vrp = get<std::string>(t[ATTRIBUTE_VOLUME_RELATIVE_PATH].get());
            std::string mount_point = m_HAL.get().get_mount_point_for_volume(volume_udi, device_udi);
            std::string uri = build_filename(mount_point, vrp);
            return filename_to_uri(uri);
        } catch (HAL::NoMountPathForVolumeError & cxe)
        {
            g_message("%s: Error: What: %s", G_STRLOC, cxe.what());
        }

        return std::string();
#else
        return Glib::ustring((*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.Location]);
#endif // HAVE_HAL
    }

    std::string
    PlaybackSourcePlaylist::get_type ()
    {
        g_return_val_if_fail(m_Private->m_TreeViewPlaylist->m_CurrentIter, std::string());
        MPX::Track t ((*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.MPXTrack]);
        if(t[ATTRIBUTE_TYPE])
            return get<std::string>(t[ATTRIBUTE_TYPE].get());
        else
            return std::string();
    }

    GHashTable *
    PlaybackSourcePlaylist::get_metadata ()
    {
        return 0;
    }

    bool
    PlaybackSourcePlaylist::play ()
    {
        std::vector<Gtk::TreePath> rows = m_Private->m_TreeViewPlaylist->get_selection()->get_selected_rows();

        if(m_Private->m_TreeViewPlaylist->m_PlayInitIter)
        {
            m_Private->m_TreeViewPlaylist->m_CurrentIter = m_Private->m_TreeViewPlaylist->m_PlayInitIter.get();
            m_Private->m_TreeViewPlaylist->m_PlayInitIter.reset ();
        }
        // NO else so we fall through to the last check
        if(!rows.empty() && !m_Private->m_TreeViewPlaylist->m_CurrentIter)
        {
            m_Private->m_TreeViewPlaylist->m_CurrentIter = m_Private->m_TreeViewPlaylist->ListStore->get_iter (rows[0]); 
            m_Private->m_TreeViewPlaylist->m_CurrentId = (*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.RowId];
            m_Private->m_TreeViewPlaylist->queue_draw();
            return true;
        }
        else
        if(rows.empty() && !m_Private->m_TreeViewPlaylist->m_CurrentIter)
        {
            TreePath path;
            path.append_index(0);
            m_Private->m_TreeViewPlaylist->m_CurrentIter = m_Private->m_TreeViewPlaylist->ListStore->get_iter(path);
            return true;
        }
        else
        if(m_Private->m_TreeViewPlaylist->m_CurrentIter)
        {
            TreeIter &iter = m_Private->m_TreeViewPlaylist->m_CurrentIter.get();
            TreePath path1 = m_Private->m_TreeViewPlaylist->ListStore->get_path(iter);
            TreePath path2 = m_Private->m_TreeViewPlaylist->ListStore->get_path(--m_Private->m_TreeViewPlaylist->ListStore->children().end());

            if(path1[0] == path2[0])
            {
                g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_END], 0);
            }
            return true;
        }

        check_caps ();
    
        return false;
    }

    bool
    PlaybackSourcePlaylist::go_next ()
    {
        TreeIter &iter = m_Private->m_TreeViewPlaylist->m_CurrentIter.get();
        TreePath path1 = m_Private->m_TreeViewPlaylist->ListStore->get_path(iter);
        TreePath path2 = m_Private->m_TreeViewPlaylist->ListStore->get_path(--m_Private->m_TreeViewPlaylist->ListStore->children().end());

        if (path1[0] < path2[0]) 
        {
            iter++;
            if((path1[0]+1) == path2[0])
            {
                g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_END], 0);
            }
            return true;
        }
    
        return false;
    }

    bool
    PlaybackSourcePlaylist::go_prev ()
    {
        g_return_val_if_fail(bool(m_Private->m_TreeViewPlaylist->m_CurrentIter), false);

        TreeIter & iter = m_Private->m_TreeViewPlaylist->m_CurrentIter.get();

        TreePath path1 = m_Private->m_TreeViewPlaylist->ListStore->get_path(iter);
        TreePath path2 = m_Private->m_TreeViewPlaylist->ListStore->get_path(m_Private->m_TreeViewPlaylist->ListStore->children().begin());

        if (path1[0] > path2[0])
        {
            iter--;
            return true;
        }

        return false;
    }

    void
    PlaybackSourcePlaylist::stop () 
    {
        g_message(G_STRFUNC);
        m_Private->m_TreeViewPlaylist->m_CurrentIter = boost::optional<Gtk::TreeIter>();
        m_Private->m_TreeViewPlaylist->m_CurrentId = boost::optional<gint64>(); 
        m_Private->m_TreeViewPlaylist->queue_draw();
        check_caps ();
    }

    void
    PlaybackSourcePlaylist::set_play ()
    {
        m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_PLAY);
        send_caps ();
    }

    void
    PlaybackSourcePlaylist::clear_play ()
    {
        m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_PLAY);
        send_caps ();
    }

    void
    PlaybackSourcePlaylist::check_caps ()
    {
        if (m_Private->m_TreeViewPlaylist->m_CurrentIter)
        {
            TreeIter & iter = m_Private->m_TreeViewPlaylist->m_CurrentIter.get();

            TreePath path1 = m_Private->m_TreeViewPlaylist->ListStore->get_path(iter);
            TreePath path2 = m_Private->m_TreeViewPlaylist->ListStore->get_path(m_Private->m_TreeViewPlaylist->ListStore->children().begin());
            TreePath path3 = m_Private->m_TreeViewPlaylist->ListStore->get_path(--m_Private->m_TreeViewPlaylist->ListStore->children().end());

            if(path1[0] > path2[0])
                m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_GO_PREV);
            else
                m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_GO_PREV);

            if(path1[0] < path3[0])
                m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_GO_NEXT);
            else
                m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_GO_NEXT);
        }
        else
        {
            m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_GO_NEXT);
            m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_GO_PREV);
        }

        gint count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(m_Private->m_TreeViewPlaylist->ListStore->gobj()), NULL);
        Signals.Items.emit(count);
    }

    void
    PlaybackSourcePlaylist::send_metadata ()
    {
        g_return_if_fail(bool(m_Private->m_TreeViewPlaylist->m_CurrentIter));

        MPX::Metadata m = (MPX::Track((*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.MPXTrack]));
        Signals.Metadata.emit( m );
    }

    void
    PlaybackSourcePlaylist::play_post () 
    {
        m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_PAUSE);
        m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_PROVIDE_METADATA);

        check_caps ();

        m_Private->m_TreeViewPlaylist->scroll_to_track();
    }

    void
    PlaybackSourcePlaylist::next_post () 
    { 
        m_Private->m_TreeViewPlaylist->queue_draw();
        check_caps ();
        m_Private->m_TreeViewPlaylist->scroll_to_track();
    }

    void
    PlaybackSourcePlaylist::prev_post ()
    {
        m_Private->m_TreeViewPlaylist->queue_draw();
        check_caps ();
        m_Private->m_TreeViewPlaylist->scroll_to_track();
    }

    void
    PlaybackSourcePlaylist::restore_context ()
    {}

    void
    PlaybackSourcePlaylist::skipped () 
    {}

    void
    PlaybackSourcePlaylist::segment () 
    {}

    void
    PlaybackSourcePlaylist::buffering_done () 
    {}

    Glib::RefPtr<Gdk::Pixbuf>
    PlaybackSourcePlaylist::get_icon ()
    {
        try{
            return IconTheme::get_default()->load_icon("source-playlist", 64, ICON_LOOKUP_NO_SVG);
        } catch(...) { return Glib::RefPtr<Gdk::Pixbuf>(0); }
    }

    Gtk::Widget*
    PlaybackSourcePlaylist::get_ui ()
    {
        return m_Private->m_UI;
    }

    UriSchemes 
    PlaybackSourcePlaylist::get_uri_schemes ()
    {
        return UriSchemes();
    }

    void    
    PlaybackSourcePlaylist::process_uri_list (Util::FileList const& uris, bool play)
    {
        m_Private->m_TreeViewPlaylist->prepare_uris (uris, play);
        if(play)
        {
            Signals.PlayRequest.emit();
        }
    }

} // end namespace Source
} // end namespace MPX 
