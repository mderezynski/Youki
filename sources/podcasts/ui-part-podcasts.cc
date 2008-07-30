//  BMP
//  Copyright (C) 2003-2008 http://bmpx.baxcktrace.info
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <time.h>
#include <errno.h>

#include <glib/gstdio.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <libglademm.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

#include "mpx-main.hh"
#include "mpx-network.hh"
#include "paths.hh"
#include "mpx-stock.hh"
#include "debug.hh"
#include "util.hh"
#include "util-file.hh"
#include "ui-tools.hh"
#include "mpx-uri.hh"

#include "bmp/library-ops.hh"

#include "dialog-add-podcast.hh"
#include "dialog-remove-update-podcast.hh"
#include "dialog-simple-progress.hh"

#include "core.hh"
#include "audio/play.hh"
#include "audio/audio-typefind.hh"
#include "video-widget.hh"
#include "ui-part-podcasts.hh"

#include "widgets/cairoextensions.hh"
#include "widgets/cell-renderer-count.hh"

using namespace Glib;
using namespace Gtk;
using namespace std;
using namespace Bmp::PodcastV2;
using Bmp::URI;

#define POD_ACTION_ADD_PODCAST      "pod-action-add-podcast"
#define POD_ACTION_DEL_PODCAST      "pod-action-del-podcast"
#define POD_ACTION_UPD_PODCAST      "pod-action-upd-podcast"
#define POD_ACTION_UPD_PODCAST_ALL  "pod-action-upd-podcast-all"
#define POD_ACTION_COPY_RSS_LINK    "pod-action-copy-rss-link"
#define POD_ACTION_COPY_WEB_LINK    "pod-action-copy-web-link"
#define POD_ACTION_EXPORT_FEEDLIST  "pod-action-export-feedlist"
#define POD_ACTION_SORT_BY  	    "pod-action-sort-by"
#define POD_ACTION_SORT_BY_NAME     "pod-action-sort-by-name"
#define POD_ACTION_SORT_BY_DATE     "pod-action-sort-by-date"

namespace
{
  const char * ui_string_casts_popup =
  "<ui>"
  ""
  "<menubar name='popup-podcasts-casts'>"
  ""
  "   <menu action='dummy' name='menu-podcasts-casts'>"
  "     <menuitem action='" POD_ACTION_UPD_PODCAST "'/>"
  "     <menuitem action='" POD_ACTION_DEL_PODCAST "'/>"
  "       <separator name='podcasts-1'/>"
  "     <menuitem action='" POD_ACTION_COPY_RSS_LINK "'/>"
  "     <menuitem action='" POD_ACTION_COPY_WEB_LINK "'/>"
  "   </menu>"
  ""
  "</menubar>"
  ""
  "</ui>";

  const char *ui_string_podcasts =
  "<ui>"
  ""
  "<menubar name='MenuBarMain'>"
  "   <placeholder name='PlaceholderSource'>"
  "   <menu action='MenuUiPartPodcasts'>"
  "     <menuitem action='" POD_ACTION_ADD_PODCAST "'/>"
  "     <separator name='podcasts-3'/>"
  "     <menuitem action='" POD_ACTION_COPY_RSS_LINK "'/>"
  "     <menuitem action='" POD_ACTION_COPY_WEB_LINK "'/>"
  "     <separator name='podcasts-2'/>"
  "     <menuitem action='" POD_ACTION_EXPORT_FEEDLIST "'/>"
  "     <separator name='podcasts-3'/>"
  "     <menuitem action='" POD_ACTION_UPD_PODCAST_ALL "'/>"
  "     <separator name='podcasts-4'/>"
  "     <menu action='" POD_ACTION_SORT_BY "'>"
  "     	<menuitem action='" POD_ACTION_SORT_BY_NAME "'/>"
  "     	<menuitem action='" POD_ACTION_SORT_BY_DATE "'/>"
  "	</menu>"
  "   </menu>"
  "   </placeholder>"
  "</menubar>"
  ""
  "</ui>";

  const char * error_messages[] =  
  {
    "An error occured while loading the RSS feed: %s",
    "The feed XML (RSS feed/Podcast XML) could not be parsed: %s",
    "The specified Feed URL is invalid: %s",
    "Encountered an invalid URI during parsing: %s",
  };

  std::string
  get_timestr_from_time_t (time_t atime)
  {
    struct tm atm, btm;
    localtime_r (&atime, &atm);
    time_t curtime = time(NULL);
    localtime_r (&curtime, &btm);

    static boost::format
      date_f ("%s, %s");

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

  std::string
  get_timestr_from_time_t_item (time_t atime)
  {
    struct tm atm;
    localtime_r (&atime, &atm);

    char bdate[64];
    strftime (bdate, 64, "%d/%m/%Y", &atm);

    char btime[64];

    strftime (btime, 64, "%H:%M", &atm);

    static boost::format
      date_f ("<b>%s</b> (%s)");

    return locale_to_utf8 ((date_f % bdate % btime).str());
  }

  bool
  video_type (std::string const& type)
  {
    bool is_video (type.substr (0, 6) == std::string("video/"));
    return is_video;
  }

  static boost::format
    podcast_title_f N_("<b>%s</b>\n<small><i>%s</i></small> <span color='#ff0000'>%s</span>");

  static boost::format
    podcast_descr_f N_("<small>%s</small>");
}

namespace Bmp
{
  namespace UiPart
  {
    Podcasts::Podcasts (RefPtr<Gnome::Glade::Xml> const& xml, RefPtr<UIManager> const& ui_manager)
    : PlaybackSource    (_("Podcasts"), NONE, F_ALWAYS_IMAGE_FRAME)
    , Base              (xml, ui_manager)
    , mUid              (1)
    {
      mPodcastManager = boost::shared_ptr<PodcastManager> (new PodcastManager());
      mPodcastManager->signal_podcast_updated().connect( sigc::mem_fun ( *this, &Podcasts::on_podcast_updated ) );
      mPodcastManager->signal_podcast_not_updated().connect( sigc::mem_fun ( *this, &Podcasts::on_podcast_not_updated ) );

      m_actions = ActionGroup::create ("ActionsPodcasts");
      m_actions->add (Action::create ("dummy", "dummy"));
      m_actions->add (Action::create ("MenuUiPartPodcasts", _("Po_dcasts")));
      m_actions->add (Action::create (POD_ACTION_SORT_BY, _("_Sort Podcasts By...")));

      m_actions->add (Action::create (POD_ACTION_ADD_PODCAST,
                                      StockID (BMP_STOCK_FEED_ADD),
                                      _("_Add Podcast...")),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_new_podcast));

      m_actions->add (Action::create (POD_ACTION_DEL_PODCAST,
                                      StockID (BMP_STOCK_FEED_DELETE),
                                      _("Remove")),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_remove_podcast));

      m_actions->add (Action::create (POD_ACTION_UPD_PODCAST,
                                      StockID (BMP_STOCK_FEED_UPDATE),
                                      _("Update")),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_update_podcast));

      m_actions->add (Action::create (POD_ACTION_UPD_PODCAST_ALL,
                                      StockID (BMP_STOCK_FEED_UPDATE),
                                      _("Update All")),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_update_all));

      m_actions->add (Action::create (POD_ACTION_COPY_RSS_LINK,
                                      Stock::COPY,
                                      _("_Copy Feed XML Link")),
                                      AccelKey (""),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_copy_xml_link));

      m_actions->add (Action::create (POD_ACTION_COPY_WEB_LINK,
                                      Stock::COPY,
                                      _("_Copy Podcast URL")),
                                      AccelKey (""),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_copy_web_link));

      m_actions->add (Action::create (POD_ACTION_EXPORT_FEEDLIST,
                                      Stock::SAVE,
                                      _("Export Feedlist (OPML Format)...")),
                                      AccelKey (""),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_export_feedlist));
 
      Gtk::RadioButtonGroup gr1;

      m_actions->add (RadioAction::create (gr1, POD_ACTION_SORT_BY_NAME,
                                      _("Name")),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_sorting_changed));

      m_actions->add (RadioAction::create (gr1, POD_ACTION_SORT_BY_DATE,
                                      _("Most Recent")),
                                      sigc::mem_fun (*this, &UiPart::Podcasts::on_sorting_changed));

      RefPtr<Gtk::RadioAction>::cast_static (m_actions->get_action (POD_ACTION_SORT_BY_NAME))->property_value() = 0; 
      RefPtr<Gtk::RadioAction>::cast_static (m_actions->get_action (POD_ACTION_SORT_BY_DATE))->property_value() = 1; 

      m_ui_manager->insert_action_group (m_actions);
      m_ui_manager->add_ui_from_string (ui_string_casts_popup);

      m_actions->get_action (POD_ACTION_COPY_RSS_LINK)->set_sensitive (false);
      m_actions->get_action (POD_ACTION_COPY_WEB_LINK)->set_sensitive (false);
      m_actions->get_action (POD_ACTION_DEL_PODCAST)->set_sensitive (false);
      m_actions->get_action (POD_ACTION_UPD_PODCAST)->set_sensitive (false);

      Icons.Rss = Util::cairo_image_surface_from_pixbuf
        (Gdk::Pixbuf::create_from_file (build_filename (BMP_IMAGE_DIR_PODCAST, "feed-default.png"))->scale_simple (48, 48, Gdk::INTERP_HYPER));

      Icons.New = Util::cairo_image_surface_from_pixbuf
        (Gdk::Pixbuf::create_from_file (build_filename (BMP_IMAGE_DIR_STOCK, "silk-new.png")));

      Icons.Playing = Util::cairo_image_surface_from_pixbuf
        (Gdk::Pixbuf::create_from_file (build_filename (BMP_IMAGE_DIR_STOCK, "play.png")));

      Icons.Updating = Util::cairo_image_surface_from_pixbuf
        (Gdk::Pixbuf::create_from_file (build_filename (BMP_IMAGE_DIR_PODCAST, "updating.png"))->scale_simple (16, 16, Gdk::INTERP_HYPER));

      m_ref_xml->get_widget ("podcasts-textview", MiscWidgets.TextViewDescription);
      m_ref_xml->get_widget ("podcasts-view-casts", CastData.View);
      m_ref_xml->get_widget ("podcasts-view-items", EpisodesData.View);

      //////////////////
      // Casts        //
      //////////////////

      TreeViewColumn *column = manage (new TreeViewColumn (_("Podcast")));

      {
        CellRendererCairoSurface *cell = manage (new CellRendererCairoSurface());
        cell->property_xalign() = 1.0; 
        cell->property_yalign() = 0.0;
        cell->property_xpad() = 0;
        cell->property_ypad() = 4;
        cell->set_fixed_size (64, 56);
        column->pack_start (*cell, false);
        column->add_attribute (*cell, "surface", CastData.ColumnRecord.image);
      }

      {
        CellRendererText *cell = manage (new CellRendererText());
        cell->property_xalign() = 0.0;
        cell->property_yalign() = 0.0;
        cell->property_xpad() = 4;
        cell->property_ypad() = 2;
        cell->property_ellipsize() = Pango::ELLIPSIZE_END;
        column->pack_start (*cell, true);
        column->add_attribute (*cell, "markup", CastData.ColumnRecord.title);
      }
      CastData.View->append_column (*column);
      column->set_resizable();

      CastData.Store = ListStore::create (CastData.ColumnRecord);
      CastData.FilterStore = TreeModelFilter::create (CastData.Store);

      MiscWidgets.FilterEntry = manage (new Sexy::IconEntry());
      Label * label = manage (new Label());
      label->set_markup_with_mnemonic (_("_Search:"));
      label->set_mnemonic_widget (*MiscWidgets.FilterEntry);
      label->show();

      MiscWidgets.FilterEntry->show();
      MiscWidgets.FilterEntry->add_clear_button ();
      MiscWidgets.FilterEntry->signal_changed().connect (sigc::mem_fun (CastData.FilterStore.operator->(), &TreeModelFilter::refilter));

      dynamic_cast<HBox*>(m_ref_xml->get_widget ("podcasts-hbox-filter"))->pack_start (*label, false, false);
      dynamic_cast<HBox*>(m_ref_xml->get_widget ("podcasts-hbox-filter"))->pack_start (*MiscWidgets.FilterEntry, true, true);

      CastData.FilterStore->set_visible_func (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::casts_visible_func));
      CastData.View->set_model (CastData.FilterStore);

      CastData.View->get_selection()->set_mode (SELECTION_SINGLE);
      CastData.View->get_selection()->signal_changed().connect (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::casts_on_selection_changed));
      CastData.View->signal_event().connect (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::casts_on_event));

      CastData.Store->set_default_sort_func (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::casts_default_sort_func));
      CastData.Store->set_sort_column (-1, SORT_ASCENDING);

      // Episodes
      for (unsigned int n = 0; n < N_COLUMNS; ++n)
      {
        switch (n)
        {
          case COLUMN_DOWNLOADED:
          {
            TreeViewColumn *column = manage (new TreeViewColumn ());
            Image * disk = manage (new Image (EpisodesData.View->render_icon (StockID (BMP_STOCK_DISK), ICON_SIZE_MENU)));
            disk->show_all ();
            column->set_widget (*disk);            

            CellRendererToggle * cell = manage (new CellRendererToggle());

            cell->property_xalign() = 0.5;
            cell->property_ypad() = 2;
            cell->signal_toggled ().connect
              (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_on_column_download_toggled));

            column->set_resizable (false);
            column->set_expand (false);

            column->pack_start (*cell, false);
            column->set_cell_data_func
              (*cell, sigc::bind (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_cell_data_func), COLUMN_DOWNLOADED, CELL_TOGGLE));

            EpisodesData.View->append_column (*column);
            break;
          }

          case COLUMN_DOWNLOADING:
          {
            TreeViewColumn *column = manage (new TreeViewColumn ());

            CellRendererProgress *cell = manage (new CellRendererProgress());
            cell->property_xalign() = 0.0;
            cell->property_ypad() = 2;

            column->set_resizable (false);
            column->set_expand (false);
            column->property_visible() = false;

            column->pack_start (*cell, false);
            column->set_cell_data_func
              (*cell, sigc::bind (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_cell_data_func), COLUMN_DOWNLOADING, CELL_PROGRESS));

            EpisodesData.View->append_column (*column);
            break;
          }

          case COLUMN_PLAYING:
          {
            TreeViewColumn *column = manage (new TreeViewColumn ()); 

            CellRendererCairoSurface *cell = manage (new CellRendererCairoSurface());
            cell->property_xalign() = 0.5;
            cell->property_width() = 22;

            column->pack_start (*cell, false);
            column->set_cell_data_func
                (*cell, sigc::bind (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_cell_data_func), COLUMN_PLAYING, CELL_SURFACE));

            column->set_resizable (false);
            column->set_expand (false);

            EpisodesData.View->append_column (*column);
            break;
          }

          case COLUMN_TITLE:
          {
            TreeViewColumn *column = manage (new TreeViewColumn (_("Episode"))); 

            CellRendererText *cell2 = manage (new CellRendererText());
            cell2->property_xpad() = 2;
            cell2->property_ypad() = 2;
            cell2->property_ellipsize() = Pango::ELLIPSIZE_END;

            column->pack_end (*cell2, true);
            column->set_cell_data_func
                (*cell2, sigc::bind (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_cell_data_func), COLUMN_TITLE, CELL_TEXT));

            column->set_resizable (true);
            column->set_expand (false);
            column->set_min_width (200);

            EpisodesData.View->append_column (*column);
            break;
          }

          case COLUMN_DATE:
          {
            CellRendererText *cell = manage (new CellRendererText());
            TreeViewColumn *column  = manage (new TreeViewColumn (_("Date"), *cell));

            cell->property_xpad() = 2;
            cell->property_ypad() = 2;

            column->set_cell_data_func
              (*cell, sigc::bind (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_cell_data_func), COLUMN_DATE, CELL_TEXT));
            column->set_resizable (false);
            column->set_expand (false);
            EpisodesData.View->append_column (*column);
            break;
          }
        }
      }

      EpisodesData.Store = ListStore::create (EpisodesData.ColumnRecord);

      EpisodesData.View->set_model (EpisodesData.Store);
      EpisodesData.View->get_selection()->set_mode (SELECTION_SINGLE);
      EpisodesData.View->get_selection()->signal_changed().connect
          (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_on_selection_changed));
      EpisodesData.View->signal_row_activated().connect
          (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_on_row_activated));
      EpisodesData.Store->set_default_sort_func
          (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::items_default_sort_func));
      EpisodesData.Store->set_sort_column (-1, SORT_ASCENDING);

      PodcastList list;
      mPodcastManager->podcast_get_list (list);
      for (PodcastList::const_iterator c = list.begin() ; c != list.end () ; ++c)
      {
         try {
            add_cast_internal (mPodcastManager->podcast_fetch (*c));
            }
          catch (PodcastV2::ParsingError& cxe)
            {
              g_warning ("%s: Parsing Error during reading of podcast: %s",
                G_STRLOC, cxe.what());
              break;
            }
          catch (PodcastV2::InvalidUriError& cxe)
            {
              g_warning ("%s: URI Is Invalid: %s",
                G_STRLOC, cxe.what());
              break;
            }
      }

      Core::Obj()->signal_shutdown_request().connect
            (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::on_shutdown_request));

      ::mcs->subscribe ("UiPart_Podcasts", "podcasts", "update-interval",
          sigc::mem_fun (*this, &Podcasts::mcs_update_interval_changed));

      if( ::mcs->key_get<int> ("podcasts", "update-interval") == Policy::UPDATE_HOURLY )
        UpdateData.TimeoutConnection = signal_timeout().connect (sigc::mem_fun (*this, &Podcasts::update_all_cycle), 3600U * 1000U); 
      else if( ::mcs->key_get<int> ("podcasts", "update-interval") == Policy::UPDATE_30MINS )
        UpdateData.TimeoutConnection = signal_timeout().connect (sigc::mem_fun (*this, &Podcasts::update_all_cycle), 1800U * 1000U); 
      else if( ::mcs->key_get<int> ("podcasts", "update-interval") == Policy::UPDATE_STARTUP )
        update_all_cycle ();
    }

    void
    Podcasts::on_sorting_changed ()
    {
	CastData.Store->unset_default_sort_func ();
        CastData.Store->set_default_sort_func (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::casts_default_sort_func));
    }

    guint
    Podcasts::add_ui ()
    {
      if( !NM::Obj()->Check_Status() )
        return 0;
      else
        return m_ui_manager->add_ui_from_string (ui_string_podcasts);
    };

    void
    Podcasts::casts_image_load (Podcast & cast, TreeIter & iter)
    {
      if( cast.podcast.count("image") != 0)
      {
        try{
          RefSurface i1 = Util::cairo_image_surface_from_pixbuf 
            (Gdk::Pixbuf::create_from_file (cast_image_filename (cast))->scale_simple (48, 48, Gdk::INTERP_BILINEAR));
          Util::cairo_image_surface_border (i1, 1.);
          (*iter)[CastData.ColumnRecord.image] = i1;
          return;
        }
        catch (...) {}
      }

      (*iter)[CastData.ColumnRecord.image] = Icons.Rss; 
    }

    void
    Podcasts::upd_cast_internal (ustring const& uri,
                                 TreeIter     & iter)
    {
      Podcast & cast = mPodcastManager->podcast_fetch (uri);
      Episode & item = cast.get_most_recent_item();

      ustring title = ustring();

      if( cast.item_map.empty() )
      {
        title = ((podcast_title_f % Markup::escape_text (cast.podcast["title"]).c_str()
                                  % _("(No Current Episodes)")
				  % (!cast.most_recent_is_played ? "*" : "")).str());
      }
      else
      {
        title = ((podcast_title_f % Markup::escape_text (cast.podcast["title"]).c_str()
                                  % Markup::escape_text (get_timestr_from_time_t (cast.most_recent_item)).c_str()
				  % (!cast.most_recent_is_played ? "*" : "")).str());
      }

      casts_image_load (cast, iter);

      (*iter)[CastData.ColumnRecord.uri] = cast.podcast["source"];
      (*iter)[CastData.ColumnRecord.status] = ST_DEFAULT; 
      (*iter)[CastData.ColumnRecord.title] = title;
      (*iter)[CastData.ColumnRecord.key] = ustring (cast.podcast["title"]).casefold_collate_key();
      (*iter)[CastData.ColumnRecord.most_recent] = cast.most_recent_item;
      (*iter)[CastData.ColumnRecord.searchtitle] = cast.podcast["title"];
      (*iter)[CastData.ColumnRecord.description] = (podcast_descr_f % cast.podcast["description"].c_str()).str();

      if( ::mcs->key_get<int> ("podcasts", "download-policy") == Policy::DOWNLOAD_MOST_RECENT )
      {
          if( cast.new_items && !item.getAsString("enclosure").empty())
          {
                
                DownloadKey key (((*iter)[CastData.ColumnRecord.uid]), item.getAsString("guid"));
                download_episode( cast, item, key ); 
          }  
      }

      if( CastData.View->get_selection()->count_selected_rows() )
      {
        TreeIter iter2 (CastData.View->get_selection ()->get_selected());
        if ( UID ((*iter2)[CastData.ColumnRecord.uid]) == UID ((*iter)[CastData.ColumnRecord.uid]))
        {
          casts_on_selection_changed ();
        }
      }
    }

    void
    Podcasts::add_cast_internal (Podcast & cast)
    {
      Episode & item = cast.get_most_recent_item();
      ustring title = ustring();

      if( cast.item_map.empty() )
      {
        title = ((podcast_title_f % Markup::escape_text (cast.podcast["title"]).c_str()
                                  % _("(No Current Episodes)")
				  % (!cast.most_recent_is_played ? "*" : "")).str());
      }
      else
      {
        title = ((podcast_title_f % Markup::escape_text (cast.podcast["title"]).c_str()
                                  % Markup::escape_text (get_timestr_from_time_t (cast.most_recent_item)).c_str()
				  % (!cast.most_recent_is_played ? "*" : "")).str());
      }

      TreeIter iter = CastData.Store->append ();

      casts_image_load (cast, iter);
      (*iter)[CastData.ColumnRecord.uri] = cast.podcast["source"];
      (*iter)[CastData.ColumnRecord.status] = ST_DEFAULT; 
      (*iter)[CastData.ColumnRecord.title] = title;
      (*iter)[CastData.ColumnRecord.key] = ustring (cast.podcast["title"]).casefold_collate_key();
      (*iter)[CastData.ColumnRecord.most_recent] = cast.most_recent_item;
      (*iter)[CastData.ColumnRecord.searchtitle] = cast.podcast["title"];
      (*iter)[CastData.ColumnRecord.description] = (podcast_descr_f % cast.podcast["description"].c_str()).str();
      (*iter)[CastData.ColumnRecord.uid] = mUid;

      mContextMap.insert (std::make_pair (mUid, iter));
      mUid++;

      if( ::mcs->key_get<int> ("podcasts", "download-policy") == Policy::DOWNLOAD_MOST_RECENT )
      {
          if( cast.new_items && !item.getAsString("enclosure").empty())
          {
                DownloadKey key (((*iter)[CastData.ColumnRecord.uid]), item.getAsString("guid"));
                download_episode( cast, item, key ); 
          }  
      }

      //CastData.View->get_selection()->select (CastData.FilterStore->convert_child_iter_to_iter (iter));
      //CastData.View->scroll_to_row(CastData.Store->get_path( iter ), 0.5);
    }

    void
    Podcasts::on_remove_podcast ()
    {
      TreeIter iter = CastData.FilterStore->convert_iter_to_child_iter (CastData.View->get_selection()->get_selected());
      Podcast & cast = mPodcastManager->podcast_fetch ((*iter)[CastData.ColumnRecord.uri]);

      static boost::format question_f ("Are you sure you want to remove '<b>%s</b>'?");
      MessageDialog dialog ((question_f % Markup::escape_text(cast.podcast["title"]).c_str()).str(), true, MESSAGE_QUESTION, BUTTONS_YES_NO, true);

      if( dialog.run() == GTK_RESPONSE_YES )
      {
        UID uid ((*iter)[CastData.ColumnRecord.uid]);
        CastData.View->get_selection()->unselect (iter);
        mPodcastManager->podcast_delete ((*iter)[CastData.ColumnRecord.uri]);
        CastData.Store->erase (iter);
        mContextMap.erase (uid);
      }
    }

    void
    Podcasts::on_update_all ()
    {
      update_all_cycle ();
    }

    void
    Podcasts::on_update_podcast ()
    {
      SimpleProgress * p = SimpleProgress::create ();
      p->show_all ();
      p->set_title (_("Updating Podcast..."));
      p->percentage (0.33);
      TreeIter iter = CastData.FilterStore->convert_iter_to_child_iter (CastData.View->get_selection()->get_selected());

      try{
        ustring uri = ((*iter)[CastData.ColumnRecord.uri]);
        p->set_label (uri);
        p->percentage (0.66);
        mPodcastManager->podcast_update (uri);
        upd_cast_internal (uri, iter);
        p->percentage (1.);
        }
      catch (PodcastV2::Exception & cxe)
        {
          delete p;
          boost::format error_f (error_messages[0]);
          MessageDialog dialog ((error_f % cxe.what()).str(), false, MESSAGE_ERROR, BUTTONS_OK, true);
          dialog.run ();
          return;
        }
      delete p;
    }

    void
    Podcasts::on_new_podcast ()
    {
      DialogAddPodcast * dialog (DialogAddPodcast::create());
      ustring uri;
      int response = dialog->run (uri);
      delete dialog;

      if( response == RESPONSE_OK )
      {
        m_ref_xml->get_widget ("podcasts-vbox")->set_sensitive (false);
        try{
            Bmp::URI u (uri);
            if(u.get_protocol() == URI::PROTOCOL_ITPC)
            {
              u.set_protocol (URI::PROTOCOL_HTTP);
              uri = ustring (u);
            }
            mPodcastManager->podcast_insert (uri);
            add_cast_internal (mPodcastManager->podcast_fetch (uri));
          }
        catch (PodcastV2::Podcast::ParseError & cxe)
          {
            boost::format error_f (error_messages[1]);
            MessageDialog dialog ((error_f % cxe.what()).str(), false, MESSAGE_ERROR, BUTTONS_OK, true);
            dialog.run ();
          }
        catch (PodcastV2::Exception & cxe)
          {
            boost::format error_f (error_messages[0]);
            MessageDialog dialog ((error_f % cxe.what()).str(), false, MESSAGE_ERROR, BUTTONS_OK, true);
            dialog.run ();
          }
        m_ref_xml->get_widget ("podcasts-vbox")->set_sensitive (true);
      }
    }

    void
    Podcasts::on_copy_xml_link ()
    {
      TreeIter iter = CastData.View->get_selection()->get_selected();
      Clipboard::get_for_display (Gdk::Display::get_default())->set_text ((*iter)[CastData.ColumnRecord.uri]);
    }

    void
    Podcasts::on_copy_web_link ()
    {
      TreeIter iter = CastData.View->get_selection()->get_selected();
      Podcast & cast = mPodcastManager->podcast_fetch ((*iter)[CastData.ColumnRecord.uri]);
      Clipboard::get_for_display (Gdk::Display::get_default())->set_text (cast.podcast["link"]);
    }

    void
    Podcasts::on_export_feedlist ()
    {
      FileChooserDialog dialog (_("Podcasts: Save Feed List - BMP"), FILE_CHOOSER_ACTION_SAVE);
      dialog.add_button (Stock::CANCEL, RESPONSE_CANCEL);
      dialog.add_button (Stock::SAVE, RESPONSE_OK);
      dialog.set_current_folder (mcs->key_get<string>("bmp", "file-chooser-path"));
      dialog.set_default_response (RESPONSE_CANCEL);
      dialog.set_default_size (750, 570);
      dialog.set_position (WIN_POS_CENTER);

      if( dialog.run() == RESPONSE_OK )
      {
        std::string filename = filename_from_uri (dialog.get_uri());
        if( filename.substr (filename.length() - 5, 5) != ".opml" )
        {
          filename += ".opml";
        }
        mPodcastManager->save_opml (filename);        
      }
    }

    void
    Podcasts::mcs_update_interval_changed (MCS_CB_DEFAULT_SIGNATURE)
    {
      UpdateData.TimeoutConnection.disconnect();

      if( ::mcs->key_get<int> ("podcasts", "update-interval") == Policy::UPDATE_HOURLY )
        UpdateData.TimeoutConnection = signal_timeout().connect (sigc::mem_fun (*this, &Podcasts::update_all_cycle), 3600U * 1000U); 
      else if( ::mcs->key_get<int> ("podcasts", "update-interval") == Policy::UPDATE_30MINS )
        UpdateData.TimeoutConnection = signal_timeout().connect (sigc::mem_fun (*this, &Podcasts::update_all_cycle), 1800U * 1000U); 
      else if( ::mcs->key_get<int> ("podcasts", "update-interval") == Policy::UPDATE_STARTUP )
        update_all_cycle ();
    }

    bool
    Podcasts::on_shutdown_request ()
    {
      if( !DownloadData.Downloads.empty() || !UpdateData.Pool.empty()) 
      {
        MessageDialog dialog (_("Podcast items still being downloaded. Are you sure you want to quit BMP?"),
                              true, MESSAGE_WARNING, BUTTONS_YES_NO, true);
        dialog.set_position (WIN_POS_CENTER);

        if (dialog.run() != GTK_RESPONSE_NO)
        { 
          for(DownloadMap::iterator i = DownloadData.Downloads.begin(); i != DownloadData.Downloads.end(); ++i)
          {
            i->second->FileRequest->cancel ();
          }
          UpdateData.Pool.clear();
          return true;
        }
        return false;
      }
      return true;
    }

    bool
    Podcasts::casts_visible_func (TreeIter const& iter)
    {
      ustring filter (ustring (MiscWidgets.FilterEntry->get_text()).lowercase());
      if( filter.empty() )
        return true;
      else
        return (ustring ((*iter)[CastData.ColumnRecord.searchtitle]).lowercase().find (filter) != ustring::npos);
    }

    Podcasts::~Podcasts ()
    { 
      for (DownloadMap::iterator i = DownloadData.Downloads.begin(); i != DownloadData.Downloads.end(); ++i)
      {
        i->second->FileRequest.clear();
      }
    }

    bool
    Podcasts::casts_on_event (GdkEvent * ev)
    {
      if( ev->type == GDK_BUTTON_PRESS )
      {
        GdkEventButton * event = reinterpret_cast <GdkEventButton *> (ev);
        if( event->button == 3 )
        {
          TreeViewColumn      * column;
          TreePath              path;
          int                   cell_x,
                                cell_y;

          if( CastData.View->get_path_at_pos (int (event->x), int (event->y), path, column, cell_x, cell_y) )
          {
            CastData.View->get_selection()->select (path);
            TreeIter iter = CastData.Store->get_iter (path);


            bool sensitivity = (DownloadData.DownloadUidSet.find (UID ((*iter)[CastData.ColumnRecord.uid])) == DownloadData.DownloadUidSet.end()) && UpdateData.Pool.empty();
            m_ui_manager->get_action (("/popup-podcasts-casts/menu-podcasts-casts/" POD_ACTION_UPD_PODCAST))->set_sensitive (sensitivity);
            m_ui_manager->get_action (("/popup-podcasts-casts/menu-podcasts-casts/" POD_ACTION_DEL_PODCAST))->set_sensitive (sensitivity);

            Menu * menu = dynamic_cast <Menu *>(Util::get_popup (m_ui_manager, "/popup-podcasts-casts/menu-podcasts-casts"));
            if (menu) 
            {
              menu->popup (event->button, event->time);
            }
            return true;
          }
        }
      }
      return false;
    }

    void
    Podcasts::items_on_column_download_toggled (ustring const& tree_path)
    {
      TreeIter iter = EpisodesData.View->get_model()->get_iter (tree_path);
      TreeIter iter_c = CastData.View->get_selection ()->get_selected();

      Podcast & cast = mPodcastManager->podcast_fetch ((*iter_c)[CastData.ColumnRecord.uri]);
      Episode & item = cast.item_map[Episode ((*iter)[EpisodesData.ColumnRecord.item]).attributes["guid"]];
 
      DownloadKey key (((*iter_c)[CastData.ColumnRecord.uid]), item.getAsString("guid"));

      if( StateData.Iter && (StateData.Iter.get() == iter) )
      {
        MessageDialog dialog (_("You have to stop playback of this item before you can download or delete it."),
                                false, MESSAGE_INFO, BUTTONS_OK, true);
        dialog.run ();
        return;
      }

      if( item.getAsBool("downloaded") ) 
      {
        DialogRemoveUpdatePodcast * p = DialogRemoveUpdatePodcast::create ();
        int response = p->run (cast, item);
        delete p;

        switch (response)
        {
          case 1: // (re)download
            break;

          case 0: // remove
          {
            MessageDialog dialog (_("Are you sure you want to <b>delete</b> this item from disk?"),
                                    true, MESSAGE_QUESTION, BUTTONS_YES_NO, true);
            if( dialog.run () == GTK_RESPONSE_YES )
            {
              if ( !g_unlink (item.attributes["filename"].c_str()) ) //TODO: Error when can't be deleted (using Bmp::FileIO?)
              {
                item.attributes["downloaded"] = "0";
                item.attributes["filename"] = ""; 
                (*iter)[EpisodesData.ColumnRecord.item] = item;
                cast.downloaded--;
                TreeIter child = CastData.FilterStore->convert_iter_to_child_iter (iter_c);
                CastData.Store->row_changed( CastData.Store->get_path( child ), child ); 
              }
              else
              {
                int errsv = errno;
                debug("ui-podcasts","Error: %s (%s)", g_strerror (errsv), item.attributes["filename"].c_str());
              }
            }
            return; 
          }

          case GTK_RESPONSE_CANCEL:
	  case GTK_RESPONSE_CLOSE:
	  case GTK_RESPONSE_DELETE_EVENT:
            return;
        }
      }

      if( !DownloadData.Downloads.empty() )
      {
        if( DownloadData.Downloads.find (key) != DownloadData.Downloads.end() ) 
        {
          MessageDialog dialog (_("Are you sure you want to abort the download of this item?"),
                                false, MESSAGE_QUESTION, BUTTONS_YES_NO, true);
          if (dialog.run() == GTK_RESPONSE_YES) 
          {
            download_cancel (key);
          }
          return; 
        }
      } 

      download_episode( cast, item , key ); 
    }

    void
    Podcasts::download_episode (Podcast & cast, Episode & item, DownloadKey & key) 
    {
      std::string path = cast_item_path (cast);
      std::string file; 

      try{
        file = cast_item_file (cast, item);
        }
      catch (EpisodeInvalidError & cxe)
        {
          MessageDialog dialog (
            _("An Error occured: This has a malformed URI and can not be downloaded."),
            false, MESSAGE_ERROR, BUTTONS_OK, true);
          dialog.run ();
          return;
        }
      catch (Glib::ConvertError & cxe)
        {
          MessageDialog dialog (
          (boost::format (_("A conversion error occured converting the Episode "
                           "title into a Filename (please try G_BROKEN_FILENAMES, Google if necessary):\n<i>%s</i>")) % cxe.what()).str(),
          false, MESSAGE_ERROR, BUTTONS_OK, true);
          dialog.run ();
          return;
        }

      Bmp::URI u (item.getAsString("enclosure"));
      if( u.get_protocol() != Bmp::URI::PROTOCOL_HTTP )
      {
        MessageDialog dialog (
          (boost::format (_("This item can not be downloaded (malformed URI): %s"))
          % item.getAsString("enclosure").c_str()).str(),
          false, MESSAGE_ERROR, BUTTONS_OK, true);
        dialog.run ();
        return;
      }

     if( g_mkdir_with_parents (path.c_str(), S_IRWXU) == 0 )
      {
        if( file_test (file, FILE_TEST_EXISTS) )
        {
          g_unlink (file.c_str());
        }

        item.attributes["downloaded"] = "";

        DownloadContextRefp dlrefp = DownloadContextRefp (new DownloadContext());

        dlrefp->FileRequest = Soup::RequestFile::create (item.getAsString("enclosure"), file); 
        dlrefp->FileRequest->file_progress().connect
          (sigc::bind (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::download_progress), key));
        dlrefp->FileRequest->file_done().connect
          (sigc::bind (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::download_done), key));
        dlrefp->FileRequest->file_aborted().connect
          (sigc::bind (sigc::mem_fun (*this, &Bmp::UiPart::Podcasts::download_aborted), key));

        dlrefp->Uri = cast.podcast["source"];
        dlrefp->EpisodeDownloaded = item;
        dlrefp->DownloadProgress = 0.; 

        DownloadData.DownloadUidSet.insert (key.first);
        DownloadData.Downloads.insert (std::make_pair (key, dlrefp));
        
        EpisodesData.View->get_column (COLUMN_DOWNLOADING)->property_visible() = true;
        dlrefp->FileRequest->run ();
        items_on_selection_changed ();
      }
    }

    void
    Podcasts::download_progress (double progress, DownloadKey key)
    {
      DownloadContextRefp & ctx = DownloadData.Downloads.find (key)->second;
      ctx->DownloadProgress = progress;
      EpisodesData.View->queue_draw ();
    }

    void
    Podcasts::download_done (std::string const& filename, DownloadKey key)
    {
      DownloadContextRefp & ctx = DownloadData.Downloads.find (key)->second;
      Episode & item = ctx->EpisodeDownloaded;
      item.attributes["downloaded"] = "1";
      item.attributes["filename"] = filename; 
      mPodcastManager->podcast_fetch_item (ctx->Uri, item.getAsString("guid")) = item;
      mPodcastManager->podcast_fetch (ctx->Uri).downloaded++;
      TreeIter iter = mContextMap.find( key.first )->second;
      CastData.Store->row_changed( CastData.Store->get_path( iter ), iter ); 

      if( CastData.View->get_selection()->count_selected_rows() )
      {
            TreeIter iter_c = CastData.View->get_selection ()->get_selected();
            UID uid ((*iter_c)[CastData.ColumnRecord.uid]);

            if( key.first == uid )
            {
                  TreeNodeChildren nodes = EpisodesData.Store->children();
                  for (TreeNodeChildren::iterator i = nodes.begin(); i != nodes.end(); ++i)
                  {
                        Episode node_item = (*i)[EpisodesData.ColumnRecord.item];
                        if (node_item.getAsString("guid") == item.getAsString("guid"))
                        {
                              (*i)[EpisodesData.ColumnRecord.item] = item; 
                              EpisodesData.View->queue_draw ();
                              break;
                        }
                  }
            }
      }
      download_remove (key);
    }

    void
    Podcasts::download_aborted (std::string const& message, DownloadKey key)
    {
      download_cancel (key); 
      MessageDialog dialog (message, false, MESSAGE_ERROR, BUTTONS_OK, true);
      dialog.run ();
    }

    void
    Podcasts::download_cancel (DownloadKey key)
    {
      DownloadContextRefp & ctx = DownloadData.Downloads.find (key)->second;
      ctx->FileRequest->cancel();
      download_remove (key);
    }

    void
    Podcasts::download_remove (DownloadKey key)
    {
      Glib::Mutex::Lock L (DataLock);

      UidSet::iterator i1= DownloadData.DownloadUidSet.find (key.first);
      if (i1 != DownloadData.DownloadUidSet.end())
        DownloadData.DownloadUidSet.erase (i1);

      DownloadMap::iterator i2 = DownloadData.Downloads.find (key);
      if (i2 != DownloadData.Downloads.end())
        DownloadData.Downloads.erase (i2);

      items_on_selection_changed ();
      
      if (DownloadData.Downloads.empty()) 
      {
        EpisodesData.View->get_column (COLUMN_DOWNLOADING)->property_visible() = false;
      }
    }

    bool
    Podcasts::sel_uid_is_current_uid ()
    {
      if( StateData.CastUid && CastData.View->get_selection()->count_selected_rows() )
      {
        TreeIter iter_c (CastData.View->get_selection ()->get_selected());
        return bool (UID ((*iter_c)[CastData.ColumnRecord.uid]) == StateData.CastUid);
      }

      return false;
    }

    int
    Podcasts::casts_default_sort_func (TreeIter const& iter_a, TreeIter const& iter_b)
    {
      int active = RefPtr<Gtk::RadioAction>::cast_static (m_actions->get_action (POD_ACTION_SORT_BY_NAME))->get_current_value();

      if (active == 0)	
      {
	ustring t1 = (*iter_a)[CastData.ColumnRecord.searchtitle];
	ustring t2 = (*iter_b)[CastData.ColumnRecord.searchtitle];
	return t1.compare (t2);
      }
      else if (active == 1)
      {     
	time_t most_recent_1 = (*iter_a)[CastData.ColumnRecord.most_recent],
	       most_recent_2 = (*iter_b)[CastData.ColumnRecord.most_recent];

	if(most_recent_1 == most_recent_2)
	  return 0;

	if(most_recent_1 < most_recent_2)
	  return 1;

	if(most_recent_1 > most_recent_2)
	  return -1;
       }

       g_assert_not_reached();
    }

    int
    Podcasts::items_default_sort_func (TreeIter const& iter_a, TreeIter const& iter_b)
    {
      Episode item_a = ((*iter_a)[EpisodesData.ColumnRecord.item]);
      Episode item_b = ((*iter_b)[EpisodesData.ColumnRecord.item]);

      time_t t_a (item_a.getAsInt("pub-date-unix"));
      time_t t_b (item_b.getAsInt("pub-date-unix"));

      if( t_a > t_b )
        return -1;

      if( t_a < t_b )
        return  1;

      return 0;
    }

    void
    Podcasts::on_podcast_updated (ustring const& uri)
    {
      Glib::Mutex::Lock L (DataLock);

      UpdateSet const& x = UpdateData.Pool.front();

      try{
          upd_cast_internal (x.first, const_cast<TreeIter&>(x.second));
        }
      catch (PodcastV2::Exception & cxe)
        {
          boost::format error_f (error_messages[0]);
          MessageDialog dialog ((error_f % cxe.what()).str(), false, MESSAGE_ERROR, BUTTONS_OK, true);
          dialog.run ();
        }

      next_update ();
    }

    void
    Podcasts::on_podcast_not_updated (ustring const& uri)
    {
      Glib::Mutex::Lock L (DataLock);

      next_update ();
    }
  
    void
    Podcasts::run_update ()
    {
      if( !UpdateData.Pool.empty() )
      {
        m_actions->get_action( POD_ACTION_ADD_PODCAST )->set_sensitive( 0 );
        m_actions->get_action( POD_ACTION_DEL_PODCAST )->set_sensitive( 0 );
        m_actions->get_action( POD_ACTION_UPD_PODCAST )->set_sensitive( 0 );

        UpdateSet const& x = UpdateData.Pool.front();
        mPodcastManager->podcast_update_async (x.first);
      }
      else
      {
        m_actions->get_action( POD_ACTION_ADD_PODCAST )->set_sensitive( 1 );
        m_actions->get_action( POD_ACTION_DEL_PODCAST )->set_sensitive( 1 );
        m_actions->get_action( POD_ACTION_UPD_PODCAST )->set_sensitive( 1 );
      }
    }

    void
    Podcasts::next_update ()
    {
      UpdateData.Pool.pop_front ();
      run_update ();
    }

    bool
    Podcasts::update_all_cycle ()
    {
      if( !UpdateData.Pool.empty() )
        return true;

      UpdateData.Pool.clear ();
      mPodcastManager->clear_pending_requests ();
  
      TreeNodeChildren const& children = CastData.Store->children();
      for (TreeNodeChildren::iterator i = children.begin(); i != children.end(); ++i)
      {
        // if it's being downloaded, skip it in this cycle FIXME: Make it possible to update during downloads
        if (DownloadData.DownloadUidSet.find ((*i)[CastData.ColumnRecord.uid]) != DownloadData.DownloadUidSet.end())
          continue;

        Podcast & cast = mPodcastManager->podcast_fetch ((*i)[CastData.ColumnRecord.uri]);
        UpdateData.Pool.push_back (UpdateSet (cast.podcast["source"], i)); 
        (*i)[CastData.ColumnRecord.status] = ST_PENDING; 
      }

      run_update ();

      return ( ::mcs->key_get<int> ("podcasts", "update-interval") != Policy::UPDATE_STARTUP );
    }

    void
    Podcasts::casts_test_cell_data_func (CellRenderer * basecell, TreeIter const& iter)
    {
      CellRendererCount * cell = dynamic_cast<CellRendererCount*>(basecell);

      Podcast & cast = mPodcastManager->podcast_fetch (ustring ((*iter)[CastData.ColumnRecord.uri]));
      if( ((*iter)[CastData.ColumnRecord.status]) == ST_PENDING)
      {
        cell->property_box() = BOX_DASHED;
      }
      else
      if(cast.downloaded)
      {
        cell->property_text() = ((boost::format ("<small><b>%llu</b></small>") % cast.downloaded).str());
        cell->property_box() = BOX_NORMAL;
      }
      else
      {
        cell->property_box() = BOX_NOBOX;
      }
    }

    void
    Podcasts::items_cell_data_func (CellRenderer * basecell, TreeIter const& iter, EpisodeColumn column, EpisodeColumnType type)
    {
      CellRendererText          * _cell_tx = 0;
      CellRendererCairoSurface  * _cell_cs = 0;
      CellRendererToggle        * _cell_tg = 0;
      CellRendererProgress      * _cell_pg = 0;

      switch (type)
      {
        case CELL_TEXT:
          _cell_tx = dynamic_cast<CellRendererText *>(basecell);
          break;

        case CELL_SURFACE:
          _cell_cs = dynamic_cast<CellRendererCairoSurface *>(basecell);
          break;

        case CELL_TOGGLE:
          _cell_tg = dynamic_cast<CellRendererToggle *>(basecell);
          break;

        case CELL_PROGRESS:
          _cell_pg = dynamic_cast<CellRendererProgress *>(basecell);
          break;
      }
  
      Episode item = ((*iter)[EpisodesData.ColumnRecord.item]);
      guint64 duration = 0;

      switch (column)
      {
        case COLUMN_PLAYING:
          if( sel_uid_is_current_uid() && StateData.Iter && (EpisodesData.Store->get_path (StateData.Iter.get()) == EpisodesData.Store->get_path (iter)) )
            _cell_cs->property_surface() = Icons.Playing;
          else
          if( !item.getAsBool("played") )
            _cell_cs->property_surface() = Icons.New; 
          else
            _cell_cs->property_surface() = RefSurface(0); 
          break;

        case COLUMN_DOWNLOADED:
          if( item.getAsBool("downloaded") )
            _cell_tg->property_active() = true; 
          else
            _cell_tg->property_active() = false; 
          break;

        case COLUMN_DOWNLOADING:
          _cell_pg->property_visible() = false; 
          if( !DownloadData.Downloads.empty() ) 
          {
            if( CastData.View->get_selection()->count_selected_rows() )
            {
              TreeIter iter_c = CastData.View->get_selection ()->get_selected();
              UID uid ((*iter_c)[CastData.ColumnRecord.uid]);
            
              DownloadKey key (uid, item.getAsString("guid"));
              DownloadMap::iterator i = DownloadData.Downloads.find (key); 
              if (i != DownloadData.Downloads.end())
              {
                _cell_pg->property_visible() = true; 
                _cell_pg->property_value() = int (i->second->DownloadProgress * 100);
              }
            }
          }
          break;

        case COLUMN_TITLE:
          _cell_tx->property_markup() = Markup::escape_text (item.item["title"]);
          break;

        case COLUMN_DATE:
          _cell_tx->property_markup() = get_timestr_from_time_t_item (item.getAsInt("pub-date-unix"));
          break;

        case COLUMN_DURATION:
          duration = item.getAsInt("enclosure-length");
          _cell_tx->property_markup() = (boost::format ("%llu:%02llu") % (duration / 60) % (duration % 60)).str();
          break;

        default: /* N_COLUMNS */ break;
      }
    }

    ///////////////////////////////////////////////////////////////////////////////////////

    void
    Podcasts::has_next_prev (bool & next, bool & prev)
    {
      next = false;
      prev = false;

      if( sel_uid_is_current_uid() && StateData.Iter )
      {
        TreeModel::Children const& children = EpisodesData.Store->children();

        unsigned int position;
        position = EpisodesData.Store->get_path (StateData.Iter.get()).get_indices().data()[0];

        next = (position > 0);
        prev = (position+1 != children.size());

        if( next )
        {
          TreePath  path  = TreePath (TreePath::size_type (1), TreePath::value_type (position-1));
          TreeIter  iter  = EpisodesData.Store->get_iter (path);
          Episode   item  = ((*iter)[EpisodesData.ColumnRecord.item]);
          next = (!video_type (item.getAsString("enclosure-type")) || (video_type (item.getAsString("enclosure-type")) && item.getAsBool("downloaded")));
        }

        if( prev )
        {
          TreePath  path  = TreePath (TreePath::size_type (1), TreePath::value_type (position+1));
          TreeIter  iter  = EpisodesData.Store->get_iter (path);
          Episode   item  = ((*iter)[EpisodesData.ColumnRecord.item]);
          prev = (!video_type (item.getAsString("enclosure-type")) || (video_type (item.getAsString("enclosure-type")) && item.getAsBool("downloaded")));
        }
      }
    }

    void
    Podcasts::current_check_played_and_caps ()
    {
      Episode & item = mPodcastManager->podcast_fetch_item (StateData.Cast.podcast["source"], StateData.Item.getAsString("guid"));
      item.attributes["played"] = "1";
      StateData.Item = item; 
      (*StateData.Iter.get())[EpisodesData.ColumnRecord.item] = StateData.Item; 

      (*mContextMap.find( StateData.CastUid )->second)[CastData.ColumnRecord.status] = ST_DEFAULT;

      if( StateData.Item.getAsBool("downloaded") )
      {
        m_caps = Caps (m_caps | PlaybackSource::CAN_SEEK);
        m_caps = Caps (m_caps | PlaybackSource::CAN_PAUSE);
      }
      else
      {
        m_caps = Caps (m_caps & ~PlaybackSource::CAN_SEEK);
        m_caps = Caps (m_caps & ~PlaybackSource::CAN_PAUSE);
      }

      // check to fix cast title/remove marker
      if(StateData.Iter && StateData.Iter.get() == EpisodesData.Store->children().begin())
      {
	// FIXME: Account for filtering in the episodes store, NOT RELEVANT _YET_
        TreeIter iter = (*mContextMap.find( StateData.CastUid )->second);
        Podcast & cast = mPodcastManager->podcast_fetch ((*iter)[CastData.ColumnRecord.uri]);

        if( cast.item_map.empty() )
        {
          (*iter)[CastData.ColumnRecord.title] =
            ((podcast_title_f % Markup::escape_text (cast.podcast["title"]).c_str()
	      			  % "" 
                                  % _("(No Current Episodes)")).str());
        }
        else
        {
          (*iter)[CastData.ColumnRecord.title] = 
	    ((podcast_title_f % Markup::escape_text (cast.podcast["title"]).c_str()
				  % "" 
                                  % Markup::escape_text (get_timestr_from_time_t (cast.most_recent_item)).c_str()).str());
        }
      }

      bool next, prev;
      has_next_prev (next, prev);

      if( sel_uid_is_current_uid() && StateData.Iter )
        {
          if( next )
            m_caps = Caps (m_caps |  PlaybackSource::CAN_GO_NEXT);
          else
            m_caps = Caps (m_caps & ~PlaybackSource::CAN_GO_NEXT);

          if( prev )
            m_caps = Caps (m_caps |  PlaybackSource::CAN_GO_PREV);
          else
            m_caps = Caps (m_caps & ~PlaybackSource::CAN_GO_PREV);
        }
      else
        {
          m_caps = Caps (m_caps & ~PlaybackSource::CAN_GO_PREV);
          m_caps = Caps (m_caps & ~PlaybackSource::CAN_GO_NEXT);
        }

      m_caps = Caps (m_caps | PlaybackSource::CAN_PROVIDE_METADATA);
    }

    void
    Podcasts::go_next_prev (Direction direction)
    {
      TreePath path;
      TreeIter iter;
      unsigned int position = 0;

      switch (direction)
      {
        case NEXT:
          if( StateData.Iter )
          {
            path = EpisodesData.Store->get_path (StateData.Iter.get());
            iter = StateData.Iter.get();
            position = path.get_indices().data()[0];

            StateData.Iter.reset();
            EpisodesData.Store->row_changed (path, iter);

            path.prev ();

            StateData.Iter = EpisodesData.Store->get_iter (path);
            EpisodesData.Store->row_changed (path, StateData.Iter.get());

            current_assign (StateData.Iter.get());
          }
          break;

        case PREV:
          if( StateData.Iter )
          {
            path = EpisodesData.Store->get_path (StateData.Iter.get());
            iter = StateData.Iter.get();
            position = path.get_indices().data()[0];

            StateData.Iter.reset();
            EpisodesData.Store->row_changed (path, iter);

            path.next ();

            StateData.Iter = EpisodesData.Store->get_iter (path);
            EpisodesData.Store->row_changed (path, StateData.Iter.get());

            current_assign (StateData.Iter.get());
          }
          break;

        default: break;
      }
    }

    void
    Podcasts::current_clear ()
    {
      if( sel_uid_is_current_uid() && StateData.Iter )
      {
        TreePath path = EpisodesData.Store->get_path(StateData.Iter.get());
        TreeIter iter = StateData.Iter.get();
        EpisodesData.Store->row_changed (path, iter);
      }

      StateData = StateDataT();
      EpisodesData.View->queue_draw ();
    }

    void
    Podcasts::current_assign (TreeIter const& iter)
    {
      current_clear ();

      TreeIter iter_c = CastData.View->get_selection ()->get_selected();

      StateData.Cast  = mPodcastManager->podcast_fetch ((*iter_c)[CastData.ColumnRecord.uri]);
      StateData.Item  = ((*iter)[EpisodesData.ColumnRecord.item]);
      StateData.Image = Glib::RefPtr<Gdk::Pixbuf>(0);

      if( !StateData.Cast.podcast["image"].empty() )
      {
        try{
            StateData.Image = Gdk::Pixbuf::create_from_file (cast_image_filename (StateData.Cast)); 
          }
        catch (...) {}
      } 

      StateData.Iter = iter;
      StateData.CastUid = (*iter_c)[CastData.ColumnRecord.uid];
      StateData.ItemUid = StateData.Item.getAsString("guid");
    }
    
    void
    Podcasts::send_metadata ()
    {
      TrackMetadata metadata;

      metadata.artist       = StateData.Cast.podcast["title"]; 
      metadata.album        = StateData.Cast.podcast["description"];
      metadata.title        = StateData.Item.item["title"];
      metadata.duration     = StateData.Item.getAsInt("enclosure-length") / 1000;
      metadata.image        = StateData.Image;

      mSignalMetadata.emit (metadata);
    }

    bool
    Podcasts::go_next ()
    {
      go_next_prev  (NEXT);
      return true;
    }

    bool
    Podcasts::go_prev ()
    {
      go_next_prev  (PREV);
      return true;
    }

    void
    Podcasts::stop ()
    {
      if ( !EpisodesData.View->get_selection()->count_selected_rows())
      {
        m_caps = Caps (m_caps & ~PlaybackSource::CAN_PLAY);
      }

      current_clear ();

      m_caps = Caps (m_caps & ~PlaybackSource::CAN_GO_PREV);
      m_caps = Caps (m_caps & ~PlaybackSource::CAN_GO_NEXT);
      m_caps = Caps (m_caps & ~PlaybackSource::CAN_SEEK);
      m_caps = Caps (m_caps & ~PlaybackSource::CAN_PAUSE);
      m_caps = Caps (m_caps & ~PlaybackSource::CAN_PROVIDE_METADATA);
      mSignalCaps.emit (m_caps);
    }

    ustring
    Podcasts::get_uri ()
    {
      if( StateData.Item.getAsBool("downloaded") )
        return filename_to_uri (StateData.Item.getAsString("filename"));
      else
        return StateData.Item.getAsString("enclosure");
    }

    ustring
    Podcasts::get_type ()
    {
      return StateData.Item.getAsString("enclosure-type") ;
    }

    void
    Podcasts::buffering_done () 
    {
      send_metadata ();
    }

    bool
    Podcasts::play ()
    {
      if( StateData.Iter )
      {
          return true;
      }
      else
      {
        TreeIter iter = EpisodesData.View->get_selection()->get_selected();
        if( iter )
        {
          current_assign (iter);
          return true;
        }
      }

      return false;
    }

    void
    Podcasts::play_post ()
    {
      current_check_played_and_caps ();
      go_next_prev (CHECKONLY);

      if (StateData.Item.getAsBool("downloaded"))
        send_metadata (); // we won't get called on buffering_done() so now is the time
    }

    void
    Podcasts::next_post ()
    {
      current_check_played_and_caps ();
      send_metadata ();
    }

    void
    Podcasts::prev_post ()
    {
      current_check_played_and_caps ();
      send_metadata ();
    }

    void
    Podcasts::restore_context ()
    {
      if( StateData.CastUid )
      {
        CastData.View->get_selection()->select (mContextMap.find( StateData.CastUid )->second);
      }
    }

    ///////////////////////////////////////////////////////////////////////////////////////

    void
    Podcasts::items_on_row_activated (TreePath const& path, TreeViewColumn* column)
    {
      TreeIter iter = EpisodesData.Store->get_iter (path);
      TreeIter iter_c = CastData.View->get_selection ()->get_selected();

      Episode item ((*iter)[EpisodesData.ColumnRecord.item]);
      UID uid ((*iter_c)[CastData.ColumnRecord.uid]);
      DownloadKey key (uid, item.getAsString("guid"));

      if( DownloadData.Downloads.find (key) != DownloadData.Downloads.end() )
      {
        MessageDialog dialog (_("This item is still being downloaded. Please wait until the download is done to start playback."),
                               false, MESSAGE_INFO, BUTTONS_OK, true);
        dialog.run ();
        return;
      }

      if( (item.getAsBool("downloaded") || (!item.getAsBool("downloaded") && !video_type (item.getAsString("enclosure-type")) ) )
           && (!DownloadData.Downloads.count (key)))
      {
        current_assign (EpisodesData.Store->get_iter (path));
        mSignalPlayRequest.emit (); 
      }
      else
      if( ((!item.getAsBool("downloaded") && (video_type (item.getAsString("enclosure-type"))))) && (!DownloadData.Downloads.count (key)))
      {
        MessageDialog dialog (_("Video items have to be downloaded in order to play them. Do you want to download this item now?"),
                               false, MESSAGE_QUESTION, BUTTONS_YES_NO, true);
        if( dialog.run() == GTK_RESPONSE_YES )
        {
          items_on_column_download_toggled (path.to_string());
        }
      }
    }

    void
    Podcasts::casts_on_selection_changed()
    {
      m_actions->get_action (POD_ACTION_COPY_WEB_LINK)->set_sensitive (false);
      m_actions->get_action (POD_ACTION_COPY_RSS_LINK)->set_sensitive (false);

      bool selected = (CastData.View->get_selection ()->count_selected_rows() > 0);

      EpisodesData.View->queue_draw();
      StateData.Iter.reset();
      EpisodesData.Store->clear ();
      MiscWidgets.TextViewDescription->get_buffer()->set_text("");

      if( selected )
      {
        TreeIter iter = CastData.FilterStore->convert_iter_to_child_iter (CastData.View->get_selection()->get_selected());
        Podcast & cast = mPodcastManager->podcast_fetch ((*iter)[CastData.ColumnRecord.uri]);
        for (EpisodeMap::iterator i = cast.item_map.begin(); i != cast.item_map.end() ; ++i)
        {
          TreeIter iter = EpisodesData.Store->append ();
          (*iter)[EpisodesData.ColumnRecord.item] = i->second;

          if( sel_uid_is_current_uid() && (i->second.attributes["guid"] == StateData.ItemUid) )
          {
            StateData.Iter = iter;
            EpisodesData.View->scroll_to_row(EpisodesData.Store->get_path(iter), 0.5);
          }
        }
          
        m_actions->get_action (POD_ACTION_COPY_WEB_LINK)->set_sensitive (!cast.podcast["link"].empty());
        m_actions->get_action (POD_ACTION_COPY_RSS_LINK)->set_sensitive (true);
      }
      else
      {
        EpisodesData.Store->clear ();
      }

      m_actions->get_action (POD_ACTION_DEL_PODCAST)->set_sensitive (selected);
      m_actions->get_action (POD_ACTION_UPD_PODCAST)->set_sensitive (selected);
      EpisodesData.View->columns_autosize();
      go_next_prev (CHECKONLY);
    }

    void
    Podcasts::items_on_selection_changed()
    {
      if( (EpisodesData.View->get_selection ()->count_selected_rows() > 0) )
      {
        TreeIter iter = EpisodesData.View->get_selection()->get_selected();
        Episode item ((*iter)[EpisodesData.ColumnRecord.item]);

        TreeIter iter_c = CastData.View->get_selection ()->get_selected();
        UID uid ((*iter_c)[CastData.ColumnRecord.uid]);
    
        DownloadKey key (uid, item.getAsString("guid")); 

        MiscWidgets.TextViewDescription->get_buffer()->set_text (Util::sanitize_html (item.getAsString("description")));

        if(    ( item.getAsBool("downloaded") 
            || (!item.getAsBool("downloaded") && !video_type (item.getAsString("enclosure-type"))))
            && (!DownloadData.Downloads.count (key)))
        {
          m_caps = Caps (m_caps | PlaybackSource::CAN_PLAY);
        }
        else
        {
          m_caps = Caps (m_caps & ~PlaybackSource::CAN_PLAY);
        }
      }
      else
      {
        MiscWidgets.TextViewDescription->get_buffer()->set_text("");
        m_caps = Caps (m_caps & ~PlaybackSource::CAN_PLAY);
      }
      mSignalCaps.emit (m_caps);
    }
  } // namespace UiPart
} // namespace Bmp
