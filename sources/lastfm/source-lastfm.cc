//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
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
//  The MPXx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <gtkmm.h>
#include <gtk/gtk.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <libglademm.h>
#include <map>
#include <cmath>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

#include "mpx/mpx-public.hh"
#include "mpx/play-public.hh"

#include "mpx/library.hh"
#include "mpx/main.hh"
#include "mpx/minisoup.hh"
#include "mpx/types.hh"
#include "mpx/uri.hh"
#include "mpx/util-ui.hh"
#include "mpx/xmltoc++.hh"

#include "source-lastfm.hh"
#include "lastfm-extra-widgets.hh"
#include "xsd-track-toptags.hxx"

#include "widgets/cell-renderer-cairo-surface.hh"
#include "mpx/util-graphics.hh"

#define STATE(e) ((m_state & e) != 0)
#define SET_STATE(e) ((m_state |= e))
#define CLEAR_STATE(e) ((m_state &= ~ e))

using namespace Glib;
using namespace Gtk;

namespace
{
    boost::format f1 ("lastfm://artist/%s/similarartists");
    boost::format f2 ("lastfm://globaltags/%s");
    boost::format f3 ("lastfm://user/%s/neighbours");

    class TagInserter
    {
      public:

        TagInserter (/*MPX::LastFMTagView & view*/ MPX::TagView & view)
        : m_View(view)
        {
        }

        void
        operator()(::tag const& t)
        {
/*            m_View.insert_tag( t.name(), t.url(), t.count() ); */
            m_View.add_tag( t.name(), std::log10(t.count()*2) );
        }

      private:

        // MPX::LastFMTagView & m_View;
        MPX::TagView & m_View;
    };
}

namespace MPX
{
namespace Source
{
    Glib::RefPtr<Gdk::Pixbuf>
    LastFM::get_icon ()
    {
        try{
            return IconTheme::get_default()->load_icon("source-lastfm", 64, ICON_LOOKUP_NO_SVG);
        } catch(...) { return Glib::RefPtr<Gdk::Pixbuf>(0); }
    }

    Gtk::Widget*
    LastFM::get_ui ()
    {
        return m_UI;
    }

    LastFM::LastFM (const Glib::RefPtr<Gtk::UIManager>& ui_manager, MPX::Player & player)
    : PlaybackSource(ui_manager, _("LastFM"), C_CAN_PROVIDE_METADATA, F_ASYNC)
    , m_Player(player)
    {
		m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_PAUSE);
		m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_GO_PREV);

		const std::string path (build_filename(DATA_DIR, build_filename("glade","source-lastfm.glade")));
		m_ref_xml = Gnome::Glade::Xml::create (path);

		m_UI = m_ref_xml->get_widget("source-lastfm");
        m_ref_xml->get_widget("cbox-sel", m_CBox_Sel);
        m_ref_xml->get_widget("url-entry", m_URL_Entry);
        m_ref_xml->get_widget("hbox-error", m_HBox_Error);
        m_ref_xml->get_widget("label-error", m_Label_Error);
        m_ref_xml->get_widget("button-error-hide", m_Button_Error_Hide);
        m_ref_xml->get_widget_derived("lastfm-tag-info-view", m_TagInfoView);
        m_TagView = new TagView(m_ref_xml, "tag-area");
        (dynamic_cast<Gtk::Image*>(m_ref_xml->get_widget("lastfm-artists-throbber")))->set(build_filename(DATA_DIR, "images" G_DIR_SEPARATOR_S "throbber.gif"));

        m_Button_Error_Hide->signal_clicked().connect( sigc::mem_fun( *m_HBox_Error, &Gtk::Widget::hide ));

        m_CBox_Sel->set_active(0);
        m_URL_Entry->signal_activate().connect( sigc::mem_fun( *this, &LastFM::on_url_entry_activated ));

        m_LastFMRadio.handshake();
        if(m_LastFMRadio.connected())
    		m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_PLAY);

        m_LastFMRadio.signal_playlist().connect( sigc::mem_fun( *this, &LastFM::on_playlist ));
        m_LastFMRadio.signal_no_playlist().connect( sigc::mem_fun( *this, &LastFM::on_no_playlist ));
        m_LastFMRadio.signal_tuned().connect( sigc::mem_fun( *this, &LastFM::on_tuned ));
        m_LastFMRadio.signal_tune_error().connect( sigc::mem_fun( *this, &LastFM::on_tune_error ));

        mcs->key_register("lastfm", "discoverymode", true);
    }

    //////

    void
    LastFM::on_url_entry_activated()
    {
        int c = m_CBox_Sel->get_active();

        switch(c)
        {
            case 0:
                m_LastFMRadio.playurl((f1 % m_URL_Entry->get_text()).str());
                break;

            case 1:
                m_LastFMRadio.playurl((f2 % m_URL_Entry->get_text()).str());
                break;

            case 2:
                m_LastFMRadio.playurl((f3 % m_URL_Entry->get_text()).str());
                break;
        }

        m_URL_Entry->set_text("");
    }

    void
    LastFM::on_playlist (XSPF::Playlist const& plist)
    {
        PAccess<MPX::Play> pa;
        m_Player.get_object(pa);
        pa.get().set_custom_httpheader((boost::format ("Cookie: Session=%s") % m_LastFMRadio.session().SessionId).str().c_str());

        bool next = (m_Playlist && (m_PlaylistIter == m_Playlist.get().Items.end()));

        m_Playlist = plist;
        m_PlaylistIter = m_Playlist.get().Items.begin();

        if(next)
            Signals.NextAsync.emit();
        else
            Signals.PlayAsync.emit();
    }

    void
    LastFM::on_no_playlist ()
    {
        Signals.StopRequest.emit();
    }

    void
    LastFM::on_tuned ()
    {
        m_LastFMRadio.get_xspf_playlist();
    }

    void
    LastFM::on_tune_error (Glib::ustring const& error)
    {
        m_Label_Error->set_text(error);
        m_HBox_Error->show();
    }

    ////// 

    std::string
    LastFM::get_uri ()
    {
        g_return_val_if_fail(m_Playlist, std::string());
        g_return_val_if_fail(m_PlaylistIter != m_Playlist.get().Items.end(), std::string());

        XSPF::Item const& item = *m_PlaylistIter;
        URI u (item.location);
        std::string location = "http://play.last.fm" + u.fullpath();
        return location; 
    }

    void
    LastFM::go_next_async ()
    {
        m_PlaylistIter++;

        if( m_PlaylistIter == m_Playlist.get().Items.end() )
        {
            m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_GO_NEXT);
            send_caps();
            m_LastFMRadio.get_xspf_playlist ();
        } 
        else
        {
            Signals.NextAsync.emit();
        }
    }

    void
    LastFM::go_prev_async ()
    {
    }

    void
    LastFM::play_async ()
    {
        m_Playlist.reset();
        m_LastFMRadio.get_xspf_playlist();
    }

    bool
    LastFM::go_next ()
    {
        m_PlaylistIter++;
		return true;
    }

    bool
    LastFM::go_prev ()
    {
		return false;
    }

    bool
    LastFM::play ()
    {
        return false;
    }

    void
    LastFM::stop ()
    {
        m_LastFMRadio.get_xspf_playlist_cancel();
        PAccess<MPX::Play> pa;
        m_Player.get_object(pa);
        pa.get().set_custom_httpheader(NULL);
        m_TagView->clear();
    }

	void
	LastFM::send_metadata ()
	{
	}

    void
    LastFM::next_post ()
    {
        play_post ();
    }

    void
    LastFM::play_post ()
    {
        g_return_if_fail (m_Playlist);
        g_return_if_fail (m_PlaylistIter != m_Playlist.get().Items.end());

        XSPF::Item const& item = *m_PlaylistIter;

        Metadata metadata;
        metadata[ATTRIBUTE_ARTIST] = item.creator;
        metadata[ATTRIBUTE_ALBUM] = item.album;
        metadata[ATTRIBUTE_TITLE] = item.title;
        metadata[ATTRIBUTE_TIME] = gint64(item.duration);
        metadata[ATTRIBUTE_GENRE] = "Last.fm: " + m_Playlist.get().Title; 
        metadata.Image = Util::get_image_from_uri (item.image);
        Signals.Metadata.emit (metadata);

        if( m_PlaylistIter != m_Playlist.get().Items.end() )
          m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_GO_NEXT);
        else
          m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_GO_NEXT);

        m_TagView->clear ();

        try{
            URI u ((boost::format ("http://ws.audioscrobbler.com/1.0/track/%s/%s/toptags.xml") % item.creator % item.title).str(), true);    
            MPX::XmlInstance< ::toptags> tags ((ustring(u)));
            if(tags.xml().tag().size())
            {
                std::random_shuffle( tags.xml().tag().begin(), tags.xml().tag().end()-1 );
                std::iter_swap( tags.xml().tag().begin()+tags.xml().tag().size()/2, tags.xml().tag().end()-1);
                std::for_each(tags.xml().tag().begin(), tags.xml().tag().end(), TagInserter(*m_TagView));
            }
        } catch (std::runtime_error & cxe)
        {
            g_message("%s: Error: %s", G_STRLOC, cxe.what());
        }
    }

    void
    LastFM::restore_context ()
    {
    }

  } // Source

    ////////////////////////////////////////////////////////////////////////

    TagInfoViewT::TagInfoViewT (BaseObjectType                 * obj,
                                        RefPtr<Gnome::Glade::Xml> const& xml)
    : TreeView    (obj)
    , mEndDisplay (false)
    {
      Glib::RefPtr<Gdk::Pixbuf> image = Gdk::Pixbuf::create_from_file (build_filename( DATA_DIR, "images" G_DIR_SEPARATOR_S "artist-unknown.png" ));
      mUnknownArtist = Util::cairo_image_surface_from_pixbuf (image);

      xml->get_widget ("lastfm-artists-notebook", mNotebook);
      xml->get_widget ("lastfm-tag-entry", mEntry);
      xml->get_widget ("lastfm-cbox-tag-match", mCbox);

      signal_event().connect( sigc::mem_fun( *this, &TagInfoViewT::on_event_cb ));

      mCbox->set_active( 0 );
      mCbox->signal_changed().connect( sigc::compose( sigc::mem_fun( *this, &TagInfoViewT::idSetAuto),
                                                      sigc::mem_fun( *mEntry, &Gtk::Entry::get_text )));

      mEntry->signal_changed().connect( sigc::mem_fun( *this, &TagInfoViewT::clear ) );
      mEntry->signal_activate().connect( sigc::compose( sigc::mem_fun( *this, &TagInfoViewT::idSetAuto),
                                                        sigc::mem_fun( *mEntry, &Gtk::Entry::get_text )));
      mStore = ListStore::create (mColumns);

      TreeViewColumn *column = 0; 
      CellRendererCairoSurface *cell1 = 0; 
      CellRendererText *cell2 = 0; 

      column = manage (new TreeViewColumn ());
      column->set_resizable (false);
      column->set_expand (false);

      cell1 = manage (new CellRendererCairoSurface ());
      cell1->property_xpad() = 8;
      cell1->property_ypad() = 4;
      cell1->property_yalign() = 0.;
      column->pack_start (*cell1, false);
      column->set_cell_data_func (*cell1, sigc::bind( sigc::mem_fun( *this, &TagInfoViewT::cellDataFunc ), 0));
       
      cell2 = manage (new CellRendererText ());
      cell2->property_ellipsize() = Pango::ELLIPSIZE_END;
      cell2->property_xpad() = 4;
      cell2->property_yalign() = 0.;
      cell2->property_height() = 58;
      column->pack_start (*cell2, true);
      column->set_cell_data_func (*cell2, sigc::bind( sigc::mem_fun( *this, &TagInfoViewT::cellDataFunc ), 1));

      append_column (*column);

      get_selection()->set_mode (SELECTION_BROWSE);
      get_selection()->signal_changed().connect( sigc::mem_fun( *this, &TagInfoViewT::on_selection_changed ) );
      set_model (mStore);
    }

    bool
    TagInfoViewT::isEmpty ()
    {
        return mEntry->get_text().empty();
    }

    void
    TagInfoViewT::set_ui_manager (RefPtr<Gtk::UIManager> const &ui_manager)
    {
#if 0
      m_ui_manager = ui_manager;
      m_actions = Gtk::ActionGroup::create ("Actions_UiPartLASTFM-TagArtistsView");
      m_actions->add  (Gtk::Action::create ("lastfm-action-tav-view-in-library",
                                            _("View in Library")),
                                            sigc::mem_fun (*this, &TagInfoViewT::on_view_in_library));
      m_actions->add  (Gtk::Action::create ("lastfm-action-tav-play-lastfm",
                                            Gtk::StockID (BMP_STOCK_LASTFM),
                                            _("Play on Last.fm")),
                                            sigc::mem_fun (*this, &TagInfoViewT::on_play_lastfm));
      m_ui_manager->insert_action_group (m_actions);
      m_ui_manager->add_ui_from_string (ui_string_tav);
#endif
    }

    bool
    TagInfoViewT::on_event_cb (GdkEvent * ev)
    {
#if 0
      if( ev->type == GDK_BUTTON_PRESS )
      {
        GdkEventButton * event = reinterpret_cast <GdkEventButton *> (ev);
        if( event->button == 3 )
        {
          Gtk::Menu * menu = dynamic_cast < Gtk::Menu* > (Util::get_popup (m_ui_manager, "/popup-lastfm-tav/menu-lastfm-tav"));
          if (menu) // better safe than screwed
          {
            menu->popup (event->button, event->time);
          }
        }
      }
#endif
      return false;
    }

    void
    TagInfoViewT::on_selection_changed ()
    {
#if 0 
      if( get_selection()->count_selected_rows() )
      {
        TreeIter iter = get_selection()->get_selected();
        bool hasAlbums = ((*iter)[mColumns.hasAlbums]);
        m_actions->get_action("lastfm-action-tav-view-in-library")->set_sensitive( hasAlbums );
      }
#endif
    }

    void
    TagInfoViewT::on_view_in_library ()
    {
#if 0
        TreeIter iter = get_selection()->get_selected();
        LastFMArtist a = ((*iter)[mColumns.artist]);
        Signals.GoToMBID.emit( a.mbid );
#endif
    }

    void
    TagInfoViewT::on_play_lastfm ()
    {
#if 0
        TreeIter iter = get_selection()->get_selected();
        LastFMArtist a = ((*iter)[mColumns.artist]);
        Signals.ArtistActivated.emit((f_artist % a.name.c_str()).str());
#endif
    }

    void
    TagInfoViewT::clear ()
    {
      mDisplayLock.lock ();
      mEndDisplay = true;
      mDisplayLock.unlock ();
      mProcessIdler.disconnect ();
      mStore->clear ();
      mEndDisplay = false;
      mNotebook->set_current_page( 0 );
      //m_actions->get_action("lastfm-action-tav-view-in-library")->set_sensitive( false );
    }

    bool
    TagInfoViewT::idler ()
    {
      Glib::Mutex::Lock L (mDisplayLock);
      if( mEndDisplay )
        return false;

      if( mIterator->streamable )
      {
        if( !mIterator->thumbnail.empty())
        {
              Glib::RefPtr<Gdk::Pixbuf> image = Util::get_image_from_uri (mIterator->thumbnail);
              if( image )
              {
                    mImage = Util::cairo_image_surface_from_pixbuf (image);
                    Util::cairo_image_surface_border (mImage, 1.);
                    (*mTreeIterator)[mColumns.image] = mImage;
              }
        }

        LastFMArtist a = ((*mTreeIterator)[mColumns.artist]);

#if 0
        if( !a.mbid.empty() )
        {
              DB::RowV v; 
              Library::Obj()->get (v, (boost::format ("SELECT count(*) AS cnt FROM album JOIN "
                                                      "album_artist ON album.album_artist_j = album_artist.id WHERE "
                                                      "mb_album_artist_id ='%s'") % a.mbid).str(), false);  
              guint64 count = boost::get<guint64>(v[0].find("cnt")->second);
              if( count > 0 )
              {
                    (*mTreeIterator)[mColumns.hasAlbums] = true;
        
                    if( count > 1 )
                      (*mTreeIterator)[mColumns.info] = (boost::format ("<small><b>%llu</b> %s</small>") % count % _("Albums in Library")).str();
                    else
                      (*mTreeIterator)[mColumns.info] = (boost::format ("<small><b>%llu</b> %s</small>") % count % _("Album in Library")).str();
              }
              else
              {
                    (*mTreeIterator)[mColumns.hasAlbums] = false;
              }
        }
#endif

        ++mTreeIterator;
      }

      return (++mIterator != mArtists.end());
    }

    void  
    TagInfoViewT::data_cb (char const * data, guint size, guint code)
    {
      if( code == 200 )
      {
        std::string chunk;
        chunk.append( data, size );

        mArtists = LastFMArtistV();

        try{
            ArtistParser p (mArtists);
            Markup::ParseContext context (p);
            context.parse (chunk);
            context.end_parse ();
          }
        catch (Glib::MarkupError & cxe)
          {
            g_message("%s: Markup Error: '%s'", G_STRFUNC, cxe.what().c_str());
            return;
          }
        catch (Glib::ConvertError & cxe)
          {
            g_message("%s: Conversion Error: '%s'", G_STRFUNC, cxe.what().c_str());
            return;
          }

        if( mArtists.empty () )
        {
          g_message("%s: No Artists Parsed", G_STRFUNC); 
          mNotebook->set_current_page( 0 );
          return;
        }

        mIterator = mArtists.begin();
        mTopRank = mIterator->count;

        for(LastFMArtistV::const_iterator i = mArtists.begin(); i != mArtists.end(); ++i)
        {
          if( i->streamable )
          { 
            TreeIter iter = mStore->append ();
            (*iter)[mColumns.artist] = *i;
            (*iter)[mColumns.image] = mUnknownArtist;
            (*iter)[mColumns.hasAlbums] = false;
          }
        }

        mNotebook->set_current_page( 0 );
        mTreeIterator = mStore->children().begin();
        mProcessIdler = signal_idle().connect( sigc::mem_fun( *this, &TagInfoViewT::idler ) );
      }
      else
        g_message("%s: HTTP Response Code is not 200 OK", G_STRFUNC); 
    }

    void
    TagInfoViewT::idSetAuto (ustring const& id)
    {
      idSet( ID_AUTO, id );
    }

    void
    TagInfoViewT::idSet (IdType type, ustring const& id)
    {
      if( mEntry->get_text() != id )
        mEntry->set_text( id ); 
      else
        clear ();

      if( mEntry->get_text().empty() ) 
        return;

      mNotebook->set_current_page( 1 );
      std::string uri; 

      int row = mCbox->get_active_row_number();
      if( (row != int( type )) && ( type != ID_AUTO ))
      {
        mCbox->set_active( int( type ) );
      }

      switch( row ) 
      {
        case 0:
          uri = (boost::format ("http://ws.audioscrobbler.com/1.0/tag/%s/topartists.xml") % URI::escape_string(id).c_str()).str();
          break;
  
        case 1:
          uri = (boost::format ("http://ws.audioscrobbler.com/1.0/artist/%s/similar.xml") % URI::escape_string(id).c_str()).str();
          break;
      } 

      mArtistReq = Soup::Request::create (uri);
      mArtistReq->request_callback().connect (sigc::mem_fun (*this, &TagInfoViewT::data_cb));
      mArtistReq->run ();
    }

    void
    TagInfoViewT::cellDataFunc (CellRenderer * baseCell, TreeModel::iterator const& i, int column)
    {
      CellRendererCairoSurface * pix = 0; 
      CellRendererText * text = 0; 

      switch( column )
      {
        case 0:
          pix = dynamic_cast<CellRendererCairoSurface*>(baseCell);
          pix->property_surface() = (*i)[mColumns.image];
          return;
      
        case 1:
          text = dynamic_cast<CellRendererText*>(baseCell);
          LastFMArtist a = ((*i)[mColumns.artist]);
          ustring info = ((*i)[mColumns.info]);
          text->property_markup() = (boost::format ("<b>%s</b>\n%.2f %%\n%s")
                                        % Markup::escape_text (a.name).c_str()
                                        % (100. * (double (a.count) / double (mTopRank))) 
                                        % info).str(); // info goes in raw, is escaped above
          return;
      }
    }

    void
    ArtistParser::on_start_element  (Glib::Markup::ParseContext & context,
                                     Glib::ustring const& element_name,
                                     AttributeMap const& attributes)
    {
      if (element_name == "artist")
      {
        SET_STATE(E_ARTIST);

        m_current = LastFMArtist();

        if (attributes.find ("name") != attributes.end())
        {
          m_current.name = attributes.find ("name")->second;
        }

        if (attributes.find ("count") != attributes.end())
        {
          m_current.count = g_ascii_strtoull (attributes.find ("count")->second.c_str(), NULL, 10); 
        }

        m_current.streamable = (attributes.find ("streamable") != attributes.end()) && (
                          (attributes.find("streamable")->second == "yes") || (attributes.find("streamable")->second == "1"));
      }
      else
      if (element_name == "name")
      {
        SET_STATE(E_NAME);
      }
      else
      if (element_name == "mbid")
      {
        SET_STATE(E_MBID);
      }
      else
      if (element_name == "playcount")
      {
        SET_STATE(E_COUNT);
      }
      else
      if (element_name == "rank")
      {
        SET_STATE(E_RANK);
      }
      else
      if (element_name == "url")
      {
        SET_STATE(E_URL);
      }
      else
      if (element_name == "thumbnail")
      {
        SET_STATE(E_THUMBNAIL);
      }
      else
      if (element_name == "image")
      {
        SET_STATE(E_IMAGE);
      }
      else
      if (element_name == "image_small")
      {
        SET_STATE(E_IMAGE_SMALL);
      }
      else
      if (element_name == "streamable")
      {
        SET_STATE(E_STREAMABLE);
      }
      else
      if (element_name == "match")
      {
        SET_STATE(E_MATCH);
      }
    }

    void
    ArtistParser::on_end_element    (Glib::Markup::ParseContext& context,
                                     Glib::ustring const& element_name)
    {
      if (element_name == "artist")
      {
        CLEAR_STATE(E_ARTIST);
        m_artists.push_back (m_current);
      }
      else
      if (element_name == "name")
      {
        CLEAR_STATE(E_NAME);
      }
      else
      if (element_name == "mbid")
      {
        CLEAR_STATE(E_MBID);
      }
      else
      if (element_name == "playcount")
      {
        CLEAR_STATE(E_COUNT);
      }
      else
      if (element_name == "rank")
      {
        CLEAR_STATE(E_RANK);
      }
      else
      if (element_name == "url")
      {
        CLEAR_STATE(E_URL);
      }
      else
      if (element_name == "thumbnail")
      {
        CLEAR_STATE(E_THUMBNAIL);
      }
      else
      if (element_name == "image")
      {
        CLEAR_STATE(E_IMAGE);
      }
      else
      if (element_name == "image_small")
      {
        CLEAR_STATE(E_IMAGE_SMALL);
      }
      else
      if (element_name == "streamable")
      {
        CLEAR_STATE(E_STREAMABLE);
      }
      else
      if (element_name == "match")
      {
        CLEAR_STATE(E_MATCH);
      }
    }

    void
    ArtistParser::on_text  (Glib::Markup::ParseContext & context,
                            Glib::ustring const& text)
    {
      if (STATE(E_ARTIST))
      {
        if (STATE(E_NAME))
        {
          m_current.name = text;
        }
        else
        if (STATE(E_MBID))
        {
          m_current.mbid = text;
        }
        else
        if (STATE(E_COUNT))
        {
          m_current.count = g_ascii_strtoull (text.c_str(), NULL, 10); 
        }
        else
        if (STATE(E_RANK))
        {
          m_current.rank = g_ascii_strtoull (text.c_str(), NULL, 10); 
        }
        else
        if (STATE(E_URL))
        {
          m_current.url = text;
        }
        else
        if (STATE(E_THUMBNAIL))
        {
          m_current.thumbnail = text;
        }
        else
        if (STATE(E_IMAGE))
        {
          m_current.image = text;
        }
        else
        if (STATE(E_IMAGE_SMALL))
        {
          m_current.thumbnail = text;
        }
        else
        if (STATE(E_STREAMABLE))
        {
          m_current.streamable = bool (g_ascii_strtoull (text.c_str(), NULL, 10));
        }
        else
        if (STATE(E_MATCH))
        {
          m_current.count = 100 * strtod (text.c_str(), NULL);
        }
      }
    }
} // MPX
