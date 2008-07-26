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
#include <giomm.h>
#include <libglademm.h>
#include <Python.h>
#define NO_IMPORT
#include <pygobject.h>
#include <boost/algorithm/string.hpp>


#include "mpx/hal.hh"
#include "mpx/library.hh"
#include "mpx/main.hh"
#include "mpx/playlistparser-xspf.hh"
#include "mpx/playlistparser-pls.hh"
#include "mpx/sql.hh"
#include "mpx/stock.hh"
#include "mpx/types.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/util-string.hh"
#include "mpx/widgetloader.h"

#include "mpx/widgets/cell-renderer-cairo-surface.hh"
#include "mpx/widgets/cell-renderer-count.hh"
#include "mpx/widgets/cell-renderer-vbox.hh"
#include "mpx/widgets/gossip-cell-renderer-expander.h"
#include "mpx/widgets/timed-confirmation.hh"

#include "glib-marshalers.h"
#include "musiclib-py.hh"
#include "source-musiclib.hh"
#include "top-albums-fetch-thread.hh"

#include "listview.hh"

using namespace Gtk;
using namespace Glib;
using namespace Gnome::Glade;
using namespace MPX;
using boost::get;
using boost::algorithm::trim;

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

    char const * ui_source =
    "<ui>"
    ""
    "<menubar name='MenubarMain'>"
    "   <placeholder name='placeholder-source'>"
    "     <menu action='menu-source-musiclib'>"
    "         <menuitem action='musiclib-sort-by-name'/>"
    "         <menuitem action='musiclib-sort-by-date'/>"
    "         <menuitem action='musiclib-sort-by-rating'/>"
    "         <menuitem action='musiclib-sort-by-alphabet'/>"
    "         <separator/>"
    "         <menuitem action='musiclib-show-only-new'/>"
    "         <separator/>"
    "         <menuitem action='musiclib-show-albums'/>"
    "         <menuitem action='musiclib-show-collections'/>"
    "         <menuitem action='musiclib-show-alltracks'/>"
    "         <separator/>"
    "         <menuitem action='musiclib-show-ccdialog'/>"
    "         <separator/>"
    "         <menuitem action='musiclib-action-recache-covers'/>"
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

    std::string
    ovariant_get_string (MPX::OVariant & self)
    {
        if(!self.is_initialized())
        {
            return std::string();
        }

        std::string output;

        switch(self.get().which())
        {
            case 0:
                output = boost::lexical_cast<std::string>(boost::get<gint64>(self.get()));
                break;
            case 1:
                output = boost::lexical_cast<std::string>(boost::get<double>(self.get()));
                break;
            case 2:
                output = boost::get<std::string>(self.get());
                break;
        }

        return output;
    }

    char const* attribute_names[] =
    {
        N_("File Location"),
        N_("Name"),
        N_("Genre"),
        N_("Comment"),
        N_("MusicIP PUID"),
        N_("File Hash"),
        N_("MusicBrainz Track ID"),
        N_("Artist"),
        N_("Artist Sort Name"),
        N_("MusicBrainz Artist ID"),
        N_("Album"),
        N_("MusicBrainz Album ID"),
        N_("Release Date"),
        N_("Release Country"),
        N_("Release Type"),
        N_("Amazon ASIN"),
        N_("Album Artist"),
        N_("Album Artist Sort Name"),
        N_("MusicBrainz Album Artist ID"),
        N_("MIME type"),
        N_("HAL Volume UDI"),
        N_("HAL Device UDI"),
        N_("Volume-relative Path"),
        N_("Insert Path"),
        N_("Location Name"),
        N_("Track"),
        N_("Time"),
        N_("Rating"),
        N_("Date"),
        N_("Change Time"),
        N_("Bitrate"),
        N_("Samplerate"),
        N_("Play Count"),
        N_("Play Date"),
        N_("Insert Date"),
        N_("Is MB Album Artist"),
        N_("Active"),
        N_("MPX Track ID"),
        N_("MPX Album ID")
    };

    typedef sigc::signal<void, int, bool> SignalColumnState;

    class ColumnControlView : public WidgetLoader<Gtk::TreeView>
    {
        public:
               
                struct Signals_t 
                {
                    SignalColumnState ColumnState;
                };

                Signals_t Signals;

                class Columns_t : public Gtk::TreeModelColumnRecord
                {
                    public: 

                            Gtk::TreeModelColumn<Glib::ustring> Name;
                            Gtk::TreeModelColumn<int>           ID;
                            Gtk::TreeModelColumn<bool>          Active;

                            Columns_t ()
                            {
                                add(Name);
                                add(ID);
                                add(Active);
                            };
                };


                Columns_t                       Columns;
                Glib::RefPtr<Gtk::ListStore>    Store;

                ColumnControlView (const Glib::RefPtr<Gnome::Glade::Xml>& xml)
                : WidgetLoader<Gtk::TreeView>(xml, "cc-treeview")
                {
                    Store = Gtk::ListStore::create(Columns); 
                    set_model(Store);

                    TreeViewColumn *col = manage( new TreeViewColumn(_("Active")));
                    CellRendererToggle *cell1 = manage( new CellRendererToggle );
                    cell1->property_xalign() = 0.5;
                    cell1->signal_toggled().connect(
                        sigc::mem_fun(
                            *this,
                            &ColumnControlView::on_cell_toggled
                    ));
                    col->pack_start(*cell1, true);
                    col->add_attribute(*cell1, "active", Columns.Active);
                    append_column(*col);            

                    append_column(_("Column"), Columns.Name);            

                    for( int i = 0; i < N_ATTRIBUTES_INT; ++i )
                    {
                        TreeIter iter = Store->append();
                        
                        (*iter)[Columns.Name]   = _(attribute_names[i]);
                        (*iter)[Columns.ID]     = i;
                        (*iter)[Columns.Active] = false;
                    }
                };

                void
                on_cell_toggled(Glib::ustring const& path)
                {
                    TreeIter iter = Store->get_iter(path);

                    bool active = (*iter)[Columns.Active];
                    (*iter)[Columns.Active] = !active;

                    Signals.ColumnState.emit((*iter)[Columns.ID], !active);
                }
    };

    class ColumnControlDialog : public WidgetLoader<Gtk::Dialog>
    {
                ColumnControlView *m_ControlView;
    
        public:

                ColumnControlDialog(const Glib::RefPtr<Gnome::Glade::Xml>& xml)
                : WidgetLoader<Gtk::Dialog>(xml, "cc-dialog")
                {
                    m_ControlView = new ColumnControlView(xml);
                }

                SignalColumnState&
                signal_column_state()
                {
                    return m_ControlView->Signals.ColumnState;
                }
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

    static const int N_STARS = 6;

    struct MusicLibPrivate
    {
        Glib::RefPtr<Gnome::Glade::Xml>     m_RefXml;
        Gtk::Widget                       * m_UI;

        class PlaylistTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              ColumnControlDialog                 * m_CCDialog;

              MPX::Source::PlaybackSourceMusicLib & m_MLib;

              PAccess<MPX::Library>                 m_Lib;
              PAccess<MPX::HAL>                     m_HAL;

              Glib::RefPtr<Gdk::Pixbuf>             m_Playing;
              Glib::RefPtr<Gdk::Pixbuf>             m_Bad;

              TreePath                              m_PathButtonPress;
              int                                   m_ButtonDepressed;

              Glib::RefPtr<Gtk::UIManager>          m_UIManager;
              Glib::RefPtr<Gtk::ActionGroup>        m_ActionGroup;      

            public:

              boost::optional<Gtk::TreeIter>        m_CurrentIter;
              boost::optional<Gtk::TreeIter>        m_PlayInitIter;
              boost::optional<gint64>               m_CurrentId;
              Glib::RefPtr<Gdk::Pixbuf>             m_Stars[N_STARS];

              PlaylistColumnsT                      PlaylistColumns;
              Glib::RefPtr<Gtk::ListStore>          ListStore;

              enum Column
              {
                C_PLAYING,
                C_TITLE,
                C_LENGTH,
                C_ARTIST,
                C_ALBUM,
                C_TRACK,
                C_RATING,
              };

              static const int N_FIRST_CUSTOM = 7;

              PlaylistTreeView(
                    Glib::RefPtr<Gnome::Glade::Xml> const& xml,
                    PAccess<MPX::Library>           const& lib,
                    PAccess<MPX::HAL>               const& hal,
                    MPX::Source::PlaybackSourceMusicLib  & mlib
              )
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-playlist")
              , m_MLib(mlib)
              , m_Lib(lib)
              , m_HAL(hal)
              , m_ButtonDepressed(0)
              {
                set_has_tooltip();
                set_rules_hint();

                m_Playing = Gdk::Pixbuf::create_from_file(Glib::build_filename(Glib::build_filename(DATA_DIR,"images"),"play.png"));
                m_Bad = render_icon (StockID (MPX_STOCK_ERROR), ICON_SIZE_SMALL_TOOLBAR);

                for(int n = 0; n < 6; ++n)
                {
                    m_Stars[n] = Gdk::Pixbuf::create_from_file(
                        build_filename(
                            build_filename(
                                DATA_DIR,
                                "images"
                            ),
                            (boost::format("stars%d.png") % n).str()
                    ));
                }

                TreeViewColumn * col = manage (new TreeViewColumn(""));
                CellRendererPixbuf * cell = manage (new CellRendererPixbuf);
                col->pack_start(*cell, true);
                col->set_min_width(24);
                col->set_max_width(24);
                col->set_cell_data_func(*cell, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncIcon ));
                append_column(*col);

                append_column(_("Name"), PlaylistColumns.Name);

                col = manage (new TreeViewColumn(_("Time")));
                CellRendererText * cell2 = manage (new CellRendererText);
                col->property_alignment() = 1.;
                col->pack_start(*cell2, true);
                col->set_cell_data_func(*cell2, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncTime ));
                col->set_sort_column_id(PlaylistColumns.Length);
                g_object_set(G_OBJECT(cell2->gobj()), "xalign", 1.0f, NULL);
                append_column(*col);

                append_column(_("Artist"), PlaylistColumns.Artist);
                append_column(_("Album"), PlaylistColumns.Album);
                append_column(_("Track"), PlaylistColumns.Track);

                col = manage (new TreeViewColumn(_("My Rating")));
                cell = manage (new CellRendererPixbuf);
                col->pack_start(*cell, false);
                col->set_min_width(66);
                col->set_max_width(66);
                col->set_cell_data_func(*cell, sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncRating ));
                append_column(*col);

                //////////////////////////////// 

                cell2 = manage (new CellRendererText);
                for( int i = 0; i < N_ATTRIBUTES_INT; ++i)
                {
                        col = manage (new TreeViewColumn(_(attribute_names[i])));
                        col->pack_start(*cell2, true);
                        col->set_cell_data_func(*cell2, sigc::bind(sigc::mem_fun( *this, &PlaylistTreeView::cellDataFuncCustom ), i ));
                        col->property_visible()= false;
                        append_column(*col);
                }
    
                ////////////////////////////////

                get_column(C_TITLE)->set_sort_column_id(PlaylistColumns.Name);
                get_column(C_LENGTH)->set_sort_column_id(PlaylistColumns.Length);
                get_column(C_ARTIST)->set_sort_column_id(PlaylistColumns.Artist);
                get_column(C_ALBUM)->set_sort_column_id(PlaylistColumns.Album);
                get_column(C_TRACK)->set_sort_column_id(PlaylistColumns.Track);
                get_column(C_RATING)->set_sort_column_id(PlaylistColumns.Rating);

                get_column(0)->set_resizable(false);
                get_column(1)->set_resizable(true);
                get_column(2)->set_resizable(false);
                get_column(3)->set_resizable(true);
                get_column(4)->set_resizable(true);
                get_column(5)->set_resizable(false);
                get_column(6)->set_resizable(false);

                ListStore = Gtk::ListStore::create(PlaylistColumns);

                ListStore->set_sort_func(PlaylistColumns.Artist,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByArtist ));
                ListStore->set_sort_func(PlaylistColumns.Album,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByAlbum ));
                ListStore->set_sort_func(PlaylistColumns.Track,
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortByTrack ));

                ListStore->set_default_sort_func(
                    sigc::mem_fun( *this, &PlaylistTreeView::slotSortDefault ));

                get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
                get_selection()->signal_changed().connect( sigc::mem_fun( *this, &PlaylistTreeView::on_selection_changed ) );

                set_headers_clickable();
                set_model(ListStore);

                m_UIManager = Gtk::UIManager::create();
                m_ActionGroup = Gtk::ActionGroup::create ("Actions_UiPartPlaylist-PlaylistList");

                m_ActionGroup->add  (Gtk::Action::create("dummy","dummy"));

                m_ActionGroup->add  (Gtk::Action::create (ACTION_PLAY,
                                    Gtk::StockID (GTK_STOCK_MEDIA_PLAY),
                                    _("Play")),
                                    sigc::mem_fun(mlib, &MPX::Source::PlaybackSourceMusicLib::action_cb_play));

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

                m_CCDialog = new ColumnControlDialog(xml);
                m_CCDialog->signal_column_state().connect(
                    sigc::mem_fun(
                        *this,
                        &PlaylistTreeView::on_column_toggled
                ));

				set_tooltip_text(_("Drag and drop albums, tracks and files here to add them to the playlist."));

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-album"));
                Entries.push_back(TargetEntry("mpx-track"));
                drag_dest_set(Entries, DEST_DEFAULT_MOTION);
                drag_dest_add_uri_targets();
              }

              void
              on_column_toggled( int column, bool state )
              {
                get_column(N_FIRST_CUSTOM + column)->property_visible() = state;
              }

              void
              action_cb_show_ccdialog ()
              {
                m_CCDialog->run();
                m_CCDialog->hide();
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
                    m_MLib.action_cb_go_to_album(id);
                  }
                  else
                    g_message("%s: No Album ID", G_STRLOC);
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

                  m_MLib.check_nextprev_caps();
                  m_MLib.send_caps ();

                  if(ListStore->children().empty())
                  {
                    m_MLib.plist_end(true);
                  }

                  check_for_end ();
              }

              virtual void
              action_cb_playlist_remove_remaining ()
              {
                  PathV p; 
                  TreePath path = ListStore->get_path(m_CurrentIter.get());
                  path.next();
        
                  while( guint(path.get_indices().data()[0]) < ListStore->children().size() )
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

                  m_MLib.check_nextprev_caps();
                  m_MLib.send_caps ();

                  check_for_end ();
              }

              void
              on_selection_changed ()
              {
                  if(get_selection()->count_selected_rows() == 1)
                      m_MLib.set_play();
                  else
                      m_MLib.clear_play();
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
                  m_MLib.check_nextprev_caps();
                  m_MLib.send_caps ();
                  columns_autosize();
                  m_MLib.plist_end(true);
              }

              void
              check_for_end ()
              {
                  if(m_CurrentIter)
                  {
                        if(TreePath (m_CurrentIter.get()) == TreePath(1, ListStore->children().size() - 1))
                        {
                            m_MLib.plist_end(false);
                        }
                  }
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
                      (*iter)[PlaylistColumns.Name] = get<std::string>(r["title"]);
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
                  (*iter)[PlaylistColumns.IsBad] = false; 
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
                      (*iter)[PlaylistColumns.Name] = get<std::string>(track[ATTRIBUTE_TITLE].get()); 

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
                  (*iter)[PlaylistColumns.IsBad] = false; 
              }

              void
              append_album (gint64 id)
              {
                  SQL::RowV v;
                  m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = '%lld' ORDER BY track") % id).str()); 
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
                      PlaylistParser::Base * pp = 0;

                      if(Util::str_has_suffix_nocase(*i, "xspf"))
                      {
                          pp = new PlaylistParser::XSPF;
                      }
                      else
                      if(Util::str_has_suffix_nocase(*i, "pls"))
                      {
                          pp = new PlaylistParser::PLS;
                      }

                      if( pp )
                      {
                          Track_v v;

                          pp->read(*i, v); 

                          for(Track_v::const_iterator i = v.begin(); i != v.end(); ++i)
                          {
                            MPX::Track t = *i;
                            if(!begin)
                                iter = ListStore->insert_after(iter);
                            place_track(t, iter); 
                          }

                          delete pp;
                      }

                      Track track;
                      m_Lib.get().getMetadata(*i, track);

                      if(!begin)
                      {
                          iter = ListStore->insert_after(iter);
                      }

                      begin = false;

                      place_track(track, iter);
                  }

                  if(!dirs.empty())
                  {
                      for(Util::FileList::const_iterator i = dirs.begin(); i != dirs.end(); ++i)
                      {
                            Util::FileList files;
                            Util::collect_audio_paths(*i, files);
                            append_uris(files,iter,begin);
                      }
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

                  m_MLib.check_nextprev_caps ();
                  m_MLib.send_caps ();
              }

              virtual bool
              on_button_press_event (GdkEventButton* event)
              {
                  int cell_x, cell_y ;
                  TreeViewColumn *col ;
                  TreePath path;

                  if(get_path_at_pos (event->x, event->y, path, col, cell_x, cell_y))
                  {
                      if(col == get_column(6))
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
                    if((ev->type == GDK_BUTTON_RELEASE) && m_CurrentIter)
                    {
                        check_for_end ();
                    }
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
              cellDataFuncTime (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                  CellRendererText *cell_t = dynamic_cast<CellRendererText*>(basecell);
                  guint64 Length = (*iter)[PlaylistColumns.Length]; 
                  g_object_set(G_OBJECT(cell_t->gobj()), "xalign", 1.0f, NULL);
                  cell_t->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
              }

              void
              cellDataFuncIcon (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                  CellRendererPixbuf *cell_p = dynamic_cast<CellRendererPixbuf*>(basecell);

                  if( (*iter)[PlaylistColumns.IsBad] )
                  {
                      cell_p->property_pixbuf() = m_Bad; 
                      return;
                  }

                  if( m_CurrentIter && m_CurrentIter.get() == iter )
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

              void
              cellDataFuncCustom (CellRenderer * basecell, TreeModel::iterator const &iter, int attribute)
              {
                  CellRendererText *cell_t = dynamic_cast<CellRendererText*>(basecell);
                  MPX::Track track = (*iter)[PlaylistColumns.MPXTrack]; 

                  if(track.has(attribute))
                  {
                    cell_t->property_text() = ovariant_get_string(track[attribute]);
                  }
                  else
                  {
                    cell_t->property_text() = ""; 
                  }
              }

              int
              slotSortDefault(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  return 0;
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
                  {
                      m_PlayInitIter = iter;
                  }

                  append_uris (uris, iter, begin);
                  m_MLib.check_nextprev_caps();
                  m_MLib.send_caps ();
              }

              bool
              check_current_exists ()
              {
                g_return_val_if_fail(m_CurrentIter,false);

                std::string location;
#ifdef HAVE_HAL
                try{
                    MPX::Track t ((*m_CurrentIter.get())[PlaylistColumns.MPXTrack]);
                    std::string volume_udi = get<std::string>(t[ATTRIBUTE_HAL_VOLUME_UDI].get());
                    std::string device_udi = get<std::string>(t[ATTRIBUTE_HAL_DEVICE_UDI].get());
                    std::string vrp = get<std::string>(t[ATTRIBUTE_VOLUME_RELATIVE_PATH].get());
                    std::string mount_point = m_HAL.get().get_mount_point_for_volume(volume_udi, device_udi);
                    std::string uri = build_filename(mount_point, vrp);
                    location = filename_to_uri(uri);
                } catch (HAL::NoMountPathForVolumeError & cxe)
                {
                    g_message("%s: Error: What: %s", G_STRLOC, cxe.what());
                }
#else
                location = (*m_CurrentIter.get())[PlaylistColumns.Location];
#endif // HAVE_HAL

                if( !location.empty() ) 
                {
                    try{
                        Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri( location );
                        bool exists = file->query_exists();
                        (*m_CurrentIter.get())[PlaylistColumns.IsBad] = !exists; 
                        return exists;
                    } catch( Glib::Error & cxe ) {
                        g_warning("%s: %s", G_STRFUNC, cxe.what().c_str());
                    }
    
                }
                
                return false;
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
            Gtk::TreeModelColumn<bool>                                  NewAlbum;

            Gtk::TreeModelColumn<std::string>                           AlbumSort;
            Gtk::TreeModelColumn<std::string>                           ArtistSort;

            Gtk::TreeModelColumn<gint64>                                Id;
            Gtk::TreeModelColumn<std::string>                           MBID;
            Gtk::TreeModelColumn<std::string>                           AlbumArtistMBID;

            Gtk::TreeModelColumn<gint64>                                Date;
            Gtk::TreeModelColumn<gint64>                                InsertDate;
            Gtk::TreeModelColumn<gint64>                                Rating;

            Gtk::TreeModelColumn<Glib::ustring>                         TrackTitle;
            Gtk::TreeModelColumn<Glib::ustring>                         TrackArtist;
            Gtk::TreeModelColumn<std::string>                           TrackArtistMBID;
            Gtk::TreeModelColumn<gint64>                                TrackNumber;
            Gtk::TreeModelColumn<gint64>                                TrackLength;
            Gtk::TreeModelColumn<gint64>                                TrackId;

            AlbumColumnsT ()
            {
                add (RowType);
                add (Image);
                add (Text);

                add (HasTracks);
                add (NewAlbum);

                add (AlbumSort);
                add (ArtistSort);

                add (Id);
                add (MBID);
                add (AlbumArtistMBID);

                add (Date);
                add (InsertDate);
                add (Rating);

                add (TrackTitle);
                add (TrackArtist);
                add (TrackArtistMBID);
                add (TrackNumber);
                add (TrackLength);
                add (TrackId);
            }
        };

        struct LFMColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
            Gtk::TreeModelColumn<AlbumRowType>                          RowType;
            Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> >            Image;
            Gtk::TreeModelColumn<Glib::ustring>                         Text;

            Gtk::TreeModelColumn<bool>                                  HasTracks;
            Gtk::TreeModelColumn<bool>                                  NewAlbum;

            Gtk::TreeModelColumn<std::string>                           AlbumSort;
            Gtk::TreeModelColumn<std::string>                           ArtistSort;

            Gtk::TreeModelColumn<gint64>                                Id;
            Gtk::TreeModelColumn<std::string>                           MBID;
            Gtk::TreeModelColumn<std::string>                           AlbumArtistMBID;

            Gtk::TreeModelColumn<gint64>                                Date;
            Gtk::TreeModelColumn<gint64>                                InsertDate;
            Gtk::TreeModelColumn<gint64>                                Rating;

            Gtk::TreeModelColumn<Glib::ustring>                         TrackTitle;
            Gtk::TreeModelColumn<Glib::ustring>                         TrackArtist;
            Gtk::TreeModelColumn<std::string>                           TrackArtistMBID;
            Gtk::TreeModelColumn<gint64>                                TrackNumber;
            Gtk::TreeModelColumn<gint64>                                TrackLength;
            Gtk::TreeModelColumn<gint64>                                TrackId;

            Gtk::TreeModelColumn<int>                                   IsMPXAlbum;

            LFMColumnsT ()
            {
                add (RowType);
                add (Image);
                add (Text);

                add (HasTracks);
                add (NewAlbum);

                add (AlbumSort);
                add (ArtistSort);

                add (Id);
                add (MBID);
                add (AlbumArtistMBID);

                add (Date);
                add (InsertDate);
                add (Rating);

                add (TrackTitle);
                add (TrackArtist);
                add (TrackArtistMBID);
                add (TrackNumber);
                add (TrackLength);
                add (TrackId);

                add (IsMPXAlbum);
            }
        };

        class AlbumTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              typedef std::set<TreeIter>                IterSet;
              typedef std::map<std::string, IterSet>    MBIDIterMap;
              typedef std::map<gint64, TreeIter>        IdIterMap; 

            public:

              Glib::RefPtr<Gtk::TreeStore>          TreeStore;
              Glib::RefPtr<Gtk::TreeModelFilter>    TreeStoreFilter;

            private:

            // treemodel stuff

              AlbumColumnsT                         AlbumColumns;

            // objects

              PAccess<MPX::Library>                 m_Lib;
              PAccess<MPX::Covers>                  m_Covers;
              MPX::Source::PlaybackSourceMusicLib&  m_MLib;

            // view mappings

              MBIDIterMap                           m_MBIDIterMap;
              IdIterMap                             m_AlbumIterMap;

            // disc+rating pixbufs
    
              Cairo::RefPtr<Cairo::ImageSurface>    m_DiscDefault;
              Glib::RefPtr<Gdk::Pixbuf>             m_DiscDefault_Pixbuf;
              Glib::RefPtr<Gdk::Pixbuf>             m_Stars[6];

            // DND state variables

              boost::optional<std::string>          m_DragMBID;
              boost::optional<gint64>               m_DragAlbumId;
              boost::optional<gint64>               m_DragTrackId;
              Gtk::TreePath                         m_PathButtonPress;

            // state variables
    
              bool                                  m_ButtonPressed;
              bool                                  m_ShowNew;

             // widgets

              Gtk::Entry*                           m_FilterEntry;
              Gtk::Label*                           m_LabelShowing;

            public:

              AlbumTreeView (Glib::RefPtr<Gnome::Glade::Xml> const& xml,    
                         PAccess<MPX::Library> const& lib, PAccess<MPX::Covers> const& amzn, MPX::Source::PlaybackSourceMusicLib & mlib)
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-albums")
              , m_Lib(lib)
              , m_Covers(amzn)
              , m_MLib(mlib)
              , m_ButtonPressed(false)
              , m_ShowNew(false)
              {
                for(int n = 0; n < 6; ++n)
                    m_Stars[n] = Gdk::Pixbuf::create_from_file(build_filename(build_filename(DATA_DIR,"images"),
                        (boost::format("stars%d.png") % n).str()));

                m_Lib.get().signal_new_album().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_album ));
                m_Lib.get().signal_new_track().connect( sigc::mem_fun( *this, &AlbumTreeView::on_new_track ));
                m_Lib.get().signal_album_updated().connect( sigc::mem_fun( *this, &AlbumTreeView::on_album_updated ));
                m_Lib.get().signal_reload().connect( sigc::mem_fun( *this, &AlbumTreeView::album_list_load ));

                m_Covers.get().signal_got_cover().connect( sigc::mem_fun( *this, &AlbumTreeView::on_got_cover ));

                xml->get_widget("label-showing", m_LabelShowing);

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
                celltext->property_height() = 72;
                celltext->property_ellipsize() = Pango::ELLIPSIZE_MIDDLE;
                cvbox->property_renderer1() = celltext;

                CellRendererPixbuf *cellpixbuf = manage (new CellRendererPixbuf);
                cellpixbuf->property_xalign() = 0.;
                cellpixbuf->property_ypad() = 2;
                cellpixbuf->property_xpad() = 2;
                cvbox->property_renderer2() = cellpixbuf;

                col->pack_start(*cvbox, true);
                col->set_cell_data_func(
                    *cvbox,
                    sigc::mem_fun(
                        *this,
                        &AlbumTreeView::cellDataFuncText1
                ));

                CellRendererCount *cellcount = manage (new CellRendererCount);
                cellcount->property_box() = BOX_NORMAL;
                col->pack_start(*cellcount, false);

                col->set_cell_data_func(
                    *cellcount,
                    sigc::mem_fun(
                        *this,
                        &AlbumTreeView::cellDataFuncText2
                ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);

                col->set_cell_data_func(
                    *celltext,
                    sigc::mem_fun(
                        *this,
                        &AlbumTreeView::cellDataFuncText3
                ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);

                col->set_cell_data_func(
                    *celltext,
                    sigc::mem_fun(
                    *this,
                    &AlbumTreeView::cellDataFuncText4
                ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                celltext->property_xalign() = 0.;
                celltext->property_xpad() = 2;

                col->set_cell_data_func(
                    *celltext,
                    sigc::mem_fun(
                        *this,
                        &AlbumTreeView::cellDataFuncText5
                ));

                append_column(*col);

                TreeStore = Gtk::TreeStore::create(AlbumColumns);

                TreeStoreFilter = Gtk::TreeModelFilter::create(TreeStore);

                TreeStoreFilter->set_visible_func(
                    sigc::mem_fun(
                        *this,
                        &AlbumTreeView::album_visible_func
                ));

                TreeStoreFilter->signal_row_inserted().connect((
                    sigc::hide(sigc::hide(sigc::mem_fun(
                        *this,
                        &AlbumTreeView::on_row_added_or_deleted
                )))));

                TreeStoreFilter->signal_row_deleted().connect((
                    sigc::hide(sigc::mem_fun(
                        *this,
                        &AlbumTreeView::on_row_added_or_deleted
                ))));


                set_model(TreeStoreFilter);

                TreeStore->set_sort_func(0 , sigc::mem_fun( *this, &AlbumTreeView::slotSortAlpha ));
                TreeStore->set_sort_func(1 , sigc::mem_fun( *this, &AlbumTreeView::slotSortDate ));
                TreeStore->set_sort_func(2 , sigc::mem_fun( *this, &AlbumTreeView::slotSortRating ));
                TreeStore->set_sort_func(3 , sigc::mem_fun( *this, &AlbumTreeView::slotSortStrictAlpha ));

                TreeStore->set_sort_column(0, Gtk::SORT_ASCENDING);

                m_DiscDefault_Pixbuf =
                    Gdk::Pixbuf::create_from_file(
                        build_filename(
                            DATA_DIR,
                            build_filename("images","disc-default.png")
                        )
                    )->scale_simple(90,90,Gdk::INTERP_BILINEAR);

                m_DiscDefault = Util::cairo_image_surface_from_pixbuf(m_DiscDefault_Pixbuf->scale_simple(90,90,Gdk::INTERP_BILINEAR));

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-album", TARGET_SAME_APP, 0x80));
                Entries.push_back(TargetEntry("mpx-track", TARGET_SAME_APP, 0x81));
                drag_source_set(Entries); 

                xml->get_widget("album-filter-entry", m_FilterEntry);

                m_FilterEntry->signal_changed().connect(
                    sigc::mem_fun(
                        *this,
                        &AlbumTreeView::on_filter_entry_changed
                ));

                album_list_load ();
              }

        protected:

              virtual void
              on_row_activated (const TreeModel::Path& path, TreeViewColumn* column)
              {
                TreeIter iter = TreeStore->get_iter (TreeStoreFilter->convert_path_to_child_path(path));
                if(path.get_depth() == ROW_ALBUM)
                {
                        gint64 id = (*iter)[AlbumColumns.Id];
                        m_MLib.play_album(id);
                }
                else
                {
                        gint64 id = (*iter)[AlbumColumns.TrackId];
                        IdV v;
                        v.push_back(id);
                        m_MLib.play_tracks(v);
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

                    std::string album_artist_mb_id = (*iter)[AlbumColumns.AlbumArtistMBID];
                    std::string track_artist_mb_id;

                    SQL::RowV v;
                    m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track;") % gint64((*iter)[AlbumColumns.Id])).str());

                    for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                    {
                        SQL::Row & r = *i;

                        TreeIter child = TreeStore->append(iter->children());

                        if(r.count("mb_artist_id"))
                        {
                            track_artist_mb_id = get<std::string>(r["mb_artist_id"]);
                        }
            
                        if( album_artist_mb_id != track_artist_mb_id )
                        {
                            (*child)[AlbumColumns.TrackArtist] = (boost::format (_("<small>by</small> %s")) % Markup::escape_text(get<std::string>(r["artist"]))).str();;
                        }

                        (*child)[AlbumColumns.TrackTitle] = get<std::string>(r["title"]);
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

                        if( (cell_x >= 124) && (cell_x <= 184) && (cell_y > 72) && (cell_y < 90))
                        {
                            int rating = ((cell_x - 124)+7) / 12;
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

              virtual bool
              on_button_release_event (GdkEventButton* event)
              {
                g_atomic_int_set(&m_ButtonPressed, 0);
                return false;
              }

              virtual bool
              on_motion_notify_event (GdkEventMotion *event)
              {
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

                TreePath path;              
                TreeViewColumn *col;
                int cell_x, cell_y;
                if(get_path_at_pos (x_orig, y_orig, path, col, cell_x, cell_y))
                {
                    TreeIter iter = TreeStore->get_iter(TreeStoreFilter->convert_path_to_child_path(path));
                    AlbumRowType rt = (*iter)[AlbumColumns.RowType];
                    if( rt == ROW_ALBUM )
                    {
                            if(!g_atomic_int_get(&m_ButtonPressed))
                                return false;

                            if( (cell_x >= 124) && (cell_x <= 184) && (cell_y >= 72) && (cell_y < 90))
                            {
                                int rating = ((cell_x - 124)+7) / 12;
                                (*iter)[AlbumColumns.Rating] = rating;  
                                m_Lib.get().albumRated(m_DragAlbumId.get(), rating);
                                return true;
                            }
                    }
                }
                return false;
              } 

        private:

              void
              on_got_cover(const Glib::ustring& mbid)
              {
                Cairo::RefPtr<Cairo::ImageSurface> surface;

                m_Covers.get().fetch(mbid, surface, COVER_SIZE_ALBUM_LIST);

                surface = Util::cairo_image_surface_round(surface, 9.5);
                Util::cairo_image_surface_rounded_border(surface, .5, 9.5);

                IterSet & set = m_MBIDIterMap[mbid];

                for(IterSet::iterator i = set.begin(); i != set.end(); ++i)
                {
                    (*(*i))[AlbumColumns.Image] = surface;
                }
              }

              void
              place_album (SQL::Row & r, gint64 id)
              {
                TreeIter iter = TreeStore->append();
                m_AlbumIterMap.insert(std::make_pair(id, iter));
                TreeStore->append(iter->children()); //create dummy/placeholder row for tracks

                (*iter)[AlbumColumns.RowType] = ROW_ALBUM; 
                (*iter)[AlbumColumns.HasTracks] = false; 
                (*iter)[AlbumColumns.NewAlbum] = get<gint64>(r["album_new"]);
                (*iter)[AlbumColumns.Image] = m_DiscDefault; 
                (*iter)[AlbumColumns.Id] = id; 

                std::string asin;
                std::string year; 
                std::string country;
                std::string artist;
                std::string type;

                std::string album = get<std::string>(r["album"]);

                if(r.count("album_rating"))
                {
                    (*iter)[AlbumColumns.Rating] = get<gint64>(r["album_rating"]);
                }

                if(r.count("album_insert_date"))
                {
                    (*iter)[AlbumColumns.InsertDate] = get<gint64>(r["album_insert_date"]);
                }

                if(r.count("amazon_asin"))
                {
                    asin = get<std::string>(r["amazon_asin"]);
                }

                if(r.count("mb_album_id"))
                {
                    std::string mbid = get<std::string>(r["mb_album_id"]);

                    IterSet & s = m_MBIDIterMap[mbid];
                    s.insert(iter);

                    (*iter)[AlbumColumns.MBID] = mbid; 
                }

                if(r.count("mb_album_artist_id"))
                {
                    std::string mb_album_artist_id = get<std::string>(r["mb_album_artist_id"]);
                    (*iter)[AlbumColumns.AlbumArtistMBID] = mb_album_artist_id; 
                }

                if(r.count("mb_release_date"))
                {
                    year = get<std::string>(r["mb_release_date"]);
                    if(year.size())
                    {
                        year = year.substr(0,4);
                        try{
                            (*iter)[AlbumColumns.Date] = boost::lexical_cast<int>(year);
                        } catch( boost::bad_lexical_cast ) {
                        } 
                    }
                }
                else
                {
                    (*iter)[AlbumColumns.Date] = 0; 
                }

                if(r.count("mb_release_country"))
                {
                    country = get<std::string>(r["mb_release_country"]); 
                }

                if(r.count("mb_release_type"))
                {
                    type = get<std::string>(r["mb_release_type"]); 
                }

                if(r.find("album_artist_sortname") != r.end())
                {
                    artist = get<std::string>(r["album_artist_sortname"]);
                }
                else
                {
                    artist = get<std::string>(r["album_artist"]);
                }

                trim(country);
                trim(year);
                trim(type);

                if( !country.empty() )
                {
                        (*iter)[AlbumColumns.Text] =
                            (boost::format("<span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3% %4%</span>\n<span size='8000'>%5%</span>")
                                % Markup::escape_text(album).c_str()
                                % Markup::escape_text(artist).c_str()
                                % country
                                % year
                                % type
                            ).str();
                }
                else
                {
                        (*iter)[AlbumColumns.Text] =
                            (boost::format("<span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3%</span>\n<span size='8000'>%4%</span>")
                                % Markup::escape_text(album).c_str()
                                % Markup::escape_text(artist).c_str()
                                % year
                                % type
                            ).str();
                }


                (*iter)[AlbumColumns.AlbumSort] = ustring(album).collate_key();
                (*iter)[AlbumColumns.ArtistSort] = ustring(artist).collate_key();
              } 
   
              void
              update_album (SQL::Row & r, gint64 id)
              {
                IdIterMap::iterator i = m_AlbumIterMap.find(id);
                if(i  == m_AlbumIterMap.end())
                    return;

                TreeIter iter = (*i).second; 

                (*iter)[AlbumColumns.NewAlbum] = get<gint64>(r["album_new"]);

                std::string asin;
                std::string year; 
                std::string country;
                std::string artist;
                std::string type;

                std::string album = get<std::string>(r["album"]);

                if(r.count("album_rating"))
                {
                    (*iter)[AlbumColumns.Rating] = get<gint64>(r["album_rating"]);
                }

                if(r.count("album_insert_date"))
                {
                    (*iter)[AlbumColumns.InsertDate] = get<gint64>(r["album_insert_date"]);
                }

                if(r.count("amazon_asin"))
                {
                    asin = get<std::string>(r["amazon_asin"]);
                }

                if(r.count("mb_album_id"))
                {
                    std::string mbid = get<std::string>(r["mb_album_id"]);

                    IterSet & s = m_MBIDIterMap[mbid];
                    s.insert(iter);

                    (*iter)[AlbumColumns.MBID] = mbid; 
                }

                if(r.count("mb_release_date"))
                {
                    year = get<std::string>(r["mb_release_date"]);
                    if(year.size())
                    {
                        year = year.substr(0,4);
                        try{
                            (*iter)[AlbumColumns.Date] = boost::lexical_cast<int>(year);
                        } catch( boost::bad_lexical_cast ) {
                        } 
                    }
                }
                else
                {
                    (*iter)[AlbumColumns.Date] = 0; 
                }

                if(r.count("mb_release_country"))
                {
                    country = get<std::string>(r["mb_release_country"]); 
                }

                if(r.count("mb_release_type"))
                {
                    type = get<std::string>(r["mb_release_type"]); 
                }

                if(r.find("album_artist_sortname") != r.end())
                {
                    artist = get<std::string>(r["album_artist_sortname"]);
                }
                else
                {
                    artist = get<std::string>(r["album_artist"]);
                }

                trim(country);
                trim(year);
                trim(type);

                if( !country.empty() )
                {
                        (*iter)[AlbumColumns.Text] =
                            (boost::format("<span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3% %4%</span>\n<span size='8000'>%5%</span>")
                                % Markup::escape_text(album).c_str()
                                % Markup::escape_text(artist).c_str()
                                % country
                                % year
                                % type
                            ).str();
                }
                else
                {
                        (*iter)[AlbumColumns.Text] =
                            (boost::format("<span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3%</span>\n<span size='8000'>%4%</span>")
                                % Markup::escape_text(album).c_str()
                                % Markup::escape_text(artist).c_str()
                                % year
                                % type
                            ).str();
                }


                (*iter)[AlbumColumns.AlbumSort] = ustring(album).collate_key();
                (*iter)[AlbumColumns.ArtistSort] = ustring(artist).collate_key();
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
                    m_Covers.get().cache(get<std::string>(r["mb_album_id"]));
                }
              }

              void
              on_album_updated(gint64 id)
              {
                SQL::RowV v;
                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album.id = %lld;") % id).str());

                g_return_if_fail(!v.empty());

                SQL::Row & r = v[0];

                update_album (r, id); 
              }

              void
              on_new_album(gint64 id)
              {
                SQL::RowV v;
                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album.id = %lld;") % id).str());

                g_return_if_fail(!v.empty());

                SQL::Row & r = v[0];

                place_album (r, id); 
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
                        if(track[ATTRIBUTE_MB_ARTIST_ID])
                            (*child)[AlbumColumns.TrackArtistMBID] = get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get());
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

              int
              slotSortStrictAlpha(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[AlbumColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[AlbumColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  std::string alb_a = (*iter_a)[AlbumColumns.AlbumSort];
                  std::string alb_b = (*iter_b)[AlbumColumns.AlbumSort];
                  std::string art_a = (*iter_a)[AlbumColumns.ArtistSort];
                  std::string art_b = (*iter_b)[AlbumColumns.ArtistSort];

                  if(art_a != art_b)
                      return art_a.compare(art_b);

                  return alb_a.compare(alb_b);
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
                    {
                        cell1->property_markup() = (*iter)[AlbumColumns.Text]; 
                    }

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
                CellRendererCount *cell = dynamic_cast<CellRendererCount*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_text() = (boost::format("%lld") % (*iter)[AlbumColumns.TrackNumber]).str();
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

              void
              cellDataFuncText5 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = (*iter)[AlbumColumns.TrackArtist];
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
                                "render", TRUE,
                                "expander-style", row_expanded ? GTK_EXPANDER_EXPANDED : GTK_EXPANDER_COLLAPSED,
                                NULL);
                  } else {
                      g_object_set (cell, "visible", FALSE, "render", FALSE, NULL);
                  }
              }

              bool
              album_visible_func (TreeIter const& iter)
              {
                  AlbumRowType rt = (*iter)[AlbumColumns.RowType];

                  if( m_ShowNew && rt == ROW_ALBUM ) 
                  {
                    bool visible = (*iter)[AlbumColumns.NewAlbum];
                    if( !visible ) 
                        return false;
                  } 

                  ustring filter (ustring (m_FilterEntry->get_text()).lowercase());

                  TreePath path (TreeStore->get_path(iter));

                  if( filter.empty() || path.get_depth() > 1)
                    return true;
                  else
                    return (Util::match_keys (ustring((*iter)[AlbumColumns.Text]).lowercase(), filter)); 
              }
    
              void
              update_album_count_display ()
              {
                    TreeNodeChildren::size_type n1 = TreeStoreFilter->children().size();
                    TreeNodeChildren::size_type n2 = TreeStore->children().size();
                    m_LabelShowing->set_text ((boost::format (_("showing %lld of %lld albums")) % n1 % n2).str());
              }

              void
              on_row_added_or_deleted ()
              {
                    update_album_count_display ();
              }

              void
              on_filter_entry_changed ()
              {
                    TreeStoreFilter->refilter();
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

              void
              set_new_albums_state (int state)
              {
                 m_ShowNew = state;
                 TreeStoreFilter->refilter();
              }

        };

        class LFMTreeView
            :   public WidgetLoader<Gtk::TreeView>
            ,   public sigx::glib_auto_dispatchable
        {
              typedef std::set<TreeIter>                IterSet;
              typedef std::map<std::string, IterSet>    MBIDIterMap;
              typedef std::map<gint64, TreeIter>        IdIterMap; 

            public:

              Glib::RefPtr<Gtk::TreeStore>          TreeStore;
              Glib::RefPtr<Gtk::TreeModelFilter>    TreeStoreFilter;

            private:

              LFMColumnsT                           LFMColumns;
              PAccess<MPX::Library>                 m_Lib;
              PAccess<MPX::Covers>                  m_Covers;
              MPX::Source::PlaybackSourceMusicLib&  m_MLib;

              MBIDIterMap                           m_MBIDIterMap;
              IdIterMap                             m_AlbumIterMap;

              Cairo::RefPtr<Cairo::ImageSurface>    m_DiscDefault;
              Glib::RefPtr<Gdk::Pixbuf>             m_DiscDefault_Pixbuf;
              Glib::RefPtr<Gdk::Pixbuf>             m_Stars[6];

              boost::optional<std::string>          m_DragMBID;
              boost::optional<gint64>               m_DragAlbumId;
              boost::optional<gint64>               m_DragTrackId;
              Gtk::TreePath                         m_PathButtonPress;

              bool                                  m_ButtonPressed;
              bool                                  m_DragSource;

              Gtk::Entry*                           m_FilterEntry;
              TopAlbumsFetchThread                * m_Thread;


              virtual void
              on_row_activated (const TreeModel::Path& path, TreeViewColumn* column)
              {
                TreeIter iter = TreeStore->get_iter (TreeStoreFilter->convert_path_to_child_path(path));
                if(path.get_depth() == ROW_ALBUM)
                {
                        if( (*iter)[LFMColumns.IsMPXAlbum] )
                        {
                            gint64 id = (*iter)[LFMColumns.Id];
                            m_MLib.play_album(id);
                        }
                }
                else
                {
                        gint64 id = (*iter)[LFMColumns.TrackId];
                        IdV v;
                        v.push_back(id);
                        m_MLib.play_tracks(v);
                }
              }

              virtual void
              on_row_expanded (const TreeIter &iter_filter,
                               const TreePath &path) 
              {
                TreeIter iter = TreeStoreFilter->convert_iter_to_child_iter(iter_filter);
                if(!(*iter)[LFMColumns.HasTracks])
                {
                    GtkTreeIter children;
                    bool has_children = (gtk_tree_model_iter_children(GTK_TREE_MODEL(TreeStore->gobj()), &children, const_cast<GtkTreeIter*>(iter->gobj())));

                    SQL::RowV v;
                    m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track;") % gint64((*iter)[LFMColumns.Id])).str());

                    for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                    {
                        SQL::Row & r = *i;
                        TreeIter child = TreeStore->append(iter->children());
                        (*child)[LFMColumns.TrackTitle] = get<std::string>(r["title"]);
                        (*child)[LFMColumns.TrackArtist] = get<std::string>(r["artist"]);
                        (*child)[LFMColumns.TrackNumber] = get<gint64>(r["track"]);
                        (*child)[LFMColumns.TrackLength] = get<gint64>(r["time"]);
                        (*child)[LFMColumns.TrackId] = get<gint64>(r["id"]);
                        (*child)[LFMColumns.RowType] = ROW_TRACK; 
                    }

                    if(v.size())
                    {
                        (*iter)[LFMColumns.HasTracks] = true;
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
                    AlbumRowType rt = (*iter)[LFMColumns.RowType];
                    if( rt == ROW_ALBUM )
                    {
                             if( (*iter)[LFMColumns.IsMPXAlbum] ) 
                             {
                                std::vector<TargetEntry> Entries;
                                Entries.push_back(TargetEntry("mpx-album", TARGET_SAME_APP, 0x80));
                                Entries.push_back(TargetEntry("mpx-track", TARGET_SAME_APP, 0x81));
                                drag_source_set(Entries); 
                                m_DragSource = true;

                                if(!g_atomic_int_get(&m_ButtonPressed))
                                    return false;

                                if( (cell_x >= 102) && (cell_x <= 162) && (cell_y >= 65) && (cell_y <=78))
                                {
                                    int rating = ((cell_x - 102)+7) / 12;
                                    (*iter)[LFMColumns.Rating] = rating;  
                                    m_Lib.get().albumRated(m_DragAlbumId.get(), rating);
                                    return true;
                                }
                             }
                             else
                             {
                                drag_source_unset ();
                                m_DragSource = false;
                             }
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
                    if(m_PathButtonPress.get_depth() == ROW_ALBUM && (*iter)[LFMColumns.IsMPXAlbum])
                    {
                        m_DragMBID = (*iter)[LFMColumns.MBID];
                        m_DragAlbumId = (*iter)[LFMColumns.Id];
                        m_DragTrackId.reset(); 
                
                        g_atomic_int_set(&m_ButtonPressed, 1);

                        if( (cell_x >= 102) && (cell_x <= 162) && (cell_y >= 65) && (cell_y <=78))
                        {
                            int rating = ((cell_x - 102)+7) / 12;
                            (*iter)[LFMColumns.Rating] = rating;  
                            m_Lib.get().albumRated(m_DragAlbumId.get(), rating);
                        }
                    }
                    else
                    if(m_PathButtonPress.get_depth() == ROW_TRACK)
                    {
                        m_DragMBID.reset(); 
                        m_DragAlbumId.reset();
                        m_DragTrackId = (*iter)[LFMColumns.TrackId];
                    }
                }
                TreeView::on_button_press_event(event);
                return false;
              }

              void
              on_got_cover(const Glib::ustring& mbid)
              {
                Cairo::RefPtr<Cairo::ImageSurface> surface;
                m_Covers.get().fetch(mbid, surface, COVER_SIZE_ALBUM_LIST);

                Util::cairo_image_surface_border(surface, 2.);
                surface = Util::cairo_image_surface_round(surface, 6.);

                IterSet & set = m_MBIDIterMap[mbid];
                for(IterSet::iterator i = set.begin(); i != set.end(); ++i)
                {
                    (*(*i))[LFMColumns.Image] = Util::cairo_image_surface_to_pixbuf(surface);
                }
              }

              void
              place_album (SQL::Row & r, gint64 id)
              {
                TreeIter iter = TreeStore->append();
                m_AlbumIterMap.insert(std::make_pair(id, iter));
                TreeStore->append(iter->children()); //create dummy/placeholder row for tracks

                (*iter)[LFMColumns.RowType] = ROW_ALBUM; 
                (*iter)[LFMColumns.HasTracks] = false; 
                (*iter)[LFMColumns.NewAlbum] = get<gint64>(r["album_new"]);
                (*iter)[LFMColumns.Id] = id; 
                (*iter)[LFMColumns.IsMPXAlbum] = 1;

                if(r.count("album_rating"))
                {
                    gint64 rating = get<gint64>(r["album_rating"]);
                    (*iter)[LFMColumns.Rating] = rating;
                }

                gint64 idate = 0;
                if(r.count("album_insert_date"))
                {
                    idate = get<gint64>(r["album_insert_date"]);
                    (*iter)[LFMColumns.InsertDate] = idate;
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

                    (*iter)[LFMColumns.MBID] = mbid; 

                    Cairo::RefPtr<Cairo::ImageSurface> surface;
                    if( m_Covers.get().fetch( mbid, surface, COVER_SIZE_ALBUM_LIST ))
                    {
                        surface = Util::cairo_image_surface_round(surface, 9.5);
                        Util::cairo_image_surface_rounded_border(surface, .5, 9.5);
                        (*iter)[LFMColumns.Image] = Util::cairo_image_surface_to_pixbuf(surface);
                    }
                    else
                        (*iter)[LFMColumns.Image] = m_DiscDefault_Pixbuf;
                }
                else
                    (*iter)[LFMColumns.Image] = m_DiscDefault_Pixbuf;

                std::string date; 
                if(r.count("mb_release_date"))
                {
                    date = get<std::string>(r["mb_release_date"]);
                    if(date.size())
                    {
                        gint64 date_int;
                        sscanf(date.c_str(), "%04lld", &date_int);
                        (*iter)[LFMColumns.Date] = date_int; 
                        date = date.substr(0,4) + ", ";
                    }
                }
                else
                    (*iter)[LFMColumns.Date] = 0; 

                std::string ArtistSort;
                if(r.find("album_artist_sortname") != r.end())
                    ArtistSort = get<std::string>(r["album_artist_sortname"]);
                else
                    ArtistSort = get<std::string>(r["album_artist"]);

                (*iter)[LFMColumns.Text] =
                    (boost::format("<span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3%Added: %4%</span>")
                        % Markup::escape_text(get<std::string>(r["album"])).c_str()
                        % Markup::escape_text(ArtistSort).c_str()
                        % date
                        % get_timestr_from_time_t(idate)
                    ).str();

                (*iter)[LFMColumns.AlbumSort] = ustring(get<std::string>(r["album"])).collate_key();
                (*iter)[LFMColumns.ArtistSort] = ustring(ArtistSort).collate_key();
              } 
   
              void
              on_new_track(Track & track, gint64 album_id, gint64 artist_id)
              {
                if(m_AlbumIterMap.count(album_id))
                {
                    TreeIter iter = m_AlbumIterMap[album_id];
                    if (((*iter)[LFMColumns.HasTracks]))
                    {
                        TreeIter child = TreeStore->append(iter->children());
                        if(track[ATTRIBUTE_TITLE])
                            (*child)[LFMColumns.TrackTitle] = get<std::string>(track[ATTRIBUTE_TITLE].get());
                        if(track[ATTRIBUTE_ARTIST])
                            (*child)[LFMColumns.TrackArtist] = get<std::string>(track[ATTRIBUTE_ARTIST].get());
                        if(track[ATTRIBUTE_MB_ARTIST_ID])
                            (*child)[LFMColumns.TrackArtistMBID] = get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get());
                        if(track[ATTRIBUTE_TRACK])
                            (*child)[LFMColumns.TrackNumber] = get<gint64>(track[ATTRIBUTE_TRACK].get());
                        if(track[ATTRIBUTE_TIME])
                            (*child)[LFMColumns.TrackLength] = get<gint64>(track[ATTRIBUTE_TIME].get());

                        (*child)[LFMColumns.TrackId] = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get());
                        (*child)[LFMColumns.RowType] = ROW_TRACK; 
                    }
                }
                else
                    g_warning("%s: Got new track without associated album! Consistency error!", G_STRLOC);
              }

              int
              slotSortRating(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[LFMColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[LFMColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[LFMColumns.Rating];
                  gint64 alb_b = (*iter_b)[LFMColumns.Rating];

                  return alb_b - alb_a;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[LFMColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[LFMColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
              }

              int
              slotSortAlpha(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[LFMColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[LFMColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[LFMColumns.Date];
                  gint64 alb_b = (*iter_b)[LFMColumns.Date];
                  std::string art_a = (*iter_a)[LFMColumns.ArtistSort];
                  std::string art_b = (*iter_b)[LFMColumns.ArtistSort];

                  if(art_a != art_b)
                      return art_a.compare(art_b);

                  return alb_a - alb_b;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[LFMColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[LFMColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
              }

              int
              slotSortDate(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[LFMColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[LFMColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[LFMColumns.InsertDate];
                  gint64 alb_b = (*iter_b)[LFMColumns.InsertDate];

                  return alb_b - alb_a;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[LFMColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[LFMColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
              }

              int
              slotSortStrictAlpha(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                AlbumRowType rt_a = (*iter_a)[LFMColumns.RowType];
                AlbumRowType rt_b = (*iter_b)[LFMColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  std::string alb_a = (*iter_a)[LFMColumns.AlbumSort];
                  std::string alb_b = (*iter_b)[LFMColumns.AlbumSort];
                  std::string art_a = (*iter_a)[LFMColumns.ArtistSort];
                  std::string art_b = (*iter_b)[LFMColumns.ArtistSort];

                  if(art_a != art_b)
                      return art_a.compare(art_b);

                  return alb_a.compare(alb_b);
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[LFMColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[LFMColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
              }
              void
              cellDataFuncCover (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererPixbuf *cell = dynamic_cast<CellRendererPixbuf*>(basecell);
                if(path.get_depth() == ROW_ALBUM)
                {
                    cell->property_visible() = true;
                    cell->property_pixbuf() = (*iter)[LFMColumns.Image]; 
                    cell->property_sensitive() = (*iter)[LFMColumns.IsMPXAlbum];
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
                    {
                        cell1->property_markup() = (*iter)[LFMColumns.Text]; 
                    }

                    if(cell2)
                    {
                        gint64 i = ((*iter)[LFMColumns.Rating]);
                        g_return_if_fail((i >= 0) && (i <= 5));
                        cell2->property_pixbuf() = m_Stars[i];
                    }

                    cell1->property_sensitive() = (*iter)[LFMColumns.IsMPXAlbum];
                    cell2->property_sensitive() = (*iter)[LFMColumns.IsMPXAlbum];
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
                CellRendererCount *cell = dynamic_cast<CellRendererCount*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_text() = (boost::format("%lld") % (*iter)[LFMColumns.TrackNumber]).str();
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
                    cell->property_markup() = Markup::escape_text((*iter)[LFMColumns.TrackTitle]);
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
                    gint64 Length = (*iter)[LFMColumns.TrackLength];
                    cell->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
                }
                else
                {
                    cell->property_visible() = false; 
                }
              }

              void
              cellDataFuncText5 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = (boost::format (_("<small>by</small> %s")) % Markup::escape_text((*iter)[LFMColumns.TrackArtist])).str();
                }
                else
                {
                    cell->property_visible() = false; 
                }
              }

              static void
              rb_sourcelist_expander_cell_data_func (GtkTreeViewColumn *column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model_,
                                     GtkTreeIter       *iter_,
                                     gpointer           data) 
              {
                  GtkTreePath *path_ = gtk_tree_model_get_path (model_, iter_);

                  if (gtk_tree_model_iter_has_child (model_, iter_))
                  {
                      gboolean row_expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (column->tree_view), path_);

                      g_object_set (cell,
                                "visible", TRUE,
                                "render", TRUE,
                                "expander-style", row_expanded ? GTK_EXPANDER_EXPANDED : GTK_EXPANDER_COLLAPSED,
                                NULL);
                  }
                  else
                  {
                     if(gtk_tree_path_get_depth(path_) == 1)
                     {
                          g_object_set (cell,
                                "visible", TRUE,
                                "render", FALSE,
                                NULL);
                     }
                     else
                          g_object_set (cell,
                                "visible", FALSE,
                                "render", FALSE,
                                NULL);
                  }

                  gtk_tree_path_free (path_);
              }

              bool
              album_visible_func (TreeIter const& iter)
              {
                  ustring filter (ustring (m_FilterEntry->get_text()).lowercase());
                  TreePath path (TreeStore->get_path(iter));

                  if( filter.empty() || path.get_depth() > 1)
                    return true;
                  else
                    return (Util::match_keys (ustring((*iter)[LFMColumns.Text]).lowercase(), filter)); 
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

              bool
              slot_select(
                const Glib::RefPtr <Gtk::TreeModel>&    model,
                const Gtk::TreeModel::Path&             path,
                bool                                    was_selected
              )
              {
                TreeIter iter = TreeStoreFilter->get_iter(path);
                return (*iter)[LFMColumns.IsMPXAlbum] || path.get_depth() == 2;
              }

              LFMTreeView (Glib::RefPtr<Gnome::Glade::Xml> const& xml,    
                         PAccess<MPX::Library> const& lib, PAccess<MPX::Covers> const& amzn, MPX::Source::PlaybackSourceMusicLib & mlib)
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-lfm")
              , m_Lib(lib)
              , m_Covers(amzn)
              , m_MLib(mlib)
              , m_ButtonPressed(false)
              , m_DragSource(true)
              {
                for(int n = 0; n < 6; ++n)
                    m_Stars[n] = Gdk::Pixbuf::create_from_file(build_filename(build_filename(DATA_DIR,"images"),
                        (boost::format("stars%d.png") % n).str()));

                m_Lib.get().signal_new_track().connect( 
                    sigc::mem_fun( *this, &LFMTreeView::on_new_track
                ));

                xml->get_widget("lfm-artist-entry", m_FilterEntry);

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

                CellRendererPixbuf * cellpixbuf = manage (new CellRendererPixbuf);
                col->pack_start(*cellpixbuf, false);
                col->set_cell_data_func(*cellpixbuf, sigc::mem_fun( *this, &LFMTreeView::cellDataFuncCover ));
                cellpixbuf->property_xpad() = 4;
                cellpixbuf->property_ypad() = 4;
                cellpixbuf->property_yalign() = 0.;
                cellpixbuf->property_xalign() = 0.;

                CellRendererVBox *cvbox = manage (new CellRendererVBox);

                CellRendererText *celltext = manage (new CellRendererText);
                celltext->property_yalign() = 0.;
                celltext->property_ypad() = 4;
                celltext->property_height() = 52;
                celltext->property_ellipsize() = Pango::ELLIPSIZE_MIDDLE;
                cvbox->property_renderer1() = celltext;

                cellpixbuf = manage (new CellRendererPixbuf);
                cellpixbuf->property_xalign() = 0.;
                cellpixbuf->property_ypad() = 2;
                cellpixbuf->property_xpad() = 2;
                cvbox->property_renderer2() = cellpixbuf;

                col->pack_start(*cvbox, true);
                col->set_cell_data_func(*cvbox, sigc::mem_fun( *this, &LFMTreeView::cellDataFuncText1 ));

                CellRendererCount *cellcount = manage (new CellRendererCount);
                col->pack_start(*cellcount, false);
                col->set_cell_data_func(*cellcount, sigc::mem_fun( *this, &LFMTreeView::cellDataFuncText2 ));
                cellcount->property_box() = BOX_NORMAL;

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &LFMTreeView::cellDataFuncText3 ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &LFMTreeView::cellDataFuncText4 ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                celltext->property_xalign() = 0.;
                celltext->property_xpad() = 2;
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &LFMTreeView::cellDataFuncText5 ));
                append_column(*col);


                TreeStore = Gtk::TreeStore::create(LFMColumns);
                TreeStoreFilter = Gtk::TreeModelFilter::create(TreeStore);
                set_model(TreeStoreFilter);

                TreeStore->set_sort_func(0 , sigc::mem_fun( *this, &LFMTreeView::slotSortAlpha ));
                TreeStore->set_sort_func(1 , sigc::mem_fun( *this, &LFMTreeView::slotSortDate ));
                TreeStore->set_sort_func(2 , sigc::mem_fun( *this, &LFMTreeView::slotSortRating ));
                TreeStore->set_sort_func(3 , sigc::mem_fun( *this, &LFMTreeView::slotSortStrictAlpha ));

                m_DiscDefault_Pixbuf = Gdk::Pixbuf::create_from_file(build_filename(DATA_DIR, build_filename("images","disc-default.png")))->scale_simple(90,90,Gdk::INTERP_BILINEAR);
                m_DiscDefault = Util::cairo_image_surface_from_pixbuf(m_DiscDefault_Pixbuf->scale_simple(90,90,Gdk::INTERP_BILINEAR));

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-album", TARGET_SAME_APP, 0x80));
                Entries.push_back(TargetEntry("mpx-track", TARGET_SAME_APP, 0x81));
                drag_source_set(Entries); 

                m_FilterEntry->signal_activate().connect(
                    sigc::mem_fun( *this, &LFMTreeView::on_artist_entry_activated
                ));

                m_FilterEntry->signal_changed().connect(
                    sigc::mem_fun( *this, &LFMTreeView::on_artist_entry_changed
                ));

                m_Thread = new TopAlbumsFetchThread;
                m_Thread->run();

                m_Thread->SignalAlbum().connect(
                    sigc::mem_fun(*this, &LFMTreeView::on_thread_new_album
                ));

                m_Thread->SignalStopped().connect(
                    sigc::mem_fun(*this, &LFMTreeView::on_thread_stopped
                ));

                get_selection()->set_select_function(
                    sigc::mem_fun(
                        *this,
                        &LFMTreeView::slot_select
                ));
              }

              void
              on_artist_entry_changed ()
              {
                m_Thread->RequestStop();
              }

              void
              on_artist_entry_activated ()
              {
                m_Thread->RequestStop();
                m_Thread->RequestLoad(m_FilterEntry->get_text());
              }

              void
              on_thread_new_album(Glib::RefPtr<Gdk::Pixbuf> icon, std::string const& artist, std::string const& album)
              {
                SQL::RowV v;
                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album LIKE '%s' AND album_artist LIKE '%s';") % mprintf("%q",album.c_str()) % mprintf("%q", artist.c_str())).str());

                if(!v.empty())
                {
                    SQL::Row & r = v[0];
                    place_album (r, get<gint64>(r["id"])); 
                }
                else
                {
                    if( icon )
                    {
                        TreeIter iter = TreeStore->append();

                        Cairo::RefPtr<Cairo::ImageSurface> surface =
                            Util::cairo_image_surface_from_pixbuf(
                                icon->scale_simple(
                                    90,
                                    90,
                                    Gdk::INTERP_BILINEAR
                            ));

                        surface = Util::cairo_image_surface_round(surface, 9.5);
                        Util::cairo_image_surface_rounded_border(surface, .5, 9.5);

                        (*iter)[LFMColumns.Image]       = Util::cairo_image_surface_to_pixbuf(surface);
                        (*iter)[LFMColumns.RowType]     = ROW_ALBUM;
                        (*iter)[LFMColumns.HasTracks]   = false;
                        (*iter)[LFMColumns.Id]          = 0;
                        (*iter)[LFMColumns.IsMPXAlbum]  = 0;

                        (*iter)[LFMColumns.Text] =
                            (boost::format("<span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>")
                                % Markup::escape_text(album).c_str()
                                % Markup::escape_text(artist).c_str()
                            ).str();

                        (*iter)[LFMColumns.AlbumSort] = ustring(album).collate_key();
                        (*iter)[LFMColumns.ArtistSort] = ustring(artist).collate_key();
                    }
                }
              }

              void
              on_thread_stopped ()
              {
                  TreeStore->clear();
              }

        };

        struct AllTracksColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
            Gtk::TreeModelColumn<Glib::ustring> Artist;
            Gtk::TreeModelColumn<Glib::ustring> Album;
            Gtk::TreeModelColumn<guint64>       Track;
            Gtk::TreeModelColumn<Glib::ustring> Name;
            Gtk::TreeModelColumn<guint64>       Length;

            // These hidden columns are used for sorting
            // They don't contain sortnames, as one might 
            // think from their name, but instead the MB
            // IDs (if not available, then just the plain name)
            // They are used only for COMPARISON FOR EQUALITY.
            // Obviously, comparing them with compare() is
            // useless if they're MB IDs

            Gtk::TreeModelColumn<Glib::ustring> ArtistSort;
            Gtk::TreeModelColumn<Glib::ustring> AlbumSort;
            Gtk::TreeModelColumn<gint64>        RowId;
            Gtk::TreeModelColumn<std::string>   Location;
            Gtk::TreeModelColumn< ::MPX::Track> MPXTrack;
            Gtk::TreeModelColumn<gint64>        Rating;
            Gtk::TreeModelColumn<bool>          IsMPXTrack;
            Gtk::TreeModelColumn<bool>          IsBad;

            AllTracksColumnsT ()
            {
                add (Artist);
                add (Album);
                add (Track);
                add (Name);
                add (Length);
                add (ArtistSort);
                add (AlbumSort);
                add (RowId);
                add (Location);
                add (MPXTrack);
                add (Rating);
                add (IsMPXTrack);
                add (IsBad);
            };
        };

        class AllTracksTreeView
        {
              MPX::Source::PlaybackSourceMusicLib & m_MLib;
              PAccess<MPX::Library>                 m_Lib;
              PAccess<MPX::HAL>                     m_HAL;
              ListView                            * m_ListView;
              DataModelFilterP                    * m_FilterModel;

            public:

              AllTracksTreeView(
                    Glib::RefPtr<Gnome::Glade::Xml> const& xml,
                    PAccess<MPX::Library>           const& lib,
                    PAccess<MPX::HAL>               const& hal,
                    MPX::Source::PlaybackSourceMusicLib  & mlib
              )
              : m_MLib(mlib)
              , m_Lib(lib)
              , m_HAL(hal)
              {
                    Gtk::ScrolledWindow     * scrollwin = dynamic_cast<Gtk::ScrolledWindow*>(xml->get_widget("musiclib-alltracks-sw")); 
                    Gtk::Entry              * entry     = dynamic_cast<Gtk::Entry*>(xml->get_widget("musiclib-alltracks-filter-entry")); 
                    ListView                * view      = new ListView;
                   
                    ModelP model = ModelP( new ModelT );

                    SQL::RowV v;
                    m_Lib.get().getSQL(v, (boost::format("SELECT id, artist, album, title FROM track_view ORDER BY album_artist, album, track_view.track")).str()); 
                    for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                    {
                            SQL::Row & r = *i;
                            std::string artist = boost::get<std::string>(r["artist"]);
                            std::string album  = boost::get<std::string>(r["album"]);
                            std::string title  = boost::get<std::string>(r["title"]);
                            gint64         id  = boost::get<gint64>(r["id"]);

                            Row4 row (artist, album, title, id); 
                            model->push_back(row);
                    }

                    DataModelP m (new DataModel(model));
                    DataModelFilterP f (new DataModelFilter(m));

                    entry->signal_changed().connect(
                        sigc::bind(
                            sigc::mem_fun(
                                *this,
                                &AllTracksTreeView::on_entry_changed
                            ),
                            f,
                            entry
                    ));

                    ColumnP c1 (new Column);
                    c1->set_column(0);

                    ColumnP c2 (new Column);
                    c2->set_column(1);

                    ColumnP c3 (new Column);
                    c3->set_column(2);

                    view->append_column(c1);
                    view->append_column(c2);
                    view->append_column(c3);

                    view->set_model(f);

                    scrollwin->add(*view);
                    view->show();
                    scrollwin->show_all();

                    view->signal_track_activated().connect(
                        sigc::mem_fun(
                            *this,
                            &AllTracksTreeView::on_track_activated
                    ));
              }

              void
              on_track_activated (gint64 id)
              {
                IdV v (1, id);
                m_MLib.play_tracks(v);
              }

              void
              on_entry_changed (DataModelFilterP model, Gtk::Entry* entry)
              {
                    model->set_filter(entry->get_text());
              }
        };

#if 0 
        class AllTracksTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              MPX::Source::PlaybackSourceMusicLib & m_MLib;
              PAccess<MPX::Library>                 m_Lib;
              PAccess<MPX::HAL>                     m_HAL;

              Glib::RefPtr<Gdk::Pixbuf>             m_Playing;
              Glib::RefPtr<Gdk::Pixbuf>             m_Bad;

              Glib::RefPtr<Gtk::UIManager>          m_UIManager;
              Glib::RefPtr<Gtk::ActionGroup>        m_ActionGroup;      

            public:

              Glib::RefPtr<Gdk::Pixbuf>             m_Stars[N_STARS];

              AllTracksColumnsT                     AllTracksColumns;
              Glib::RefPtr<Gtk::ListStore>          ListStore;
              Glib::RefPtr<Gtk::TreeModelFilter>    ListStoreFilter;

              Gtk::Entry                          * m_FilterEntry;

              enum Column
              {
                C_TITLE,
                C_ARTIST,
                C_LENGTH,
                C_ALBUM,
                C_TRACK,
                C_RATING,
              };

              static const int N_FIRST_CUSTOM = 6;

              AllTracksTreeView(
                    Glib::RefPtr<Gnome::Glade::Xml> const& xml,
                    PAccess<MPX::Library>           const& lib,
                    PAccess<MPX::HAL>               const& hal,
                    MPX::Source::PlaybackSourceMusicLib  & mlib
              )
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-alltracks")
              , m_MLib(mlib)
              , m_Lib(lib)
              , m_HAL(hal)
              {
                set_has_tooltip();
                set_rules_hint();

                for(int n = 0; n < 6; ++n)
                {
                    m_Stars[n] = Gdk::Pixbuf::create_from_file(
                        build_filename(
                            build_filename(
                                DATA_DIR,
                                "images"
                            ),
                            (boost::format("stars%d.png") % n).str()
                    ));
                }

                TreeViewColumn * col = 0; 

                append_column(_("Name"), AllTracksColumns.Name);
                append_column(_("Artist"), AllTracksColumns.Artist);

                col = manage (new TreeViewColumn(_("Time")));
                CellRendererText * cell2 = manage (new CellRendererText);
                col->property_alignment() = 1.;
                col->pack_start(*cell2, true);
                col->set_cell_data_func(*cell2, sigc::mem_fun( *this, &AllTracksTreeView::cellDataFuncTime ));
                col->set_sort_column_id(AllTracksColumns.Length);
                g_object_set(G_OBJECT(cell2->gobj()), "xalign", 1.0f, NULL);
                append_column(*col);

                append_column(_("Album"), AllTracksColumns.Album);
                append_column(_("Track"), AllTracksColumns.Track);

                col = manage (new TreeViewColumn(_("My Rating")));
                CellRendererPixbuf *cell = manage (new CellRendererPixbuf);
                col->pack_start(*cell, false);
                col->set_min_width(66);
                col->set_max_width(66);
                col->set_cell_data_func(*cell, sigc::mem_fun( *this, &AllTracksTreeView::cellDataFuncRating ));
                append_column(*col);

                //////////////////////////////// 

#if 0
                cell2 = manage (new CellRendererText);
                for( int i = 0; i < N_ATTRIBUTES_INT; ++i)
                {
                        col = manage (new TreeViewColumn(_(attribute_names[i])));
                        col->pack_start(*cell2, true);
                        col->set_cell_data_func(*cell2, sigc::bind(sigc::mem_fun( *this, &AllTracksTreeView::cellDataFuncCustom ), i ));
                        col->property_visible()= false;
                        append_column(*col);
                }
#endif
    
                ////////////////////////////////

                get_column(C_TITLE)->set_sort_column_id(AllTracksColumns.Name);
                get_column(C_ARTIST)->set_sort_column_id(AllTracksColumns.Artist);
                get_column(C_LENGTH)->set_sort_column_id(AllTracksColumns.Length);
                get_column(C_ALBUM)->set_sort_column_id(AllTracksColumns.Album);
                get_column(C_TRACK)->set_sort_column_id(AllTracksColumns.Track);
                get_column(C_RATING)->set_sort_column_id(AllTracksColumns.Rating);

                get_column(0)->set_resizable(true);
                get_column(1)->set_resizable(true);
                get_column(2)->set_resizable(false);
                get_column(3)->set_resizable(true);
                get_column(4)->set_resizable(false);
                get_column(5)->set_resizable(false);

                ListStore = Gtk::ListStore::create(AllTracksColumns);
                ListStoreFilter = Gtk::TreeModelFilter::create(ListStore);

                ListStoreFilter->set_visible_func(
                    sigc::mem_fun(
                        *this,
                        &AllTracksTreeView::track_visible_func
                ));

                ListStore->set_sort_func(
                    AllTracksColumns.Artist,
                    sigc::mem_fun(
                        *this,
                        &AllTracksTreeView::slotSortByArtist
                ));

                ListStore->set_sort_func(
                    AllTracksColumns.Album,
                    sigc::mem_fun(
                        *this,
                        &AllTracksTreeView::slotSortByAlbum
                ));

                ListStore->set_sort_func(
                    AllTracksColumns.Track,
                    sigc::mem_fun(
                        *this,
                        &AllTracksTreeView::slotSortByTrack
                ));

                get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

                set_headers_clickable();

                m_UIManager = Gtk::UIManager::create();
                m_ActionGroup = Gtk::ActionGroup::create ("Actions_UiPartPlaylist-AllTracksList");
                m_ActionGroup->add  (Gtk::Action::create("dummy","dummy"));
                m_UIManager->insert_action_group (m_ActionGroup);

                xml->get_widget("alltracks-filter-entry", m_FilterEntry);

                m_FilterEntry->signal_changed().connect(
                    sigc::mem_fun(
                        *this,
                        &AllTracksTreeView::on_filter_entry_changed
                ));

                m_Lib.get().signal_new_track().connect( 
                    sigc::mem_fun(
                        *this,
                        &AllTracksTreeView::on_new_track
                ));

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-track", TARGET_SAME_APP, 0x81));
                drag_source_set(Entries); 

                append_tracks();
                set_model(ListStoreFilter);
              }

        protected:

              virtual void
              on_row_activated (const TreeModel::Path& path, TreeViewColumn* column)
              {
                TreeIter iter = ListStore->get_iter( ListStoreFilter->convert_path_to_child_path(path) );
                gint64 id = (*iter)[AllTracksColumns.RowId];
                IdV v (1, id);
                m_MLib.play_tracks(v);
              }

              virtual void
              on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, SelectionData& selection_data, guint info, guint time)
              {
                std::vector<Gtk::TreePath> rows = get_selection()->get_selected_rows();
                TreeIter iter = ListStore->get_iter( ListStoreFilter->convert_path_to_child_path(rows[0]));
                gint64 * id = new gint64((*iter)[AllTracksColumns.RowId]);
                selection_data.set("mpx-track", 8, reinterpret_cast<const guint8*>(id), 8);
              }

        private:

              void
              on_filter_entry_changed ()
              {
                ListStoreFilter->refilter();
              }

              void
              on_new_track(Track & track, gint64 album_id, gint64 artist_id)
              {
                TreeIter iter = ListStore->append();
                place_track( track, iter );
              }

              bool
              track_visible_func (TreeIter const& iter)
              {
                  std::string filter = m_FilterEntry->get_text().lowercase();
                  TreePath path = ListStore->get_path(iter);

                  if( filter.empty() ) return true;

                  typedef std::vector<std::string> split_vector_type; 

                  split_vector_type SplitVec;
                  boost::algorithm::split( SplitVec, filter, boost::algorithm::is_any_of(" "));
    
                  split_vector_type MatchVec;
                  MatchVec.push_back(ustring((*iter)[AllTracksColumns.Artist]).lowercase());
                  MatchVec.push_back(ustring((*iter)[AllTracksColumns.Album]).lowercase());
                  MatchVec.push_back(ustring((*iter)[AllTracksColumns.Name]).lowercase());

                  for(split_vector_type::const_iterator i = SplitVec.begin(); i != SplitVec.end(); ++i)
                  {
                    bool found_fragment = false;
                    for(split_vector_type::const_iterator x = MatchVec.begin(); x != MatchVec.end(); ++x)
                    {
                        if((*i).empty())
                            continue;

                        if(boost::algorithm::find_first(*x, *i))
                        {
                            found_fragment = true; 
                        }
                    }
                    if(! found_fragment )
                    {
                        return false;
                    }
                  }

                  return true;
             }
    
 
              void
              place_track(SQL::Row & r, Gtk::TreeIter const& iter)
              {
                  if(r.count("id"))
                      (*iter)[AllTracksColumns.RowId] = get<gint64>(r["id"]); 
                  else
                      g_critical("%s: No id for track, extremeley suspicious", G_STRLOC);

                  if(r.count("artist"))
                      (*iter)[AllTracksColumns.Artist] = get<std::string>(r["artist"]); 
                  if(r.count("album"))
                      (*iter)[AllTracksColumns.Album] = get<std::string>(r["album"]); 
                  if(r.count("track"))
                      (*iter)[AllTracksColumns.Track] = guint64(get<gint64>(r["track"]));
                  if(r.count("title"))
                      (*iter)[AllTracksColumns.Name] = get<std::string>(r["title"]);
                  if(r.count("time"))
                      (*iter)[AllTracksColumns.Length] = guint64(get<gint64>(r["time"]));
                  if(r.count("mb_artist_id"))
                      (*iter)[AllTracksColumns.ArtistSort] = get<std::string>(r["mb_artist_id"]);
                  if(r.count("mb_album_id"))
                      (*iter)[AllTracksColumns.AlbumSort] = get<std::string>(r["mb_album_id"]);
                  if(r.count("rating"))
                      (*iter)[AllTracksColumns.Rating] = get<gint64>(r["rating"]);

                  (*iter)[AllTracksColumns.Location] = get<std::string>(r["location"]); 
                  (*iter)[AllTracksColumns.MPXTrack] = m_Lib.get().sqlToTrack(r); 
                  (*iter)[AllTracksColumns.IsMPXTrack] = true; 
                  (*iter)[AllTracksColumns.IsBad] = false; 
              }

              void
              place_track(MPX::Track & track, Gtk::TreeIter const& iter)
              {
                  if(track[ATTRIBUTE_MPX_TRACK_ID])
                      (*iter)[AllTracksColumns.RowId] = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get()); 
                  else
                      g_critical("Warning, no id given for track; this is totally wrong and should never happen.");


                  if(track[ATTRIBUTE_ARTIST])
                      (*iter)[AllTracksColumns.Artist] = get<std::string>(track[ATTRIBUTE_ARTIST].get()); 

                  if(track[ATTRIBUTE_ALBUM])
                      (*iter)[AllTracksColumns.Album] = get<std::string>(track[ATTRIBUTE_ALBUM].get()); 

                  if(track[ATTRIBUTE_TRACK])
                      (*iter)[AllTracksColumns.Track] = guint64(get<gint64>(track[ATTRIBUTE_TRACK].get()));

                  if(track[ATTRIBUTE_TITLE])
                      (*iter)[AllTracksColumns.Name] = get<std::string>(track[ATTRIBUTE_TITLE].get()); 

                  if(track[ATTRIBUTE_TIME])
                      (*iter)[AllTracksColumns.Length] = guint64(get<gint64>(track[ATTRIBUTE_TIME].get()));

                  if(track[ATTRIBUTE_MB_ARTIST_ID])
                      (*iter)[AllTracksColumns.ArtistSort] = get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get());

                  if(track[ATTRIBUTE_MB_ALBUM_ID])
                      (*iter)[AllTracksColumns.AlbumSort] = get<std::string>(track[ATTRIBUTE_MB_ALBUM_ID].get());

                  if(track[ATTRIBUTE_RATING])
                      (*iter)[AllTracksColumns.Rating] = get<gint64>(track[ATTRIBUTE_RATING].get());

                  if(track[ATTRIBUTE_LOCATION])
                      (*iter)[AllTracksColumns.Location] = get<std::string>(track[ATTRIBUTE_LOCATION].get());
                  else
                      g_critical("Warning, no location given for track; this is totally wrong and should never happen.");

                  (*iter)[AllTracksColumns.MPXTrack] = track; 
                  (*iter)[AllTracksColumns.IsMPXTrack] = track[ATTRIBUTE_MPX_TRACK_ID] ? true : false; 
                  (*iter)[AllTracksColumns.IsBad] = false; 
              }

              void          
              append_tracks ()
              {
                  SQL::RowV v;
                  m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view ORDER BY album_artist, album, track_view.track")).str()); 
                  TreeIter iter = ListStore->append();

                  for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                  {
                          SQL::Row & r = *i;
                          
                          if(i != v.begin())
                              iter = ListStore->insert_after(iter);

                          place_track(r, iter);
                  }
              } 

              virtual bool
              on_button_press_event (GdkEventButton* event)
              {
                  int cell_x, cell_y ;
                  TreeViewColumn *col ;
                  TreePath path;

                  if(get_path_at_pos (event->x, event->y, path, col, cell_x, cell_y))
                  {
                      if(col == get_column(5))
                      {
                          int rating = (cell_x + 7) / 12;
                          g_return_val_if_fail(((rating >= 0) && (rating <= 5)), false);
                          TreeIter iter = ListStore->get_iter(path);
                          (*iter)[AllTracksColumns.Rating] = rating;   
                          m_Lib.get().trackRated(gint64((*iter)[AllTracksColumns.RowId]), rating);
                      }
                  }
                  TreeView::on_button_press_event(event);
                  return false;
              }

              virtual bool
              on_event (GdkEvent * ev)
              {
                  if( ev->type == GDK_BUTTON_PRESS )
                  {
                    GdkEventButton * event = reinterpret_cast <GdkEventButton *> (ev);
                    if( event->button == 3 )
                    {
#if 0
                      Gtk::Menu * menu = dynamic_cast < Gtk::Menu* > (Util::get_popup (m_UIManager, "/popup-playlist-list/menu-playlist-list"));
                      if (menu) // better safe than screwed
                      {
                        menu->popup (event->button, event->time);
                      }
                      return true;
#endif
                    }
                  }
                  return false;
              }

              void
              cellDataFuncTime (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                  CellRendererText *cell_t = dynamic_cast<CellRendererText*>(basecell);
                  guint64 Length = (*iter)[AllTracksColumns.Length]; 
                  g_object_set(G_OBJECT(cell_t->gobj()), "xalign", 1.0f, NULL);
                  cell_t->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
              }

              void
              cellDataFuncRating (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
                  CellRendererPixbuf *cell_p = dynamic_cast<CellRendererPixbuf*>(basecell);
                  if(!(*iter)[AllTracksColumns.IsMPXTrack])
                  {
                      cell_p->property_sensitive() = false; 
                      cell_p->property_pixbuf() = Glib::RefPtr<Gdk::Pixbuf>(0);
                  }
                  else
                  {
                      cell_p->property_sensitive() = true; 
                      gint64 i = ((*iter)[AllTracksColumns.Rating]);
                      g_return_if_fail((i >= 0) && (i <= 5));
                      cell_p->property_pixbuf() = m_Stars[i];
                  }
              }

              void
              cellDataFuncCustom (CellRenderer * basecell, TreeModel::iterator const &iter, int attribute)
              {
                  CellRendererText *cell_t = dynamic_cast<CellRendererText*>(basecell);
                  MPX::Track track = (*iter)[AllTracksColumns.MPXTrack]; 

                  if(track.has(attribute))
                  {
                    cell_t->property_text() = ovariant_get_string(track[attribute]);
                  }
                  else
                  {
                    cell_t->property_text() = ""; 
                  }
              }

              int
              slotSortDefault(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  return 0;
              }

              int
              slotSortById(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  guint64 id_a = (*iter_a)[AllTracksColumns.RowId];
                  guint64 id_b = (*iter_b)[AllTracksColumns.RowId];
      
                  return (id_a - id_b); // FIXME: int overflow
              }

              int
              slotSortByTrack(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  ustring alb_a = (*iter_a)[AllTracksColumns.AlbumSort];
                  ustring alb_b = (*iter_b)[AllTracksColumns.AlbumSort];
                  guint64 trk_a = (*iter_a)[AllTracksColumns.Track];
                  guint64 trk_b = (*iter_b)[AllTracksColumns.Track];

                  if(alb_a != alb_b)
                      return 0;

                  return (trk_a - trk_b); // FIXME: int overflow
              }

              int
              slotSortByAlbum(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  ustring arts_a = (*iter_a)[AllTracksColumns.ArtistSort];
                  ustring arts_b = (*iter_b)[AllTracksColumns.ArtistSort];
                  ustring alb_a = (*iter_a)[AllTracksColumns.Album];
                  ustring alb_b = (*iter_b)[AllTracksColumns.Album];

                  return alb_a.compare(alb_b); 
              }

              int
              slotSortByArtist(const TreeIter& iter_a, const TreeIter& iter_b)
              {
                  ustring art_a = (*iter_a)[AllTracksColumns.Artist];
                  ustring art_b = (*iter_b)[AllTracksColumns.Artist];

                  return art_a.compare(art_b); 
              }
        };
#endif

        PlaylistTreeView        *   m_TreeViewPlaylist;
        AlbumTreeView           *   m_TreeViewAlbums;
        LFMTreeView             *   m_TreeViewLFM;
        AllTracksTreeView       *   m_TreeViewAllTracks;

        PAccess<MPX::Library>       m_Lib;
        PAccess<MPX::Covers>        m_Covers;
        PAccess<MPX::HAL>           m_HAL;

        MusicLibPrivate (MPX::Player & player, MPX::Source::PlaybackSourceMusicLib & mlib, Glib::RefPtr<Gnome::Glade::Xml> xml)
        {
            m_RefXml = xml;
            m_UI = m_RefXml->get_widget("source-musiclib");
            player.get_object(m_Lib);
            player.get_object(m_Covers);
            player.get_object(m_HAL);
            m_TreeViewPlaylist = new PlaylistTreeView(m_RefXml, m_Lib, m_HAL, mlib);
            m_TreeViewAlbums = new AlbumTreeView(m_RefXml, m_Lib, m_Covers, mlib);
            m_TreeViewLFM = new LFMTreeView(m_RefXml, m_Lib, m_Covers, mlib);
            m_TreeViewAllTracks = new AllTracksTreeView(m_RefXml, m_Lib, m_HAL, mlib);
        }
    };
}

namespace MPX
{
namespace Source
{
    bool PlaybackSourceMusicLib::m_signals_installed = false;

    PlaybackSourceMusicLib::PlaybackSourceMusicLib (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player & player)
    : PlaybackSource(ui_manager, _("Music Library"), C_CAN_SEEK)
    , m_MainUIManager(ui_manager)
    , m_Skipped(false)
    {
        add_cap(C_CAN_SEEK);

        mpx_musiclib_py_init();

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
                            g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

            m_signals_installed = true;
        }

        player.get_object(m_Lib);
        player.get_object(m_HAL);
        player.get_object(m_Covers);

        const std::string path (build_filename(DATA_DIR, build_filename("glade","source-musiclib.glade")));
        m_RefXml = Gnome::Glade::Xml::create (path);

        m_Private = new MusicLibPrivate(player,*this,m_RefXml);
        m_Private->m_TreeViewPlaylist->signal_row_activated().connect( sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_plist_row_activated ) );
        m_Private->m_TreeViewPlaylist->signal_query_tooltip().connect( sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_plist_query_tooltip ) );

        mcs->domain_register("PlaybackSourceMusicLib");
        mcs->key_register("PlaybackSourceMusicLib", "divider-position", 250);

        (dynamic_cast<Gtk::HPaned*>(m_Private->m_UI))->set_position(mcs->key_get<int>("PlaybackSourceMusicLib", "divider-position"));


        m_MainActionGroup = ActionGroup::create("ActionsMusicLib");
        m_MainActionGroup->add(Action::create("menu-source-musiclib", _("Music _Library")));

        Gtk::RadioButtonGroup gr1;
        m_MainActionGroup->add (RadioAction::create( gr1, "musiclib-sort-by-name", _("Sort Albums by Album/Date/Artist")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_sort_column_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-name"))->property_value() = 0;

        m_MainActionGroup->add (RadioAction::create( gr1, "musiclib-sort-by-date", _("Sort Albums by Time Added")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_sort_column_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-date"))->property_value() = 1;

        m_MainActionGroup->add (RadioAction::create( gr1, "musiclib-sort-by-rating", _("Sort Albums by Rating")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_sort_column_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-rating"))->property_value() = 2;

        m_MainActionGroup->add (RadioAction::create( gr1, "musiclib-sort-by-alphabet", _("Sort Albums Alphabetically")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_sort_column_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-sort-by-alphabet"))->property_value() = 3;

        m_MainActionGroup->add (ToggleAction::create( "musiclib-show-only-new", _("Show only New Albums")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_show_new_albums ));



        Gtk::RadioButtonGroup gr2;
        m_MainActionGroup->add (RadioAction::create( gr2, "musiclib-show-albums", _("All Albums")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_view_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-show-albums"))->property_value() = 0;

        m_MainActionGroup->add (RadioAction::create( gr2, "musiclib-show-alltracks", _("All Tracks")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_view_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-show-alltracks"))->property_value() = 1;

        m_MainActionGroup->add (RadioAction::create( gr2, "musiclib-show-collections", _("Last.fm Albums-by-Tag View")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::on_view_change ));
        RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action("musiclib-show-collections"))->property_value() = 2;



        m_MainActionGroup->add (Action::create( "musiclib-show-ccdialog", _("Configure columns...")),
                                                sigc::mem_fun( *m_Private->m_TreeViewPlaylist, &MusicLibPrivate::PlaylistTreeView::action_cb_show_ccdialog ));

        m_MainActionGroup->add (Action::create( "musiclib-action-recache-covers", _("Refresh album covers")),
                                                sigc::mem_fun( *this, &PlaybackSourceMusicLib::action_cb_refresh_covers ));

        m_MainUIManager->insert_action_group(m_MainActionGroup);
    }

    PlaybackSourceMusicLib::~PlaybackSourceMusicLib ()
    {
        delete m_Private;
    }

    void
    PlaybackSourceMusicLib::action_cb_play ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.m_CurrentIter = boost::optional<Gtk::TreeIter>(); 
        playlist.m_CurrentId = boost::optional<gint64>();
        Signals.PlayRequest.emit();
    }

    void
    PlaybackSourceMusicLib::action_cb_go_to_album(gint64 id)
    {
        m_Private->m_TreeViewAlbums->go_to_album(id);
    }

    void
    PlaybackSourceMusicLib::action_cb_refresh_covers ()
    {
        TimedConfirmation dialog (_("Please confirm Cover Refresh"), 4);
        int response = dialog.run(_("Are you sure you want to Refresh <b>all</b> covers at this time? (previous covers will be irrevocably lost)"));
        if( response == Gtk::RESPONSE_OK )
        {
            m_Lib.get().recacheCovers();
        }
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
        return "36068e19-dfb3-49cd-85b4-52cea16fe0fd";
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
    PlaybackSourceMusicLib::on_plist_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.m_CurrentIter = playlist.ListStore->get_iter( path );
        playlist.m_CurrentId = (*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.RowId];

        Signals.PlayRequest.emit();
    }

    void
    PlaybackSourceMusicLib::on_sort_column_change ()
    {
        int value = RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action ("musiclib-sort-by-name"))->get_current_value();
        m_Private->m_TreeViewAlbums->TreeStore->set_sort_column(value, Gtk::SORT_ASCENDING);    
    }

    void
    PlaybackSourceMusicLib::on_show_new_albums ()
    {
        int value = RefPtr<Gtk::ToggleAction>::cast_static (m_MainActionGroup->get_action ("musiclib-show-only-new"))->get_active();
        m_Private->m_TreeViewAlbums->set_new_albums_state(value);
    }

    void
    PlaybackSourceMusicLib::on_view_change ()
    {
        int value = RefPtr<Gtk::RadioAction>::cast_static (m_MainActionGroup->get_action ("musiclib-show-albums"))->get_current_value();
        dynamic_cast<Gtk::Notebook*>(m_RefXml->get_widget("musiclib-notebook-views"))->set_current_page(value);
    }

    bool
    PlaybackSourceMusicLib::on_plist_query_tooltip(int x,int y, bool kbd, const Glib::RefPtr<Gtk::Tooltip> & tooltip)
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        GtkWidget * tooltip_widget = 0;
        int cell_x, cell_y;
        TreePath path;
        TreeViewColumn *col;
        int x2, y2;
        playlist.convert_widget_to_bin_window_coords(x, y, x2, y2);
        if(playlist.get_path_at_pos (x2, y2, path, col, cell_x, cell_y))
        {
            TreeIter iter = playlist.ListStore->get_iter(path);
            GtkTreeIter const* iter_c = iter->gobj();
            g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_TOOLTIP], 0, G_OBJECT(playlist.ListStore->gobj()), iter_c, &tooltip_widget);
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

    void
    PlaybackSourceMusicLib::plist_end(bool cleared)
    {
        g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_END], 0, gboolean(cleared));
    }

    void
    PlaybackSourceMusicLib::plist_sensitive(bool sensitive)
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.set_sensitive(sensitive);
    }

    Glib::RefPtr<Gtk::ListStore>
    PlaybackSourceMusicLib::get_playlist_model ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        return playlist.ListStore;
    }

    PyObject*
    PlaybackSourceMusicLib::get_playlist_current_iter ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        if(!playlist.m_CurrentIter)
        {
            Py_INCREF(Py_None);
            return Py_None;
        }

        GtkTreeIter  * iter = playlist.m_CurrentIter.get().gobj();
        return pyg_boxed_new(GTK_TYPE_TREE_ITER, iter, TRUE, FALSE);
    }

    void
    PlaybackSourceMusicLib::play_album(gint64 id)
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.clear();
        playlist.append_album(id);
        Signals.PlayRequest.emit();
    }

    void
    PlaybackSourceMusicLib::append_album(gint64 id)
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.append_album(id);
    }

    void
    PlaybackSourceMusicLib::play_tracks(IdV const& idv)
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.clear();
        playlist.append_tracks(idv, NO_ORDER);
        check_nextprev_caps ();
        send_caps();
        Signals.PlayRequest.emit();
    }

    void
    PlaybackSourceMusicLib::append_tracks(IdV const& idv)
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.append_tracks(idv, NO_ORDER);
        check_nextprev_caps ();
        send_caps();
    }

    std::string
    PlaybackSourceMusicLib::get_uri () 
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        g_return_val_if_fail(playlist.m_CurrentIter, std::string());
        
#ifdef HAVE_HAL
        try{
            MPX::Track t ((*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.MPXTrack]);
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
        return Glib::ustring((*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.Location]);
#endif // HAVE_HAL
    }

    std::string
    PlaybackSourceMusicLib::get_type ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        g_return_val_if_fail(playlist.m_CurrentIter, std::string());
        MPX::Track t ((*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.MPXTrack]);
        if(t[ATTRIBUTE_TYPE])
            return get<std::string>(t[ATTRIBUTE_TYPE].get());
        else
            return std::string();
    }

    bool
    PlaybackSourceMusicLib::play ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        std::vector<Gtk::TreePath> rows = playlist.get_selection()->get_selected_rows();

        bool exists = false;

        if(playlist.m_PlayInitIter)
        {
            playlist.m_CurrentIter = playlist.m_PlayInitIter.get();
            playlist.m_PlayInitIter.reset ();
        }

        if /* still */ (!playlist.m_CurrentIter)
        {
            if( !rows.empty() )
            {
                    playlist.m_CurrentIter = playlist.ListStore->get_iter (rows[0]); 
                    playlist.m_CurrentId = (*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.RowId];
                    playlist.queue_draw();
            }
            else
            {
                    if( playlist.ListStore->children().size() )
                    {
                        TreePath path (TreePath::size_type(1), TreePath::value_type(0));
                        playlist.m_CurrentIter = playlist.ListStore->get_iter(path);
                    }
            }
        }

        if /* finally */ ( playlist.m_CurrentIter )
        {
            exists = playlist.check_current_exists();
        }

        if( !exists )
        {
            playlist.m_CurrentIter.reset();
            playlist.m_PlayInitIter.reset();
            playlist.queue_draw();
        }

        check_nextprev_caps ();
        
        return exists;
    }

    bool
    PlaybackSourceMusicLib::go_next ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        TreeIter &iter = playlist.m_CurrentIter.get();
        TreePath path1 = playlist.ListStore->get_path(iter);
        TreePath path2 = playlist.ListStore->get_path(--playlist.ListStore->children().end());

        if (path1[0] < path2[0]) 
        {
            iter++;
            return true;
        }
    
        return false;
    }

    bool
    PlaybackSourceMusicLib::go_prev ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        g_return_val_if_fail(bool(playlist.m_CurrentIter), false);

        TreeIter & iter = playlist.m_CurrentIter.get();

        TreePath path1 = playlist.ListStore->get_path(iter);
        TreePath path2 = playlist.ListStore->get_path(playlist.ListStore->children().begin());

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
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.m_CurrentIter = boost::optional<Gtk::TreeIter>();
        playlist.m_CurrentId = boost::optional<gint64>(); 
        playlist.queue_draw();
        check_nextprev_caps ();
    }

    void
    PlaybackSourceMusicLib::set_play ()
    {
        add_cap(C_CAN_PLAY);
        send_caps ();
    }

    void
    PlaybackSourceMusicLib::clear_play ()
    {
        rem_cap(C_CAN_PLAY);
        send_caps ();
    }

    void
    PlaybackSourceMusicLib::check_nextprev_caps ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        if (!playlist.m_CurrentIter)
        {
            rem_cap(C_CAN_GO_NEXT);
            rem_cap(C_CAN_GO_PREV);
            return;
        }

        TreeIter & iter = playlist.m_CurrentIter.get();

        TreePath path1 = playlist.ListStore->get_path(iter);
        TreePath path2 = playlist.ListStore->get_path(playlist.ListStore->children().begin());
        TreePath path3 = playlist.ListStore->get_path(--playlist.ListStore->children().end());

        if(path1[0] > path2[0])
            add_cap(C_CAN_GO_PREV);
        else
            rem_cap(C_CAN_GO_PREV);

        if(path1[0] < path3[0])
            add_cap(C_CAN_GO_NEXT);
        else
            rem_cap(C_CAN_GO_NEXT);
    }

    void
    PlaybackSourceMusicLib::send_metadata ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        g_return_if_fail(bool(playlist.m_CurrentIter));

        MPX::Metadata m = (MPX::Track((*playlist.m_CurrentIter.get())[playlist.PlaylistColumns.MPXTrack]));
        Signals.Metadata.emit( m );
    }

    void
    PlaybackSourceMusicLib::play_post () 
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        add_cap(C_CAN_PAUSE);
        add_cap(C_CAN_PROVIDE_METADATA);
        check_nextprev_caps ();

        m_Private->m_TreeViewPlaylist->scroll_to_track();

        TreeIter &iter = playlist.m_CurrentIter.get();
        TreePath path1 = playlist.ListStore->get_path(iter);
        TreePath path2 = playlist.ListStore->get_path(--playlist.ListStore->children().end());

        if(path1[0] == path2[0])
        {
            plist_end(false);
        }
    }

    void
    PlaybackSourceMusicLib::next_post () 
    { 
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.queue_draw();
        check_nextprev_caps ();
        playlist.scroll_to_track();

        TreeIter &iter = playlist.m_CurrentIter.get();
        TreePath path1 = playlist.ListStore->get_path(iter);
        TreePath path2 = playlist.ListStore->get_path(--playlist.ListStore->children().end());

        if (path1[0] < path2[0]) 
        {
            if((path1[0]+1) == path2[0])
            {
                g_signal_emit(G_OBJECT(gobj()), signals[PSM_SIGNAL_PLAYLIST_END], 0);
            }
        }

        if( !m_Skipped )
        {
                // Update markov
                gint64 track_id_b = (*iter)[playlist.PlaylistColumns.RowId];
                TreeIter iter2 = iter;
                --iter2;
                if(iter2)
                {
                    gint64 track_id_a = (*iter2)[playlist.PlaylistColumns.RowId];
                    m_Lib.get().markovUpdate( track_id_a, track_id_b );
                }
        }
        else
        {
                m_Skipped = false;
        }
    }

    void
    PlaybackSourceMusicLib::prev_post ()
    {
        MusicLibPrivate::PlaylistTreeView & playlist (*m_Private->m_TreeViewPlaylist);

        playlist.queue_draw();
        check_nextprev_caps ();
        playlist.scroll_to_track();
    }

    void
    PlaybackSourceMusicLib::restore_context ()
    {}

    void
    PlaybackSourceMusicLib::skipped () 
    {
        m_Skipped = true;
    }

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
    PlaybackSourceMusicLib::get_uri_schemes ()
    {
        UriSchemes s;
        s.push_back("file");
        return s;
    }

    void    
    PlaybackSourceMusicLib::process_uri_list (Util::FileList const& uris, bool play)
    {
        m_Private->m_TreeViewPlaylist->prepare_uris (uris, play);
        if(play)
        {
            Signals.PlayRequest.emit();
        }
    }

} // end namespace Source
} // end namespace MPX 
