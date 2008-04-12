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

        TagInserter (MPX::LastFMTagView & view)
        : m_View(view)
        {
        }

        void
        operator()(::tag const& t)
        {
            m_View.insert_tag( t.name(), t.url(), t.count() );
        }

      private:

        MPX::LastFMTagView & m_View;
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
        m_ref_xml->get_widget_derived("tagview", m_TagView);
        m_ref_xml->get_widget("hbox-error", m_HBox_Error);
        m_ref_xml->get_widget("label-error", m_Label_Error);
        m_ref_xml->get_widget("button-error-hide", m_Button_Error_Hide);

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
        return item.location;
    }

    void
    LastFM::go_next_async ()
    {
        if( m_PlaylistIter == m_Playlist.get().Items.end() )
        {
            m_Caps = Caps (m_Caps & ~PlaybackSource::C_CAN_GO_NEXT);
            send_caps();
            m_LastFMRadio.get_xspf_playlist ();
        } 
        else
        {
            m_PlaylistIter++;
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
} // MPX
