/*  MPX - The Dumb Music Player
 *  Copyright (C) 2003-2006-2007 MPX Project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * The MPX project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and MPX. This
 * permission are above and beyond the permissions granted by the GPL license
 * MPX is covered by.
 */

#include <iostream>
#include <sstream>

#include <glibmm.h>
#include <glibmm/markup.h>
#include <glibmm/i18n.h>
#include <glib.h>
#include <gtkmm.h>

#include <boost/format.hpp>

#include "mpx/uri.hh"
#include "mpx/minisoup.hh"
using namespace Glib;
using namespace Gtk;

#include "radio-directory-types.hh"
#include "streams-icecast.hh"

namespace MPX
{
  namespace RadioDirectory
  {
    class IcecastParser
        : public Glib::Markup::Parser
    {
      public:

        IcecastParser (StreamListT & entries);
        virtual ~IcecastParser ();

        void check_sanity ();

      protected:

        virtual void
        on_start_element  (Glib::Markup::ParseContext& context,
                           Glib::ustring const& elementname,
                           AttributeMap const& attributes);

        virtual void
        on_end_element (Glib::Markup::ParseContext& context,
                        Glib::ustring const& elementname);

        virtual void
        on_text (Glib::Markup::ParseContext& context,
                 Glib::ustring const& text);
      private:

        StreamListT & m_entries;

#define STATE(e) ((state & e) != 0)
#define SET_STATE(e) ((state |= e))
#define CLEAR_STATE(e) ((state &= ~ e))

        enum Element
        {
          E_NONE         = 0,
          E_DIRECTORY    = 1 << 0,
          E_ENTRY        = 1 << 1,
          E_SERVER_NAME  = 1 << 2,
          E_LISTEN_URL   = 1 << 3,
          E_SERVER_TYPE  = 1 << 4,
          E_BITRATE      = 1 << 5,
          E_CHANNELS     = 1 << 6,
          E_SAMPLERATE   = 1 << 7,
          E_GENRE        = 1 << 8,
          E_CURRENT_SONG = 1 << 9,
        };

        StreamInfo entry;
        int state;
      };

      void
      IcecastParser::check_sanity  ()
      {
        if (state)
        {
          g_warning (G_STRLOC ": State should be 0, but is %d", state);
        }
      }

      IcecastParser::IcecastParser (StreamListT & entries) 
      : m_entries (entries)
      , state     (0)
      {
      }

      IcecastParser::~IcecastParser () 
      {
      }
   
      void
      IcecastParser::on_start_element  (Glib::Markup::ParseContext& context,
                                        Glib::ustring const&        name,
                                        AttributeMap const&         attributes)
      {
        if (name == "directory")
        {
          SET_STATE(E_DIRECTORY);
          return;
        }

        if (name == "entry")
        {
          SET_STATE(E_ENTRY);
          entry = StreamInfo ();
          return;
        }

        if (name == "server_name")
        {
          SET_STATE(E_SERVER_NAME);
          return;
        }

        if (name == "listen_url")
        {
          SET_STATE(E_LISTEN_URL);
          return;
        }

        if (name == "server_type")
        {
          SET_STATE(E_SERVER_TYPE);
          return;
        }

        if (name == "bitrate")
        {
          SET_STATE(E_BITRATE);
          return;
        }

        if (name == "channels")
        {
          SET_STATE(E_CHANNELS);
          return;
        }

        if (name == "samplerate")
        {
          SET_STATE(E_SAMPLERATE);
          return;
        }

        if (name == "genre")
        {
          SET_STATE(E_GENRE);
          return;
        }

        if (name == "current_song")
        {
          SET_STATE(E_CURRENT_SONG);
          return;
        }
      }

      void
      IcecastParser::on_end_element    (Glib::Markup::ParseContext& context,
                                        Glib::ustring const& name)
      {
        if (name == "directory")
        {
          CLEAR_STATE(E_DIRECTORY);
          return;
        }

        if (name == "entry")
        {
          CLEAR_STATE(E_ENTRY);
          m_entries.push_back (entry); 
          return;
        }

        if (name == "server_name")
        {
          CLEAR_STATE(E_SERVER_NAME);
          return;
        }

        if (name == "listen_url")
        {
          CLEAR_STATE(E_LISTEN_URL);
          return;
        }

        if (name == "server_type")
        {
          CLEAR_STATE(E_SERVER_TYPE);
          return;
        }

        if (name == "bitrate")
        {
          CLEAR_STATE(E_BITRATE);
          return;
        }

        if (name == "channels")
        {
          CLEAR_STATE(E_CHANNELS);
          return;
        }

        if (name == "samplerate")
        {
          CLEAR_STATE(E_SAMPLERATE);
          return;
        }

        if (name == "genre")
        {
          CLEAR_STATE(E_GENRE);
          return;
        }

        if (name == "current_song")
        {
          CLEAR_STATE(E_CURRENT_SONG);
          return;
        }
      }

      void
      IcecastParser::on_text  (Glib::Markup::ParseContext& context,
                               Glib::ustring const&  text)
      {
        if (!STATE(E_ENTRY))
          return;

        if (STATE(E_SERVER_NAME))
        {
          entry.name = text;
          return;
        }

        if (STATE(E_LISTEN_URL))
        {
          entry.uri = text;
          return;
        }

        if (STATE(E_SERVER_TYPE))
        {
          //entry.server_type = text;
          return;
        }

        if (STATE(E_GENRE))
        {
          entry.genre = text;
          return;
        }

        if (STATE(E_CHANNELS))
        {
          //entry.channels = g_ascii_strtoull (text.c_str(), NULL, 10); 
          return;
        }

        if (STATE(E_SAMPLERATE))
        {
          //entry.samplerate = g_ascii_strtoull (text.c_str(), NULL, 10); 
          return;
        }

        if (STATE(E_BITRATE))
        {
          entry.bitrate = g_ascii_strtoull (text.c_str(), NULL, 10); 
          return;
        }

        if (STATE(E_CURRENT_SONG))
        {
          entry.current += text;
          return;
        }
      }
    
      void
      Icecast::refresh_callback (char const* data, guint size, guint code)
      {
        if (code != 200)
        {
          BaseData.Request.clear();
          Signals.Stop.emit ();
          return;
        }

        std::string response;
        response.append (data, size); 

        StreamListT entries;
        try{
            IcecastParser parser (entries);
            Markup::ParseContext context (parser);
            context.parse (response);
            context.end_parse ();
            parser.check_sanity ();
        }
        catch (ConvertError& cxe)
        {
          return; 
        }
        catch (MarkupError& cxe)
        {
          return;
        }

        set_stream_list (entries);
        Signals.Stop.emit ();
      }

      void
      Icecast::refresh ()
      {
        BaseData.Streams->clear ();
        Signals.Start.emit ();
        BaseData.Request->run ();
      }

      Icecast::Icecast (BaseObjectType                       * obj,
                        Glib::RefPtr<Gnome::Glade::Xml> const& xml)
      : ViewBase (obj, xml)
      {
        BaseData.Request = Soup::Request::create ("http://dir.xiph.org/yp.xml");
        BaseData.Request->request_callback().connect (sigc::mem_fun (*this, &Icecast::refresh_callback));
		refresh ();
      }
  }
};
