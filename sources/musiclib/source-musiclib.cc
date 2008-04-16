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

#include "widgets/cell-renderer-cairo-surface.hh"
#include "widgets/cell-renderer-vbox.hh"
#include "widgets/gossip-cell-renderer-expander.h"

#include "mpx/hal.hh"
#include "mpx/library.hh"
#include "mpx/types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgetloader.h"
#include "mpx/playlistparser-xspf.hh"

#include "source-musiclib.hh"
#include "plugin-manager.hh"

using namespace Gtk;
using namespace Glib;
using namespace Gnome::Glade;
using namespace MPX;
using boost::get;

namespace
{
  const char ACTION_CLEAR [] = "action-clear";
  const char ACTION_REMOVE_ITEMS [] = "action-remove-items";
  const char ACTION_PLAY [] = "action-play";
  const char ACTION_GO_TO_ALBUM [] = "action-go-to-album";
  
  const char ui_playlist_popup [] =
  "<ui>"
  ""
  "<menubar name='popup-playlist-list'>"
  ""
  "   <menu action='dummy' name='menu-playlist-list'>"
  "       <menuitem action='action-play' name='action-play'/>"
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

  char const * ui_mainmerge1 =
  "<ui>"
  ""
  "<menubar name='MenubarMain'>"
  "   <placeholder name='placeholder-source'>"
  "     <menu action='menu-source-musiclib'>"
  "         <menuitem action='musiclib-sort-by-name'/>"
  "         <menuitem action='musiclib-sort-by-date'/>"
  "         <menuitem action='musiclib-sort-by-rating'/>"
  "         <separator/>"
  ""; 

  char const * ui_mainmerge2 =
  "     </menu>"
  "   </placeholder>"
  "</menubar>"
  ""
  "</ui>"
  "";

  Track
  sql_to_track (SQL::Row & row)
  {
        Track track;

        if (row.count("album_artist"))
          track[ATTRIBUTE_ALBUM_ARTIST] = get<std::string>(row["album_artist"]);

        if (row.count("artist"))
          track[ATTRIBUTE_ARTIST] = get<std::string>(row["artist"]);

        if (row.count("album"))
          track[ATTRIBUTE_ALBUM] = get<std::string>(row["album"]);

        if (row.count("track"))
          track[ATTRIBUTE_TRACK] = gint64(get<gint64>(row["track"]));

        if (row.count("title"))
          track[ATTRIBUTE_TITLE] = get<std::string>(row["title"]);

        if (row.count("time"))
          track[ATTRIBUTE_TIME] = gint64(get<gint64>(row["time"]));

        if (row.count("mb_artist_id"))
          track[ATTRIBUTE_MB_ARTIST_ID] = get<std::string>(row["mb_artist_id"]);

        if (row.count("mb_album_id"))
          track[ATTRIBUTE_MB_ALBUM_ID] = get<std::string>(row["mb_album_id"]);

        if (row.count("mb_track_id"))
          track[ATTRIBUTE_MB_TRACK_ID] = get<std::string>(row["mb_track_id"]);

        if (row.count("mb_album_artist_id"))
          track[ATTRIBUTE_MB_ALBUM_ARTIST_ID] = get<std::string>(row["mb_album_artist_id"]);

        if (row.count("amazon_asin"))
          track[ATTRIBUTE_ASIN] = get<std::string>(row["amazon_asin"]);

        if (row.count("id"))
          track[ATTRIBUTE_MPX_TRACK_ID] = get<gint64>(row["id"]);

        if (row.count("album_j"))
          track[ATTRIBUTE_MPX_ALBUM_ID] = get<gint64>(row["album_j"]);

        if (row.count("hal_volume_udi"))
          track[ATTRIBUTE_HAL_VOLUME_UDI] = get<std::string>(row["hal_volume_udi"]);

        if (row.count("hal_device_udi"))
          track[ATTRIBUTE_HAL_DEVICE_UDI] = get<std::string>(row["hal_device_udi"]);

        if (row.count("hal_vrp"))
          track[ATTRIBUTE_VOLUME_RELATIVE_PATH] = get<std::string>(row["hal_vrp"]);

        return track;
    }

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

        struct PlaylistColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
            Gtk::TreeModelColumn<ustring> Artist;
            Gtk::TreeModelColumn<ustring> Album;
            Gtk::TreeModelColumn<guint64> Track;
            Gtk::TreeModelColumn<ustring> Title;
            Gtk::TreeModelColumn<guint64> Length;

            // These hidden columns are used for sorting
            // They don't contain sortnames, as one might 
            // think from their name, but instead the MB
            // IDs (if not available, then just the plain name)
            // They are used only for COMPARISON FOR EQUALITY.
            // Obviously, comparing them with compare() is
            // useless if they're MB IDs

            Gtk::TreeModelColumn<ustring> ArtistSort;
            Gtk::TreeModelColumn<ustring> AlbumSort;
            Gtk::TreeModelColumn<gint64> RowId;
            Gtk::TreeModelColumn<ustring> Location;
            Gtk::TreeModelColumn< ::MPX::Track> MPXTrack;
            Gtk::TreeModelColumn<gint64> Rating;
            Gtk::TreeModelColumn<bool> IsMPXTrack;

            PlaylistColumnsT ()
            {
                add (Artist);
                add (Album);
                add (Track);
                add (Title);
                add (Length);
                add (ArtistSort);
                add (AlbumSort);
                add (RowId);
                add (Location);
                add (MPXTrack);
                add (Rating);
                add (IsMPXTrack);
            };
        };

        class PlaylistTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              PAccess<MPX::Library> m_Lib;
              MPX::Source::PlaybackSourceMusicLib & m_MusicLib;
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

              PlaylistTreeView (Glib::RefPtr<Gnome::Glade::Xml> const& xml, PAccess<MPX::Library> const& lib, MPX::Source::PlaybackSourceMusicLib & mlib)
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-playlist")
              , m_Lib(lib)
          , m_MusicLib(mlib)
              , m_RowId(0)
          , m_ButtonDepressed(0)
              {
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
                                    sigc::mem_fun (mlib, &MPX::Source::PlaybackSourceMusicLib::action_cb_play));

                m_ActionGroup->add  (Gtk::Action::create (ACTION_CLEAR,
                                    Gtk::StockID (GTK_STOCK_CLEAR),
                                    _("Clear Playlist")),
                                    sigc::mem_fun (*this, &PlaylistTreeView::clear));

                m_ActionGroup->add  (Gtk::Action::create (ACTION_REMOVE_ITEMS,
                                Gtk::StockID (GTK_STOCK_REMOVE),
                                _("Remove Selected Tracks")),
                                sigc::mem_fun (*this, &PlaylistTreeView::action_cb_playlist_remove_items));

                m_ActionGroup->add  (Gtk::Action::create (ACTION_GO_TO_ALBUM,
                                Gtk::StockID (GTK_STOCK_GO_FORWARD),
                                _("Go to selected Album")),
                                sigc::mem_fun (*this, &PlaylistTreeView::action_cb_go_selected_album));

                m_UIManager->insert_action_group (m_ActionGroup);
                m_UIManager->add_ui_from_string (ui_playlist_popup);

                Gtk::Widget * item = m_UIManager->get_widget("/ui/popup-playlist-list/menu-playlist-list/action-play");
                Gtk::Label * label = dynamic_cast<Gtk::Label*>(dynamic_cast<Gtk::Bin*>(item)->get_child());
                label->set_markup(_("<b>Play</b>"));

				set_tooltip_text(_("Drag and drop albums, tracks and files here to add them to the playlist."));
              }

              virtual void
              action_cb_go_selected_album ()
              {
                  PathV p = get_selection()->get_selected_rows();
                  TreeIter iter = ListStore->get_iter(p[0]);
                  Track t = (*iter)[PlaylistColumns.MPXTrack];
                  if(t[ATTRIBUTE_MPX_ALBUM_ID])
                  {
                    gint64 id = get<gint64>(t[ATTRIBUTE_MPX_ALBUM_ID].get());
                    m_MusicLib.action_cb_go_to_album(id);
                  }
                  else
                    g_message("%s: No Album ID", G_STRLOC);
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
                  m_MusicLib.check_caps();
                  m_MusicLib.send_caps ();
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

                  m_MusicLib.check_caps();
                  m_MusicLib.send_caps ();
              }

              void
              on_selection_changed ()
              {
                if(get_selection()->count_selected_rows() == 1)
                    m_MusicLib.set_play();
                else
                    m_MusicLib.clear_play();
              }

              void          
              append_trackid_v (TrackIdV const& tid_v, bool order = false)
              {
                if(tid_v.empty())
                    return;

                std::stringstream numbers;
                TrackIdV::const_iterator end = tid_v.end();
                end--;

                for(TrackIdV::const_iterator i = tid_v.begin(); i != tid_v.end(); ++i)
                {
                    numbers << *i;
                    if(i != end)
                        numbers << ",";
                }

                SQL::RowV v;
                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE id IN (%s) %s") % numbers.str() % (order ? "ORDER BY track_view.track" : "")).str()); 

                TreeIter iter = ListStore->append();
                m_PlayInitIter = iter;

                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                {
                        SQL::Row & r = *i;
                        
                        if(i != v.begin())
                            iter = ListStore->insert_after(iter);

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
                        else
                            g_critical("Track with no id; this should never happen.");

                        if(r.count("rating"))
                            (*iter)[PlaylistColumns.Rating] = get<gint64>(r["rating"]);

                        (*iter)[PlaylistColumns.Location] = get<std::string>(r["location"]); 
                        (*iter)[PlaylistColumns.MPXTrack] = sql_to_track(r); 
                        (*iter)[PlaylistColumns.IsMPXTrack] = true; 
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
                        else
                            g_critical("Track with no id; this should never happen.");

                        if(r.count("rating"))
                            (*iter)[PlaylistColumns.Rating] = get<gint64>(r["rating"]);

                        (*iter)[PlaylistColumns.Location] = get<std::string>(r["location"]); 
                        (*iter)[PlaylistColumns.MPXTrack] = sql_to_track(r); 
                        (*iter)[PlaylistColumns.IsMPXTrack] = true; 
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
                        (*iter)[PlaylistColumns.MPXTrack] = sql_to_track(r); 
                        (*iter)[PlaylistColumns.IsMPXTrack] = true; 
                    }
                }
                else
                {
                    bool begin = true;
                    Util::FileList uris = data.get_uris();
                    append_uris (uris, iter, begin);
                }

                m_MusicLib.check_caps ();
                m_MusicLib.send_caps ();
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
                        m_PathButtonPress = path;  
                  }
                  else
                  if( event->button == 3 )
                  {
                    m_ActionGroup->get_action (ACTION_CLEAR)->set_sensitive
                      (ListStore->children().size());

                    m_ActionGroup->get_action (ACTION_REMOVE_ITEMS)->set_sensitive
                      (get_selection()->count_selected_rows());

                    m_ActionGroup->get_action (ACTION_PLAY)->set_sensitive
                      (get_selection()->count_selected_rows());

                    m_ActionGroup->get_action (ACTION_GO_TO_ALBUM)->set_sensitive
                      (get_selection()->count_selected_rows() == 1);

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
              prepare_uris (Util::FileList const& uris)
              {
                bool begin = true;
                TreeIter iter = ListStore->append();
                m_PlayInitIter = iter;
                append_uris (uris, iter, begin);
                m_MusicLib.check_caps();
                m_MusicLib.send_caps ();
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
                        m_MLib.play_album(id, true);
                }
                else
                {
                        gint64 id = (*iter)[AlbumColumns.TrackId];
                        TrackIdV v;
                        v.push_back(id);
                        m_MLib.play_tracks(v, true);
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

                    (*iter)[AlbumColumns.HasTracks] = true;

                    if(has_children)
                    {
                        gtk_tree_store_remove(GTK_TREE_STORE(TreeStore->gobj()), &children);
                    } 
                    else
                        g_warning("%s:%d : No placeholder row present, state seems corrupted.", __FILE__, __LINE__);

                }

                scroll_to_row (path, 0.);
              }

              virtual void
              on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, SelectionData& selection_data, guint info, guint time)
              {
                if(m_DragAlbumId)
                    selection_data.set("mpx-album", 8, reinterpret_cast<const guint8*>(&m_DragAlbumId.get()), 8);
                else if(m_DragTrackId)
                    selection_data.set("mpx-track", 8, reinterpret_cast<const guint8*>(&m_DragTrackId.get()), 8);

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
              place_album (SQL::Row & r, gint64 id, bool acquire_cover = false)
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
                    m_Covers.get().cache( mbid, asin, acquire_cover ); 
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
              } 
   
              void
              album_list_load ()
              {
                TreeStore->clear ();
                m_MBIDIterMap.clear();
                m_AlbumIterMap.clear();

                SQL::RowV v;
                m_Lib.get().getSQL(v, "SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id;");

                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                {
                    SQL::Row & r = *i; 
                    place_album(r, get<gint64>(r["id"]), false);
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
              on_new_track(Track & track, gint64 album_id)
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


              AlbumTreeView (Glib::RefPtr<Gnome::Glade::Xml> const& xml,    
                         PAccess<MPX::Library> const& lib, PAccess<MPX::Covers> const& amzn, MPX::Source::PlaybackSourceMusicLib & mlib)
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-albums")
              , m_Lib(lib)
              , m_Covers(amzn)
              , m_MLib(mlib)
              , m_ButtonPressed(false)
              {
                for(int n = 0; n < 6; ++n)
                    m_Stars[n] = Gdk::Pixbuf::create_from_file(build_filename(build_filename(DATA_DIR,"images"),
                        (boost::format("stars%d.png") % n).str()));

                m_Lib.get().signal_new_album().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_album ));
                m_Lib.get().signal_new_track().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_track ));
                m_Lib.get().signal_vacuumized().connect( sigc::mem_fun( *this, &AlbumTreeView::album_list_load ));
                m_Covers.get().signal_got_cover().connect( sigc::mem_fun( *this, &AlbumTreeView::on_got_cover ));

                xml->get_widget("album-filter-entry", m_FilterEntry);

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
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &AlbumTreeView::cellDataFuncText4 ));

                append_column(*col);

                TreeStore = Gtk::TreeStore::create(AlbumColumns);
                TreeStoreFilter = Gtk::TreeModelFilter::create(TreeStore);
                TreeStoreFilter->set_visible_func (sigc::mem_fun (*this, &AlbumTreeView::albumVisibleFunc));
                m_FilterEntry->signal_changed().connect (sigc::mem_fun (TreeStoreFilter.operator->(), &TreeModelFilter::refilter));

                set_model(TreeStoreFilter);
                TreeStore->set_sort_func(0 , sigc::mem_fun( *this, &AlbumTreeView::slotSortAlpha ));
                TreeStore->set_sort_func(1 , sigc::mem_fun( *this, &AlbumTreeView::slotSortDate ));
                TreeStore->set_sort_func(2 , sigc::mem_fun( *this, &AlbumTreeView::slotSortRating ));
                TreeStore->set_sort_column(0, Gtk::SORT_ASCENDING);

                m_DiscDefault_Pixbuf = Gdk::Pixbuf::create_from_file(build_filename(DATA_DIR, build_filename("images","disc-default.png")));
                m_DiscDefault = Util::cairo_image_surface_from_pixbuf(m_DiscDefault_Pixbuf->scale_simple(72,72,Gdk::INTERP_BILINEAR));

                album_list_load ();

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-album", TARGET_SAME_APP, 0x80));
                Entries.push_back(TargetEntry("mpx-track", TARGET_SAME_APP, 0x81));
                drag_source_set(Entries); 
              }
        };

        PlaylistTreeView *m_TreeViewPlaylist;
        AlbumTreeView *m_TreeViewAlbums;
        PAccess<MPX::Library> m_Lib;
        PAccess<MPX::Covers> m_Covers;

        MusicLibPrivate (MPX::Player & player, MPX::Source::PlaybackSourceMusicLib & mlib)
        {
            const std::string path (build_filename(DATA_DIR, build_filename("glade","source-musiclib.glade")));
            m_RefXml = Gnome::Glade::Xml::create (path);
            m_UI = m_RefXml->get_widget("source-musiclib");
            player.get_object(m_Lib);
            player.get_object(m_Covers);
            m_TreeViewPlaylist = new PlaylistTreeView(m_RefXml, m_Lib, mlib);
            m_TreeViewAlbums = new AlbumTreeView(m_RefXml, m_Lib, m_Covers, mlib);
        }
    };
}

namespace MPX
{
namespace Source
{
    PlaybackSourceMusicLib::PlaybackSourceMusicLib (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player & player)
    : PlaybackSource(ui_manager, _("Music"), C_CAN_SEEK)
    , m_MainUIManager(ui_manager)
    {
        player.get_object(m_Lib);
        player.get_object(m_HAL);

        m_Private = new MusicLibPrivate(player,*this);
        m_Private->m_TreeViewPlaylist->signal_row_activated().connect( sigc::mem_fun( *this,
            &PlaybackSourceMusicLib::on_plist_row_activated ) );

        m_MainActionGroup = ActionGroup::create("ActionsMusicLib");
        m_MainActionGroup->add(Action::create("menu-source-musiclib", _("Music _Library")));

        m_PluginManager = new PlaylistPluginManager();
        std::string const user_path = build_filename(build_filename(g_get_user_data_dir(), "mpx"),"playlist-scripts");
        if(file_test(user_path, FILE_TEST_EXISTS))
            m_PluginManager->append_search_path (user_path);
        m_PluginManager->append_search_path (build_filename(DATA_DIR,"playlist-scripts"));
        m_PluginManager->load_plugins();

        Glib::RefPtr<Gtk::IconFactory> factory = Gtk::IconFactory::create();

        Gtk::RadioButtonGroup gr1;
        m_MainActionGroup->add (RadioAction::create( gr1, "musiclib-sort-by-name", "Sort Albums By Artist/Album"),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_sort_column_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-name"))->property_value() = 0;
        m_MainActionGroup->add (RadioAction::create( gr1, "musiclib-sort-by-date", "Sort Albums By Date"),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_sort_column_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-date"))->property_value() = 1;
        m_MainActionGroup->add (RadioAction::create( gr1, "musiclib-sort-by-rating", "Sort Albums By Rating"),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_sort_column_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-rating"))->property_value() = 2;

        m_MergedUI = ui_mainmerge1;
        PlaylistPluginHoldMap const& map = m_PluginManager->get_map();
        for(PlaylistPluginHoldMap::const_iterator i = map.begin(); i != map.end(); ++i)
        {
            PlaylistPluginHolderRefP const& p = i->second;
            static boost::format menuitem_f ("<menuitem action='plugin-%lld'/>");
            static boost::format name_f ("plugin-%lld");
            static boost::format stock_f ("stock-mpx-playlist-plugin-%lld");

            ustring name = (name_f % p->get_id()).str();

            if(p->get_icon())
            {
                factory->add(StockID ((stock_f % p->get_id()).str()), Gtk::IconSet(p->get_icon())); 
                m_MainActionGroup->add (Action::create( name, StockID((stock_f % p->get_id()).str()), p->get_name()),
                                        sigc::bind( sigc::mem_fun (*this, &PlaybackSourceMusicLib::action_cb_run_plugin),
                                        p->get_id()));
            }
            else
            {
                m_MainActionGroup->add (Action::create( name, p->get_name()),
                                        sigc::bind( sigc::mem_fun (*this, &PlaybackSourceMusicLib::action_cb_run_plugin),
                                        p->get_id()));
            }
            m_MergedUI += (menuitem_f % p->get_id()).str();
        }
        m_MergedUI += ui_mainmerge2;
        factory->add_default();

        m_MainUIManager->insert_action_group(m_MainActionGroup);
    }

    PlaybackSourceMusicLib::~PlaybackSourceMusicLib ()
    {
        delete m_Private;
        delete m_PluginManager;
    }

    guint
    PlaybackSourceMusicLib::add_menu ()
    {
        return m_MainUIManager->add_ui_from_string(m_MergedUI);
    }

    void
    PlaybackSourceMusicLib::on_sort_column_change ()
    {
        int value = RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action ("musiclib-sort-by-name"))->get_current_value();
        m_Private->m_TreeViewAlbums->TreeStore->set_sort_column(value, Gtk::SORT_ASCENDING);    
    }

    void
    PlaybackSourceMusicLib::play_album(gint64 id, bool play)
    {
        SQL::RowV v;
        m_Lib.get().getSQL(v, (boost::format("SELECT id, track FROM track_view WHERE album_j = '%lld'") % id).str()); 
        TrackIdV idv;
        for(SQL::RowV::const_iterator i = v.begin(); i != v.end(); ++i)
        {
            SQL::Row r = *i;
            idv.push_back(get<gint64>(r["id"]));
        }
        m_Private->m_TreeViewPlaylist->clear();
        m_Private->m_TreeViewPlaylist->append_trackid_v(idv, true);
        if(play)
          Signals.PlayRequest.emit();
    }

    void
    PlaybackSourceMusicLib::play_tracks(TrackIdV const& idv , bool play)
    {
        m_Private->m_TreeViewPlaylist->clear();
        m_Private->m_TreeViewPlaylist->append_trackid_v(idv, false);
        if(play)
          Signals.PlayRequest.emit();
    }

    void
    PlaybackSourceMusicLib::action_cb_play ()
    {
      m_Private->m_TreeViewPlaylist->m_CurrentIter = boost::optional<Gtk::TreeIter>(); 
      m_Private->m_TreeViewPlaylist->m_CurrentId = boost::optional<gint64>();
      Signals.PlayRequest.emit();
    }

    void
    PlaybackSourceMusicLib::action_cb_run_plugin (gint64 id)
    {
        TrackIdV v;
        m_PluginManager->run( id, m_Lib.get(), this );
    }

    void
    PlaybackSourceMusicLib::action_cb_go_to_album(gint64 id)
    {
        m_Private->m_TreeViewAlbums->go_to_album(id);
    }

    void
    PlaybackSourceMusicLib::on_plist_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
    {
      m_Private->m_TreeViewPlaylist->m_CurrentIter = m_Private->m_TreeViewPlaylist->ListStore->get_iter( path );
      m_Private->m_TreeViewPlaylist->m_CurrentId = (*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.RowId];
      m_Private->m_TreeViewPlaylist->queue_draw();
      Signals.PlayRequest.emit();
    }

    std::string
    PlaybackSourceMusicLib::get_uri () 
    {
#ifdef HAVE_HAL
        try{
            MPX::Track t ((*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.MPXTrack]);
            std::string volume_udi = get<std::string>(t[ATTRIBUTE_HAL_VOLUME_UDI].get());
            std::string device_udi = get<std::string>(t[ATTRIBUTE_HAL_DEVICE_UDI].get());
            std::string vrp = get<std::string>(t[ATTRIBUTE_VOLUME_RELATIVE_PATH].get());
            std::string mount_point = m_HAL.get().get_mount_point_for_volume(volume_udi, device_udi);
            return build_filename(mount_point, vrp);
        } catch (HAL::Exception)
        {
            return std::string();
        }
#else
        return Glib::ustring((*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.Location]);
#endif // HAVE_HAL
    }

    std::string
    PlaybackSourceMusicLib::get_type ()
    {
        return Glib::ustring();
    }

    GHashTable *
    PlaybackSourceMusicLib::get_metadata ()
    {
        return 0;
    }

    bool
    PlaybackSourceMusicLib::play ()
    {
        std::vector<Gtk::TreePath> rows = m_Private->m_TreeViewPlaylist->get_selection()->get_selected_rows();

        if(m_Private->m_TreeViewPlaylist->m_PlayInitIter)
        {
            m_Private->m_TreeViewPlaylist->m_CurrentIter = m_Private->m_TreeViewPlaylist->m_PlayInitIter.get();
            m_Private->m_TreeViewPlaylist->m_PlayInitIter.reset ();
            return true;
        }

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
            return true;

        check_caps ();
    
        return false;
    }

    bool
    PlaybackSourceMusicLib::go_next ()
    {
        g_return_val_if_fail(bool(m_Private->m_TreeViewPlaylist->m_CurrentIter), false);

        TreeIter & iter = m_Private->m_TreeViewPlaylist->m_CurrentIter.get();

        TreePath path1 = m_Private->m_TreeViewPlaylist->ListStore->get_path(iter);
        TreePath path3 = m_Private->m_TreeViewPlaylist->ListStore->get_path(--m_Private->m_TreeViewPlaylist->ListStore->children().end());

        if (path1[0] < path3[0]) 
        {
            iter++;
            return true;
        }
    
        return false;
    }

    bool
    PlaybackSourceMusicLib::go_prev ()
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
    PlaybackSourceMusicLib::stop () 
    {
        m_Private->m_TreeViewPlaylist->m_CurrentIter = boost::optional<Gtk::TreeIter>();
        m_Private->m_TreeViewPlaylist->m_CurrentId = boost::optional<gint64>(); 
        m_Private->m_TreeViewPlaylist->queue_draw();
        check_caps ();
    }

    void
    PlaybackSourceMusicLib::set_play ()
    {
        m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_PLAY);
        send_caps ();
    }

    void
    PlaybackSourceMusicLib::clear_play ()
    {
        m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_PLAY);
        send_caps ();
    }

    void
    PlaybackSourceMusicLib::check_caps ()
    {
        if (!m_Private->m_TreeViewPlaylist->m_CurrentIter)
        {
            m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_GO_NEXT);
            m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_GO_PREV);
            m_Caps = Caps (m_Caps &~ PlaybackSource::C_CAN_PLAY);
            return;
        }

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

    void
    PlaybackSourceMusicLib::send_metadata ()
    {
        g_return_if_fail(bool(m_Private->m_TreeViewPlaylist->m_CurrentIter));

        MPX::Metadata m = (MPX::Track((*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.MPXTrack]));
        Signals.Metadata.emit( m );
    }

    void
    PlaybackSourceMusicLib::play_post () 
    {
        m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_PAUSE);
        m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_PROVIDE_METADATA);

        check_caps ();

        m_Private->m_TreeViewPlaylist->scroll_to_track();
    }

    void
    PlaybackSourceMusicLib::next_post () 
    { 
        m_Private->m_TreeViewPlaylist->queue_draw();
        check_caps ();
        m_Private->m_TreeViewPlaylist->scroll_to_track();
    }

    void
    PlaybackSourceMusicLib::prev_post ()
    {
        m_Private->m_TreeViewPlaylist->queue_draw();
        check_caps ();
        m_Private->m_TreeViewPlaylist->scroll_to_track();
    }

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
            return IconTheme::get_default()->load_icon("source-library", 64, ICON_LOOKUP_NO_SVG);
        } catch(...) { return Glib::RefPtr<Gdk::Pixbuf>(0); }
    }

    Gtk::Widget*
    PlaybackSourceMusicLib::get_ui ()
    {
        return m_Private->m_UI;
    }

    UriSchemes 
    PlaybackSourceMusicLib::Get_Schemes ()
    {
        UriSchemes s;
        s.push_back("file");
        return s;
    }

    void    
    PlaybackSourceMusicLib::Process_URI_List (Util::FileList const& uris)
    {
        m_Private->m_TreeViewPlaylist->prepare_uris (uris);
        Signals.PlayRequest.emit();
    }

} // end namespace Source
} // end namespace MPX 
