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

#include "source-lastfm.hh"

#include <gtkmm.h>
#include <gtk/gtk.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <libglademm.h>
#include <map>
#include <cmath>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "mpx/mpx-public-mpx.hh"
#include "mpx/mpx-public-play.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-minisoup.hh"
#include "mpx/mpx-stock.hh"
#include "mpx/mpx-types.hh"
#include "mpx/mpx-uri.hh"
#include "mpx/util-graphics.hh"
#include "mpx/util-ui.hh"
#include "mpx/widgets/cell-renderer-cairo-surface.hh"
#include "mpx/xml/xmltoc++.hh"

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

        m_LastFMRadio.signal_playlist().connect(
            sigc::mem_fun(
                *this,
                &LastFM::on_playlist
        ));

        m_LastFMRadio.signal_no_playlist().connect(
            sigc::mem_fun(
                *this,
                &LastFM::on_playlist_cancel
        ));

        m_LastFMRadio.signal_tuned().connect(
            sigc::mem_fun(
                *this,
                &LastFM::on_tune_ok
        ));

        m_LastFMRadio.signal_tune_error().connect(
            sigc::mem_fun(
                *this,
                &LastFM::on_tune_error
        ));

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

        PAccess<MPX::Library> pa_lib;
        player.get_object(pa_lib);

#if 0
        ComponentTopAlbums * c = new ComponentTopAlbums(pa_lib);
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
#endif
    }

    std::string
    LastFM::get_guid ()
    {
        return "dafcb8ac-d9ff-4405-8a35-861ccdff66b5";
    }

    std::string
    LastFM::get_class_guid ()
    {
        return "51be6e23-11e0-4fc7-ac7c-d9d13461a105";
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
    LastFM::on_playlist_cancel ()
    {
        Signals.StopRequest.emit();
    }

    void
    LastFM::on_tune_ok ()
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
    LastFM::next_post ()
    {
        play_post ();
    }

    void
    LastFM::track_cover_cb (char const * data, guint size, guint code)
    {
        Glib::RefPtr<Gdk::Pixbuf> image = Glib::RefPtr<Gdk::Pixbuf>(0);

        try{
            Glib::RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create ();
            loader->write (reinterpret_cast<const guint8*>(data), size);
            loader->close ();
            image = loader->get_pixbuf();
        }
        catch (Gdk::PixbufError & cxe)
        {
            g_message("%s: Gdk::PixbufError: %s", G_STRLOC, cxe.what().c_str());
        }

        m_metadata.Image = image; 

        Signals.Metadata.emit(m_metadata);
    }

    void
    LastFM::play_post ()
    {
        g_return_if_fail (m_Playlist);
        g_return_if_fail (m_PlaylistIter != m_Playlist.get().Items.end());

        XSPF::Item const& item = *m_PlaylistIter;

        m_track_cover_req = Soup::Request::create(item.image);
        m_track_cover_req->request_callback().connect(
            sigc::mem_fun(
                *this,
                &LastFM::track_cover_cb
        ));
        m_track_cover_req->run();

        if( m_PlaylistIter != m_Playlist.get().Items.end() )
          m_Caps = Caps (m_Caps | PlaybackSource::C_CAN_GO_NEXT);
        else
          m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_GO_NEXT);
    }

    void
    LastFM::send_metadata ()
    {
        XSPF::Item const& item = *m_PlaylistIter;

        m_metadata = Metadata();
        m_metadata[ATTRIBUTE_ARTIST]  = item.creator;
        m_metadata[ATTRIBUTE_ALBUM]   = item.album;
        m_metadata[ATTRIBUTE_TITLE]   = item.title;
        m_metadata[ATTRIBUTE_TIME]    = gint64(item.duration);
        m_metadata[ATTRIBUTE_GENRE]   = "Last.fm: " + m_Playlist.get().Title; 

        Signals.Metadata.emit(m_metadata);
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
} // MPX
