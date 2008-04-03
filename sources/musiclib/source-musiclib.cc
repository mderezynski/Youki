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

#include "widgets/cell-renderer-cairo-surface.hh"
#include "widgets/cell-renderer-vbox.hh"
#include "widgets/gossip-cell-renderer-expander.h"

#include "mpx/library.hh"
#include "mpx/types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgetloader.h"

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
  const char ACTION_PLAY [] = "action-play";
  
  const char ui_playlist_popup [] =
  "<ui>"
  ""
  "<menubar name='popup-playlist-list'>"
  ""
  "   <menu action='dummy' name='menu-playlist-list'>"
  "       <menuitem action='action-play' name='action-play'/>"
  "		<separator/>"
  "       <menuitem action='action-remove-items'/>"
  "       <menuitem action='action-clear'/>"
  "   </menu>"
  ""
  "</menubar>"
  ""
  "</ui>";


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

		return track;
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
            };
        };

        class PlaylistTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              PAccess<MPX::Library> m_Lib;
			  MPX::Source::PlaybackSourceMusicLib & m_MusicLib;
              gint64 m_RowId;
			  Glib::RefPtr<Gdk::Pixbuf> m_Playing;

		      TreePath  m_PathButtonPress;
			  bool      m_ButtonDepressed;

			  Glib::RefPtr<Gtk::UIManager> m_UIManager;
			  Glib::RefPtr<Gtk::ActionGroup> m_ActionGroup;

            public:

			  boost::optional<Gtk::TreeIter> m_CurrentIter;
			  boost::optional<gint64> m_CurrentId;
              PlaylistColumnsT PlaylistColumns;
              Glib::RefPtr<Gtk::ListStore> ListStore;

              enum Column
              {
                C_PLAYING,
                C_TITLE,
                C_ARTIST,
                C_LENGTH,
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
                                            sigc::mem_fun (mlib, &MPX::Source::PlaybackSourceMusicLib::on_plist_play));

		        m_ActionGroup->add  (Gtk::Action::create (ACTION_CLEAR,
                                            Gtk::StockID (GTK_STOCK_CLEAR),
                                            _("Clear Playlist")),
                                            sigc::mem_fun (*this, &PlaylistTreeView::action_cb_playlist_clear));

			    m_ActionGroup->add  (Gtk::Action::create (ACTION_REMOVE_ITEMS,
                                            Gtk::StockID (GTK_STOCK_REMOVE),
                                            _("Remove Selected Tracks")),
                                            sigc::mem_fun (*this, &PlaylistTreeView::action_cb_playlist_remove_items));

				m_UIManager->insert_action_group (m_ActionGroup);
				m_UIManager->add_ui_from_string (ui_playlist_popup);

				Gtk::Widget * item = m_UIManager->get_widget("/ui/popup-playlist-list/menu-playlist-list/action-play");
				Gtk::Label * label = dynamic_cast<Gtk::Label*>(dynamic_cast<Gtk::Bin*>(item)->get_child());
				label->set_markup(_("<b>Play</b>"));
              }

			  virtual void
		      action_cb_playlist_clear ()	
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
							g_warning("Warning, no location given for DnD track; this is totally wrong and should never happen.");

						(*iter)[PlaylistColumns.MPXTrack] = track; 
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

                        (*iter)[PlaylistColumns.Location] = get<std::string>(r["location"]); 
					    (*iter)[PlaylistColumns.MPXTrack] = sql_to_track(r); 
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

                        (*iter)[PlaylistColumns.Location] = get<std::string>(r["location"]); 
					    (*iter)[PlaylistColumns.MPXTrack] = sql_to_track(r); 
                    }
                }
                else
                {
					bool begin = true;
					Util::FileList uris = data.get_uris();
					append_uris (uris, iter, begin);

					m_MusicLib.check_caps();
					m_MusicLib.send_caps ();
				}
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

					//convert_bin_window_to_tree_coords (int (event->x), int (event->y), tree_x, tree_y);
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

				  //convert_bin_window_to_tree_coords (int (event->x), int (event->y), tree_x, tree_y);
				  tree_x = event->x; tree_y = event->y;
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
        };

        enum AlbumRowType
        {
            ROW_ALBUM,
            ROW_TRACK,
			ROW_ARTIST,
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
            Gtk::TreeModelColumn<gint64> Rating;
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
              typedef std::map<std::string, IterSet> ASINIterMap;
              typedef std::map<gint64, TreeIter> IdIterMap; 

              AlbumColumnsT AlbumColumns;
              Glib::RefPtr<Gtk::TreeStore> TreeStore;
			  Glib::RefPtr<Gtk::TreeModelFilter> TreeStoreFilter;

              PAccess<MPX::Library> m_Lib;
              PAccess<MPX::Amazon::Covers> m_AMZN;

              ASINIterMap m_ASINIterMap;
              IdIterMap m_AlbumIterMap;
			  IdIterMap m_ArtistIterMap;

              Cairo::RefPtr<Cairo::ImageSurface> m_DiscDefault;
              Glib::RefPtr<Gdk::Pixbuf> m_DiscDefault_Pixbuf;
              Glib::RefPtr<Gdk::Pixbuf> m_Stars[6];

              std::string m_DragASIN;
              gint64 m_AlbumDragId;
              gint64 m_TrackDragId;
              TreePath m_PathButtonPress;

			  Gtk::Entry *m_FilterEntry;

              virtual void
              on_row_expanded (const TreeIter &iter_filter,
                               const TreePath &path) 
              {
				if(path.size() == 1)
				{
					scroll_to_row (path, 0.);
					return;
				}

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
                if(m_AlbumDragId)
                    selection_data.set("mpx-album", 8, reinterpret_cast<const guint8*>(&m_AlbumDragId), 8);
                else if(m_TrackDragId)
                    selection_data.set("mpx-track", 8, reinterpret_cast<const guint8*>(&m_TrackDragId), 8);
              }

              virtual void
              on_drag_begin (const Glib::RefPtr<Gdk::DragContext>& context) 
              {
                Glib::RefPtr<Gdk::Pixbuf> Cover;
                if(m_AlbumDragId)
                {
                    if(!m_DragASIN.empty())
                    {
                        m_AMZN.get().fetch(m_DragASIN, Cover);
						if(Cover)
						{
							drag_source_set_icon(Cover->scale_simple(128,128,Gdk::INTERP_BILINEAR));
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
              on_button_press_event (GdkEventButton* event)
              {
                int cell_x, cell_y ;
                TreeViewColumn *col ;

                if(get_path_at_pos (event->x, event->y, m_PathButtonPress, col, cell_x, cell_y))
                {
                    TreeIter iter = TreeStore->get_iter(TreeStoreFilter->convert_path_to_child_path(m_PathButtonPress));
                    if(m_PathButtonPress.get_depth() == 2)
                    {
                        m_DragASIN = (*iter)[AlbumColumns.ASIN];
                        m_AlbumDragId = (*iter)[AlbumColumns.Id];
                        m_TrackDragId = 0;

						if( (cell_x >= 134) && (cell_x <= 210) && (cell_y >= 65) && (cell_y <=78))
						{
							int rating = ((cell_x - 134)+7) / 15;
							(*iter)[AlbumColumns.Rating] = rating;	
							m_Lib.get().rateAlbum(m_AlbumDragId, rating);
						}
                    }
                    else
                    if(m_PathButtonPress.get_depth() == 3)
                    {
                        m_DragASIN = ""; // TODO: Use boost::optional
                        m_AlbumDragId = 0; 
                        m_TrackDragId = (*iter)[AlbumColumns.TrackId];
                    }
                }
                TreeView::on_button_press_event(event);
                return false;
              }

              void
              on_got_cover(const Glib::ustring& asin)
              {
                Cairo::RefPtr<Cairo::ImageSurface> Cover;
                m_AMZN.get().fetch(asin, Cover, COVER_SIZE_ALBUM_LIST);
                Util::cairo_image_surface_border(Cover, 1.);
                IterSet & set = m_ASINIterMap[asin];
                for(IterSet::iterator i = set.begin(); i != set.end(); ++i)
                {
                    (*(*i))[AlbumColumns.Image] = Cover;
                }
              }
    
              void
              on_new_album(const std::string& mbid, const std::string& asin, gint64 id, gint64 album_artist_id)
              {
				TreeIter iter_artist; 
				IdIterMap::const_iterator i = m_ArtistIterMap.find(album_artist_id);
				if(i !=	m_ArtistIterMap.end())
				{
					iter_artist = i->second;
				}
				else
				{
					SQL::RowV v;
					m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album_artist WHERE id = %lld;") % album_artist_id).str());
					SQL::Row & r = v[0];

					iter_artist = TreeStore->append();
					if(r.count("album_artist_sortname"))
						(*iter_artist)[AlbumColumns.Text] = get<std::string>(r["album_artist_sortname"]);
					else
						(*iter_artist)[AlbumColumns.Text] = get<std::string>(r["album_artist"]);
					(*iter_artist)[AlbumColumns.RowType] = ROW_ARTIST;
					m_ArtistIterMap[album_artist_id] = iter_artist;
				}

                TreeIter iter = TreeStore->append(iter_artist->children());
                m_AlbumIterMap[id] = iter;

                TreeStore->append(iter->children()); //create dummy/placeholder row for tracks

                (*iter)[AlbumColumns.RowType] = ROW_ALBUM; 
                (*iter)[AlbumColumns.HasTracks] = false; 
                (*iter)[AlbumColumns.Image] = m_DiscDefault; 
                (*iter)[AlbumColumns.ASIN] = asin; 
                (*iter)[AlbumColumns.Id] = id; 

                if(!asin.empty())
                {
                  IterSet & s = m_ASINIterMap[asin];
                  s.insert(iter);
                  m_AMZN.get().cache(asin);
                }

                SQL::RowV v;
                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album.id = %lld;") % id).str());

                g_return_if_fail(!v.empty());

                SQL::Row & r = v[0];

                if(r.count("album_rating"))
				{
					gint64 rating = get<gint64>(r["album_rating"]);
					(*iter)[AlbumColumns.Rating] = rating;
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
                if(r.find("album_artist_sortname") != r.end())
                    ArtistSort = get<std::string>(r["album_artist_sortname"]);
                else
                    ArtistSort = get<std::string>(r["album_artist"]);

#if 0
				if(r.count("album_genre"))
				{
					(*iter)[AlbumColumns.Text] = (boost::format("<span size='12000'>%2%</span>\n<span size='12000'><b>%1%</b></span>\n<span size='9000'>%4%, %3%</span>") % Markup::escape_text(get<std::string>(r["album"])).c_str() % Markup::escape_text(ArtistSort).c_str() % date.substr(0,4) % Markup::escape_text(get<std::string>(r["album_genre"])).c_str() ).str();
				}
				else
#endif
				{
					(*iter)[AlbumColumns.Text] = (boost::format("<span size='12000'>%2%</span>\n<span size='12000'><b>%1%</b></span>\n<span size='9000'>%3%</span>") % Markup::escape_text(get<std::string>(r["album"])).c_str() % Markup::escape_text(ArtistSort).c_str() % date.substr(0,4)).str();
				}

                (*iter)[AlbumColumns.AlbumSort] = ustring(get<std::string>(r["album"])).collate_key();
                (*iter)[AlbumColumns.ArtistSort] = ustring(ArtistSort).collate_key();
              }

              void
              album_list_load ()
              {
				m_ArtistIterMap.clear();
				TreeStore->clear ();

                SQL::RowV v;
                m_Lib.get().getSQL(v, "SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id;");
                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                {
                    SQL::Row & r = *i; 

					gint64 album_artist_id = get<gint64>(r["album_artist_j"]);

					TreeIter iter_artist; 
					IdIterMap::const_iterator i = m_ArtistIterMap.find(album_artist_id);
					if(i !=	m_ArtistIterMap.end())
					{
						iter_artist = i->second;
					}
					else
					{
						iter_artist = TreeStore->append();
						if(r.count("album_artist_sortname"))
							(*iter_artist)[AlbumColumns.Text] = get<std::string>(r["album_artist_sortname"]);
						else
							(*iter_artist)[AlbumColumns.Text] = get<std::string>(r["album_artist"]);
						(*iter_artist)[AlbumColumns.RowType] = ROW_ARTIST;
						m_ArtistIterMap[album_artist_id] = iter_artist;
					}

                    TreeIter iter = TreeStore->append(iter_artist->children());
                    TreeStore->append(iter->children()); // create placeholder/dummy row for tracks

                    (*iter)[AlbumColumns.RowType] = ROW_ALBUM; 
                    (*iter)[AlbumColumns.HasTracks] = false; 
                    (*iter)[AlbumColumns.Image] = m_DiscDefault; 
                    (*iter)[AlbumColumns.Id] = get<gint64>(r["id"]); 
      
	                if(r.count("album_rating"))
					{
						gint64 rating = get<gint64>(r["album_rating"]);
						(*iter)[AlbumColumns.Rating] = rating;
					}

                    if(r.count("amazon_asin"))
                    {
                        std::string asin = get<std::string>(r["amazon_asin"]);
                        (*iter)[AlbumColumns.ASIN] = asin; 
                        IterSet & s = m_ASINIterMap[asin];
                        s.insert(iter);
                        m_AMZN.get().cache(asin);
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

#if 0
					if(r.count("album_genre"))
					{
						(*iter)[AlbumColumns.Text] = (boost::format("<span size='12000'>%2%</span>\n<span size='12000'><b>%1%</b></span>\n<span size='9000'>%4%, %3%</span>") % Markup::escape_text(get<std::string>(r["album"])).c_str() % Markup::escape_text(ArtistSort).c_str() % date.substr(0,4) % Markup::escape_text(get<std::string>(r["album_genre"])).c_str() ).str();
					}
					else
#endif
					{
						(*iter)[AlbumColumns.Text] = (boost::format("<span size='12000'>%2%</span>\n<span size='12000'><b>%1%</b></span>\n<span size='9000'>%3%</span>") % Markup::escape_text(get<std::string>(r["album"])).c_str() % Markup::escape_text(ArtistSort).c_str() % date.substr(0,4)).str();
					}
                    (*iter)[AlbumColumns.AlbumSort] = ustring(get<std::string>(r["album"])).collate_key();
                    (*iter)[AlbumColumns.ArtistSort] = ustring(ArtistSort).collate_key();
                }
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

                        (*child)[AlbumColumns.TrackId] = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get());
                        (*child)[AlbumColumns.RowType] = ROW_TRACK; 
                    }
                }
              }

              int
              slotSort(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[AlbumColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[AlbumColumns.RowType];

				if((rt_a == ROW_ARTIST) && (rt_b == ROW_ARTIST))
				{
                  Glib::ustring text_a = (*iter_a)[AlbumColumns.Text];
                  Glib::ustring text_b = (*iter_b)[AlbumColumns.Text];
				  return text_a.compare(text_b);
				}

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

              void
              cellDataFuncCover (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererCairoSurface *cell = dynamic_cast<CellRendererCairoSurface*>(basecell);
                if(path.get_depth() == 2)
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
                if(path.get_depth() == 2)
                {
                    cvbox->property_visible() = true; 

					if(cell1)
	                    cell1->property_markup() = (*iter)[AlbumColumns.Text]; 
					if(cell2)
	                    cell2->property_pixbuf() = m_Stars[gint64((*iter)[AlbumColumns.Rating])];
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
                if(path.get_depth() == 3)
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
                if(path.get_depth() == 3)
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = Markup::escape_text((*iter)[AlbumColumns.TrackTitle]);
                }
				else
                if(path.get_depth() == 1)
				{
                    cell->property_visible() = true; 
                    cell->property_markup() = Markup::escape_text((*iter)[AlbumColumns.Text]);
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
                if(path.get_depth() == 3)
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

              AlbumTreeView (Glib::RefPtr<Gnome::Glade::Xml> const& xml,	
								PAccess<MPX::Library> const& lib, PAccess<MPX::Amazon::Covers> const& amzn)
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-albums")
              , m_Lib(lib)
              , m_AMZN(amzn)
              {

				for(int n = 0; n < 6; ++n)
				{
					m_Stars[n] = Gdk::Pixbuf::create_from_file(build_filename(build_filename(DATA_DIR,"images"), (boost::format("stars%d.png") % n).str()));
				}

                m_Lib.get().signal_new_album().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_album ));
                m_Lib.get().signal_new_track().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_track ));
                m_Lib.get().signal_vacuumized().connect( sigc::mem_fun( *this, &AlbumTreeView::album_list_load ));
                m_AMZN.get().signal_got_cover().connect( sigc::mem_fun( *this, &AlbumTreeView::on_got_cover ));

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
                celltext->property_ypad() = 2;
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
                TreeStore->set_default_sort_func(sigc::mem_fun( *this, &AlbumTreeView::slotSort ));
                TreeStore->set_sort_column(GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

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
		PAccess<MPX::Amazon::Covers> m_AMZN;

        MusicLibPrivate (MPX::Player & player, MPX::Source::PlaybackSourceMusicLib & mlib)
        {
            const std::string path (build_filename(DATA_DIR, build_filename("glade","source-musiclib.glade")));
            m_RefXml = Gnome::Glade::Xml::create (path);
            m_UI = m_RefXml->get_widget("source-musiclib");
			player.get_object(m_Lib);
			player.get_object(m_AMZN);
            m_TreeViewPlaylist = new PlaylistTreeView(m_RefXml, m_Lib, mlib);
            m_TreeViewAlbums = new AlbumTreeView(m_RefXml, m_Lib, m_AMZN);
        }
    };
}

namespace MPX
{
namespace Source
{
    PlaybackSourceMusicLib::PlaybackSourceMusicLib (MPX::Player & player)
    : PlaybackSource(_("Music"))
    {
      m_Private = new MusicLibPrivate(player,*this);
	  m_Private->m_TreeViewPlaylist->signal_row_activated().connect( sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_plist_row_activated ) );

	  m_caps = Caps (m_caps | PlaybackSource::C_CAN_SEEK);
	  send_caps ();
    }

	void
	PlaybackSourceMusicLib::on_plist_play ()
	{
      m_Private->m_TreeViewPlaylist->m_CurrentIter = boost::optional<Gtk::TreeIter>(); 
      m_Private->m_TreeViewPlaylist->m_CurrentId = boost::optional<gint64>();
	  Signals.PlayRequest.emit();
	}

	void
	PlaybackSourceMusicLib::on_plist_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
	{
      m_Private->m_TreeViewPlaylist->m_CurrentIter = m_Private->m_TreeViewPlaylist->ListStore->get_iter( path );
      m_Private->m_TreeViewPlaylist->m_CurrentId = (*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.RowId];
	  m_Private->m_TreeViewPlaylist->queue_draw();
	  Signals.PlayRequest.emit();
	}

    Glib::ustring
    PlaybackSourceMusicLib::get_uri () 
    {
		return (*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.Location];
	}

    Glib::ustring
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

		if(!rows.empty() && !m_Private->m_TreeViewPlaylist->m_CurrentIter)
		{
			m_Private->m_TreeViewPlaylist->m_CurrentIter = m_Private->m_TreeViewPlaylist->ListStore->get_iter (rows[0]); 
			m_Private->m_TreeViewPlaylist->m_CurrentId = (*m_Private->m_TreeViewPlaylist->m_CurrentIter.get())[m_Private->m_TreeViewPlaylist->PlaylistColumns.RowId];
			m_Private->m_TreeViewPlaylist->queue_draw();
			g_message("Got it");
			return true;
		}
		else
		if(m_Private->m_TreeViewPlaylist->m_CurrentIter)
			return true;
	
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
	}

	void
	PlaybackSourceMusicLib::set_play ()
	{
		m_caps = Caps (m_caps | PlaybackSource::C_CAN_PLAY);
		send_caps ();
	}

	void
	PlaybackSourceMusicLib::clear_play ()
	{
		m_caps = Caps (m_caps &~ PlaybackSource::C_CAN_PLAY);
		send_caps ();
	}

	void
	PlaybackSourceMusicLib::check_caps ()
	{
		if (!m_Private->m_TreeViewPlaylist->m_CurrentIter)
		{
			m_caps = Caps (m_caps &~ PlaybackSource::C_CAN_GO_NEXT);
			m_caps = Caps (m_caps &~ PlaybackSource::C_CAN_GO_PREV);
			m_caps = Caps (m_caps &~ PlaybackSource::C_CAN_PLAY);
			return;
		}

		TreeIter & iter = m_Private->m_TreeViewPlaylist->m_CurrentIter.get();

		TreePath path1 = m_Private->m_TreeViewPlaylist->ListStore->get_path(iter);
		TreePath path2 = m_Private->m_TreeViewPlaylist->ListStore->get_path(m_Private->m_TreeViewPlaylist->ListStore->children().begin());
		TreePath path3 = m_Private->m_TreeViewPlaylist->ListStore->get_path(--m_Private->m_TreeViewPlaylist->ListStore->children().end());

		if(path1[0] > path2[0])
		{
			m_caps = Caps (m_caps | PlaybackSource::C_CAN_GO_PREV);
			g_message("Can go previous");
		}
		else
		{
			m_caps = Caps (m_caps &~ PlaybackSource::C_CAN_GO_PREV);
			g_message("Can not go previous");
		}

		if(path1[0] < path3[0])
		{
			m_caps = Caps (m_caps | PlaybackSource::C_CAN_GO_NEXT);
			g_message("Can go next");
		}
		else
		{
			m_caps = Caps (m_caps &~ PlaybackSource::C_CAN_GO_NEXT);
			g_message("Can not go next");
		}
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
        m_caps = Caps (m_caps | PlaybackSource::C_CAN_PAUSE);
        m_caps = Caps (m_caps | PlaybackSource::C_CAN_PROVIDE_METADATA);

		check_caps ();
    }

    void
    PlaybackSourceMusicLib::next_post () 
	{ 
		m_Private->m_TreeViewPlaylist->queue_draw();
		check_caps ();
	}

    void
    PlaybackSourceMusicLib::prev_post ()
    {
		m_Private->m_TreeViewPlaylist->queue_draw();
		check_caps ();
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

} // end namespace Source
} // end namespace MPX 
