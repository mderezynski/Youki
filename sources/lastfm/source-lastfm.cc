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
#include "mpx/stock.hh"
#include "mpx/types.hh"
#include "mpx/uri.hh"
#include "mpx/util-ui.hh"
#include "mpx/xmltoc++.hh"

#include "source-lastfm.hh"
#include "lastfm-extra-widgets.hh"
#include "xsd-track-toptags.hxx"

#include "mpx/widgets/cell-renderer-cairo-surface.hh"
#include "mpx/util-graphics.hh"

#include "component-top-albums.hh"
#include "component-user-top-albums.hh"

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
            m_View.add_tag( t.name(), std::log10(t.count()*5) );
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
    : PlaybackSource(ui_manager, _("LastFM"), C_CAN_PROVIDE_METADATA | C_CAN_PLAY, F_ASYNC)
    , m_Player(player)
    {
        mcs->key_register("lastfm", "discoverymode", true);

        m_LastFMRadio.handshake();

        m_LastFMRadio.signal_playlist().connect( sigc::mem_fun( *this, &LastFM::on_playlist ));
        m_LastFMRadio.signal_no_playlist().connect( sigc::mem_fun( *this, &LastFM::on_no_playlist ));
        m_LastFMRadio.signal_tuned().connect( sigc::mem_fun( *this, &LastFM::on_tuned ));
        m_LastFMRadio.signal_tune_error().connect( sigc::mem_fun( *this, &LastFM::on_tune_error ));

		const std::string path (build_filename(DATA_DIR, build_filename("glade","source-lastfm.glade")));
		m_ref_xml = Gnome::Glade::Xml::create (path);

		m_UI = m_ref_xml->get_widget("source-lastfm");

        m_ref_xml->get_widget("url_entry", m_URL_Entry);
        m_ref_xml->get_widget("error_hbox", m_HBox_Error);
        m_ref_xml->get_widget("error_label", m_Label_Error);
        m_ref_xml->get_widget("error_hide_button", m_Button_Error_Hide);
        m_ref_xml->get_widget("station_type_cbox", m_CBox_Sel);

        m_CBox_Sel->set_active(0);

        m_Button_Error_Hide->signal_clicked().connect(
            sigc::mem_fun( *m_HBox_Error, &Gtk::Widget::hide
        ));

        m_URL_Entry->signal_activate().connect(
            sigc::mem_fun( *this, &LastFM::on_url_entry_activated
        ));

        m_ui_manager = Gtk::UIManager::create();
        m_actions = Gtk::ActionGroup::create ("Actions_UiPartLASTFM");
        m_actions->add (Gtk::Action::create ("dummy", "dummy"));
        m_ui_manager->insert_action_group (m_actions);

        m_Dock = gdl_dock_new ();
        gtk_widget_show(m_Dock);
        dynamic_cast<Alignment*>(m_ref_xml->get_widget("alignment_dock"))->add(*Glib::wrap(m_Dock,false));

        ComponentTopAlbums * c = new ComponentTopAlbums;
        ComponentUserTopAlbums * c2 = new ComponentUserTopAlbums(&player);

        GtkWidget * item = gdl_dock_item_new_with_stock(
            "TopAlbums",
            "Top Albums",
            NULL,
            GdlDockItemBehavior(GDL_DOCK_ITEM_BEH_CANT_CLOSE | GDL_DOCK_ITEM_BEH_CANT_ICONIFY)
        );

        gtk_widget_reparent(c->get_UI().gobj(), item);
        gdl_dock_add_item( GDL_DOCK( m_Dock ), GDL_DOCK_ITEM( item ), GDL_DOCK_CENTER );
        gtk_widget_show (item);

        item = gdl_dock_item_new_with_stock(
            "TopUserAlbums",
            "Top User Albums",
            NULL,
            GdlDockItemBehavior(GDL_DOCK_ITEM_BEH_CANT_CLOSE | GDL_DOCK_ITEM_BEH_CANT_ICONIFY)
        );

        gtk_widget_reparent(c2->get_UI().gobj(), item);
        gdl_dock_add_item( GDL_DOCK( m_Dock ), GDL_DOCK_ITEM( item ), GDL_DOCK_CENTER );
        gtk_widget_show (item);
    }

    std::string
    LastFM::get_guid ()
    {
        return "dafcb8ac-d9ff-4405-8a35-861ccdff66b5";
    }

    void
    LastFM::on_artist_activated (Glib::ustring const& artist_url)
    {
        m_LastFMRadio.playurl(artist_url);
    }

    void
    LastFM::on_url_entry_activated()
    {
        int c = m_CBox_Sel->get_active();
        URI u;

        switch(c)
        {
            case 0:
                u = URI ((f1 % m_URL_Entry->get_text()).str());
                break;

            case 1:
                u = URI ((f2 % m_URL_Entry->get_text()).str());
                break;

            case 2:
                u = URI ((f3 % m_URL_Entry->get_text()).str());
                break;
        }

        m_LastFMRadio.playurl((ustring(u)));
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
        m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_GO_NEXT);
        send_caps();
        m_LastFMRadio.get_xspf_playlist_cancel();
        PAccess<MPX::Play> pa;
        m_Player.get_object(pa);
        pa.get().set_custom_httpheader(NULL);
        m_Playlist.reset();
        m_PlaylistIter = m_Playlist.get().Items.begin();
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
    }

    void
    LastFM::restore_context ()
    {
    }

    UriSchemes 
    LastFM::get_uri_schemes ()
    {
        UriSchemes s;
        s.push_back("lastfm");
        return s;
    }

    void    
    LastFM::process_uri_list (Util::FileList const& uris, bool play)
    {
        g_return_if_fail (!uris.empty());
        if(play)
            m_LastFMRadio.playurl(uris[0]);
        else
            g_message("%s: Non-play processing of URIs is not supported", G_STRLOC);
    }
  } 

#if 0
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

    signal_event().connect( sigc::mem_fun( *this, &TagInfoViewT::on_event ));

    mCbox->set_active( 0 );
    mCbox->signal_changed().connect( sigc::compose( sigc::mem_fun( *this, &TagInfoViewT::idSetAuto),
                                                    sigc::mem_fun( *mEntry, &Gtk::Entry::get_text )));

    mEntry->signal_changed().connect( sigc::mem_fun( *this, &TagInfoViewT::clear ) );
    mEntry->signal_activate().connect( sigc::compose( sigc::mem_fun( *this, &TagInfoViewT::idSetAuto),
                                                      sigc::mem_fun( *mEntry, &Gtk::Entry::get_text )));
    ListStore = ListStore::create (Columns);

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
    column->set_cell_data_func (*cell1, sigc::bind( sigc::mem_fun( *this, &TagInfoViewT::cell_data_func ), 0));
     
    cell2 = manage (new CellRendererText ());
    cell2->property_ellipsize() = Pango::ELLIPSIZE_END;
    cell2->property_xpad() = 4;
    cell2->property_yalign() = 0.;
    cell2->property_height() = 58;
    column->pack_start (*cell2, true);
    column->set_cell_data_func (*cell2, sigc::bind( sigc::mem_fun( *this, &TagInfoViewT::cell_data_func ), 1));

    append_column (*column);

    get_selection()->set_mode (SELECTION_BROWSE);

    set_model (ListStore);
  }

  bool
  TagInfoViewT::isEmpty ()
  {
      return mEntry->get_text().empty();
  }

  void
  TagInfoViewT::on_row_activated (Gtk::TreePath const& path, Gtk::TreeViewColumn* G_GNUC_UNUSED)
  {
      TreeIter iter =  ListStore->get_iter(path);
      XmlArtist a = ((*iter)[Columns.Artist]);
      Signals.ArtistActivated.emit((f1 % a.name.c_str()).str());
  }

  void
  TagInfoViewT::clear ()
  {
    mDisplayLock.lock ();
    mEndDisplay = true;
    mDisplayLock.unlock ();
    mProcessIdler.disconnect ();
    ListStore->clear ();
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
                  (*mTreeIterator)[Columns.Image] = mImage;
            }
      }

      XmlArtist a = ((*mTreeIterator)[Columns.Artist]);

      ++mTreeIterator;
    }

    return (++mIterator != mXmlArtists.end());
  }

  void  
  TagInfoViewT::data_cb (char const * data, guint size, guint code)
  {
    if( code == 200 )
    {
      std::string chunk;
      chunk.append( data, size );

      mXmlArtists = XmlArtistV();

      try{
          XmlArtistParser p (mXmlArtists);
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

      if( mXmlArtists.empty () )
      {
        g_message("%s: No XmlArtists Parsed", G_STRFUNC); 
        mNotebook->set_current_page( 0 );
        return;
      }

      mIterator = mXmlArtists.begin();
      mTopRank = mIterator->count;

      for(XmlArtistV::const_iterator i = mXmlArtists.begin(); i != mXmlArtists.end(); ++i)
      {
        if( i->streamable )
        { 
          TreeIter iter = ListStore->append ();
          (*iter)[Columns.Artist] = *i;
          (*iter)[Columns.Image] = mUnknownArtist;
        }
      }

      mNotebook->set_current_page( 0 );
      mTreeIterator = ListStore->children().begin();
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

    mXmlArtistReq = Soup::Request::create (uri);
    mXmlArtistReq->request_callback().connect (sigc::mem_fun (*this, &TagInfoViewT::data_cb));
    mXmlArtistReq->run ();
  }

  void
  TagInfoViewT::cell_data_func (CellRenderer * baseCell, TreeModel::iterator const& i, int column)
  {
    CellRendererCairoSurface * pix = 0; 
    CellRendererText * text = 0; 

    switch( column )
    {
      case 0:
        pix = dynamic_cast<CellRendererCairoSurface*>(baseCell);
        pix->property_surface() = (*i)[Columns.Image];
        return;
    
      case 1:
        text = dynamic_cast<CellRendererText*>(baseCell);
        XmlArtist a = ((*i)[Columns.Artist]);
        ustring info = ((*i)[Columns.Info]);
        text->property_markup() = (boost::format ("<big><b>%s</b></big>\n%.2f %%\n%s")
                                      % Markup::escape_text (a.name).c_str()
                                      % (100. * (double (a.count) / double (mTopRank))) 
                                      % info).str(); // info goes in raw, is escaped above
        return;
    }
  }

  void
  XmlArtistParser::on_start_element  (Glib::Markup::ParseContext & context,
                                   Glib::ustring const& element_name,
                                   AttributeMap const& attributes)
  {
    if (element_name == "artist")
    {
      SET_STATE(E_ARTIST);

      m_current = XmlArtist();

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
  XmlArtistParser::on_end_element    (Glib::Markup::ParseContext& context,
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
  XmlArtistParser::on_text  (Glib::Markup::ParseContext & context,
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
#endif
} // MPX
