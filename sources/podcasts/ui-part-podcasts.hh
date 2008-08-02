//  BMP
//  Copyright (C) 2005-2007 BMP development.
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
//  The BMPx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and BMPx. This
//  permission is above and beyond the permissions granted by the GPL license
//  BMPx is covered by.

#ifndef BMP_UI_PART_PODCASTS_HH
#define BMP_UI_PART_PODCASTS_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif//HAVE_CONFIG_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <ctime>
#include <map>

#include <glib/gtypes.h>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treeview.h>
#include <gtkmm/uimanager.h>
#include <libglademm/xml.h>
#include <gdk/gdkx.h>

#include "widgets/ccwidgets.hh"
#include "widgets/cell-renderer-cairo-surface.hh"

#include "bmp/types/types-basic.hh"
#include "bmp/types/types-ui.hh"

#include "podcast.hh"
#include "podcast-utils.hh"
#include "podcast-v2-mpx-types.hh"

#include "playbacksource.hh"
#include "ui-part-base.hh"

#include "mpx-minisoup.hh"

namespace Bmp
{
  typedef boost::optional<TreeIter> OptionalTreeIter;
  
  enum CastStatus
  {
    ST_DEFAULT,
    ST_PENDING
  };

  class Casts
    : public Gtk::TreeModel::ColumnRecord
  {
    public:

      Gtk::TreeModelColumn<CastStatus>      status;
      Gtk::TreeModelColumn<RefSurface>      image;
      Gtk::TreeModelColumn<ustring>         title;
      Gtk::TreeModelColumn<ustring>         uri;
      Gtk::TreeModelColumn<UID>             uid;
      Gtk::TreeModelColumn<std::string>     key;
      Gtk::TreeModelColumn<time_t>          most_recent;
      Gtk::TreeModelColumn<ustring>         searchtitle;
      Gtk::TreeModelColumn<ustring>         description;

      Casts ()
      {
        add (status);
        add (image);
        add (title);
        add (uri);
        add (uid);
        add (key);
        add (most_recent);
        add (searchtitle);
        add (description);
      }
  };

  class Episodes
    : public Gtk::TreeModel::ColumnRecord
  {
    public:
      Gtk::TreeModelColumn<Episode> item;
      Episodes ()
      {
        add (item);
      }
  };

  enum EpisodeColumnType
  {
    CELL_TEXT,
    CELL_TOGGLE,
    CELL_PROGRESS,
    CELL_SURFACE,
  };

  enum EpisodeColumn
  {
    COLUMN_PLAYING      =   0,
    COLUMN_DOWNLOADED   =   1,
    COLUMN_DOWNLOADING  =   2,
    COLUMN_TITLE        =   3,
    COLUMN_DURATION     =   4, 
    COLUMN_DATE         =   5,

    N_COLUMNS
  };

  enum Direction
  {
    NEXT,
    PREV,
    CHECKONLY,
  };

  namespace UiPart
  {
    class Podcasts
      : public PlaybackSource,
        public Base
    {
        boost::shared_ptr<PodcastV2::PodcastManager> mPodcastManager;

        UidIterMap  mContextMap;
        UID         mUid;

      public:

        Podcasts (RefPtr<Gnome::Glade::Xml> const& xml,
                  RefPtr<Gtk::UIManager> const& ui_manager);

        virtual ~Podcasts ();

      protected:

        virtual guint
        add_ui ();

        virtual ustring
        get_uri ();

        virtual ustring
        get_type ();

        virtual bool
        go_next ();

        virtual bool
        go_prev ();

        virtual void
        stop ();

        virtual bool
        play ();
  
        virtual void
        play_post ();

        virtual void
        prev_post ();

        virtual void
        next_post ();

        virtual void
        restore_context ();

        virtual void
        buffering_done ();

     private:

        //// System Callbacks
        bool  on_shutdown_request ();

        //// Action Callbacks
        void  on_new_podcast ();
        void  on_remove_podcast ();
        void  on_update_podcast ();
        void  on_update_all ();
        void  on_copy_xml_link ();
        void  on_copy_web_link ();
        void  on_export_feedlist ();
	void  on_sorting_changed ();

        //// Internal functions
        void current_assign (TreeIter const& iter);
        void current_clear ();
        void current_check_played_and_caps ();

        bool sel_uid_is_current_uid ();
        void has_next_prev (bool & next, bool & prev);

        void go_next_prev  (Direction direction);
        void send_metadata ();

        bool
        update_all_cycle ();

        void
        add_cast_internal (PodcastV2::Podcast&);

        void
        upd_cast_internal (ustring const& uri,
                           Gtk::TreeIter&);

        struct MiscWidgetsT
        {
          Sexy::IconEntry   * FilterEntry;
          Gtk::TextView     * TextViewDescription;
        };

        MiscWidgetsT MiscWidgets;

        // Episodes
        void  items_cell_data_func (Gtk::CellRenderer * cell, Gtk::TreeIter const& m_iter, EpisodeColumn column, EpisodeColumnType type);
        int   items_default_sort_func (Gtk::TreeIter const& iter_a, Gtk::TreeIter const& iter_b);
        void  items_on_selection_changed ();
        void  items_on_column_download_toggled (ustring const&);
        void  items_on_row_activated (Gtk::TreePath const& ,
                                      Gtk::TreeViewColumn* );

        struct EpisodesDataT
        {
          Episodes                ColumnRecord;
          Gtk::TreeView*          View;
          RefPtr<Gtk::ListStore>  Store;
        };

        EpisodesDataT EpisodesData;

        // Casts
        void  casts_test_cell_data_func (Gtk::CellRenderer * cell, Gtk::TreeIter const& m_iter);
        int   casts_default_sort_func (Gtk::TreeIter const& iter_a, Gtk::TreeIter const& iter_b);
        bool  casts_visible_func (Gtk::TreeIter const& iter);
        bool  casts_on_event (GdkEvent * ev);
        void  casts_on_selection_changed ();
        void  casts_image_load (PodcastV2::Podcast&, Gtk::TreeIter&);

        struct CastDataT
        {
          Casts                         ColumnRecord;
          Gtk::TreeView*                View;
          RefPtr<Gtk::ListStore>        Store;
          RefPtr<Gtk::TreeModelFilter>  FilterStore;
        };

        CastDataT CastData;

        struct IconsT
        {
          RefSurface  Rss; 
          RefSurface  New; 
          RefSurface  Playing; 
          RefSurface  Updating; 
        };

        IconsT Icons;

        struct StateDataT
        {
          OptionalTreeIter  Iter;
          Podcast           Cast;
          Episode           Item;
          RefPixbuf         Image;
          UID               CastUid;
          ustring           ItemUid;
        };

        StateDataT StateData;


        //// Downloads
        struct DownloadContextBase
        {
          Soup::RequestFileRefP FileRequest;
          ustring               Uri;
          double                DownloadProgress;
        };

        struct DownloadContext
          : public DownloadContextBase
        {
          Episode EpisodeDownloaded;
        };

        typedef boost::shared_ptr<DownloadContext> DownloadContextRefp;
        typedef std::pair<UID, std::string> DownloadKey;

        /* typedef std::tr1::unordered_map<DownloadKey, DownloadContextRefp> DownloadMap; FIXME: Write hash func */
        typedef std::map<DownloadKey, DownloadContextRefp> DownloadMap;

        void  download_done (std::string const&, DownloadKey);  //<- callback
        void  download_aborted (std::string const&, DownloadKey); //<- callback
        void  download_progress (double, DownloadKey /*key*/); //<- callback

        void  download_episode (Podcast&, Episode&, DownloadKey&); 
        void  download_remove (DownloadKey); /* NOT a callback; merely resetting our internal download info state */
        void  download_cancel (DownloadKey); /* -"""- */

        Glib::Mutex DataLock;

        struct DownloadDataT
        {
          DownloadMap Downloads;
          UidSet      DownloadUidSet;
        };

        DownloadDataT DownloadData;

        //// Updates
        typedef std::pair<ustring, TreeIter> UpdateSet;
        typedef std::deque<UpdateSet> UpdatePool;

        struct UpdateDataT
        {
          UpdatePool        Pool;
          sigc::connection  TimeoutConnection;
        };

        UpdateDataT UpdateData;

        void  on_podcast_updated (ustring const& uri);
        void  on_podcast_not_updated (ustring const& uri);
        void  run_update ();
        void  next_update ();
        void  mcs_update_interval_changed (MCS_CB_DEFAULT_SIGNATURE);

    }; // Playlist
  } // namespace UiPart
} // namespace Bmp

#endif // !BMP_UI_PART_PODCASTS_HH

