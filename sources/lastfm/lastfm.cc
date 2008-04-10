//-*- Mode: C++; indent-tabs-mode: nil; -*-

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
//  The MPXx project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPXx. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPXx is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <gtk/gtk.h>
#include <gtkmm.h>
#include <glibmm.h>
#include <glibmm/i18n.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <iostream>
#include <fstream>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

#include "mpx/base64.h"
#include "mpx/md5.h"
#include "mpx/main.hh"
#include "mpx/uri.hh"
#include "mpx/util-string.hh"
#include "mpx/xspf-libxml2-sax.hh"
#include "mpx/xml.hh"

#include "lastfm.hh"

using namespace Gtk;
using namespace Glib;
using namespace Markup;
using namespace std;
using namespace MPX;
using boost::algorithm::split;
using boost::algorithm::split_regex;
using boost::algorithm::is_any_of;
using boost::algorithm::find_nth;
using boost::algorithm::replace_all;
using boost::algorithm::trim;
using boost::iterator_range;

#define MPX_AS_USERAGENT "Last.fm Client 1.4.2.58240 (X11)"
#define MPX_LASTFMRADIO_VERSION "1.4.2.58240"
#define MPX_LASTFMRADIO_PLATFORM "linux"
#define MPX_LASTFMRADIO_PLATFORM_VERSION "Unix%2FLinux"

namespace
{
  typedef std::map < std::string, std::string > StringMap;

  StringMap 
  parse_to_map (std::string const& buffer)
  {
    using boost::algorithm::split;
    using boost::algorithm::split_regex;
    using boost::algorithm::is_any_of;

    StringMap map;

    //vector<string> lines;
    //split_regex (lines, buffer, boost::regex ("\\\r?\\\n"));
	char **lines = g_strsplit(buffer.c_str(), "\n", -1);

    //for (unsigned int n = 0; n < lines.size(); ++n)
    for (int n = 0; lines[n] != NULL; ++n)
    {
      char **line = g_strsplit (lines[n], "=", 2);
      if (line[0] && line[1] && strlen(line[0]) && strlen(line[1]))
      {
        std::string key (line[0]);
        std::string val (line[1]);
        trim(key);
        trim(val);
        map[std::string (ustring(key).lowercase())] = val; 
      }
      g_strfreev (line);
    }
    g_strfreev (lines);
    return map;
  }

  bool
  startsWith (std::string const& source, std::string const& target)
  {
    if (source.length() < target.length())
      return false;

    return (source.substr(0, target.length()) == target);
  }
}

namespace MPX
{
namespace LastFMXMLRPC
{
      ustring
      formatXmlRpc (ustring const& method, xmlRpcVariantV const& argv)
      {
        xmlDocPtr doc (xmlNewDoc (BAD_CAST "1.0"));
        xmlNodePtr n_methodCall, n_methodName, n_params;

        n_methodCall = xmlNewNode (0, BAD_CAST "methodCall");
        xmlDocSetRootElement (doc, n_methodCall);

        n_methodName = xmlNewTextChild (n_methodCall, 0, BAD_CAST "methodName", BAD_CAST method.c_str());
        n_params = xmlNewChild (n_methodCall, 0, BAD_CAST "params", 0);

        for (xmlRpcVariantV::const_iterator i = argv.begin(); i != argv.end(); ++i)
        {
          xmlNodePtr n_param, n_value;
          n_param = xmlNewChild (n_params, 0, BAD_CAST "param", 0);
          n_value = xmlNewChild (n_param, 0, BAD_CAST "value", 0);

          xmlRpcVariant const& v (*i);

          switch (v.which())
          {
            case 0:
            {
              xmlNewTextChild (n_value, 0, BAD_CAST "string", BAD_CAST (boost::get <ustring> (v)).c_str());
              break;
            }

            case 1:
            {
              xmlNodePtr n_array (xmlNewChild (n_value, 0, BAD_CAST "array", 0)), n_data (xmlNewChild (n_array, 0, BAD_CAST "data", 0));
              UStrV const& strv (boost::get < UStrV > (v));
              for (UStrV::const_iterator i = strv.begin() ; i != strv.end(); ++i)
              {
                xmlNodePtr n_data_value (xmlNewChild (n_data, 0, BAD_CAST "value", 0)),
                           G_GNUC_UNUSED n_data_string (xmlNewTextChild (n_data_value, 0, BAD_CAST "string", BAD_CAST (*i).c_str()));
              }
              break;
            }
          }
        }

        xmlKeepBlanksDefault (0);
        xmlChar * xmldoc = 0;
        int doclen = 0;
        xmlDocDumpFormatMemoryEnc (doc, &xmldoc, &doclen, "UTF-8", 1);
        ustring doc_utf8 ((const char*)(xmldoc));
        g_free (xmldoc);
        return doc_utf8;
      }

      /////////////////////////////////////////////////
      ///// Requests
      /////////////////////////////////////////////////

      XmlRpcCall::XmlRpcCall ()
      {
        m_soup_request = Soup::Request::create ("http://ws.audioscrobbler.com/1.0/rw/xmlrpc.php", true) ;
        m_soup_request->add_header("User-Agent", "BMP-2.0");
        m_soup_request->add_header("Accept-Charset", "utf-8");
        m_soup_request->add_header("Connection", "close");
      }
      void
      XmlRpcCall::setMethod (const ustring &method)
      { 
        ustring xmlData = formatXmlRpc (method, m_v);
        m_soup_request->add_request ("text/xml", xmlData);
      }
      void
      XmlRpcCall::cancel ()
      {
        m_soup_request.clear();
      }
      XmlRpcCall::~XmlRpcCall ()
      {
      }


      XmlRpcCallSync::XmlRpcCallSync ()
      {
        m_soup_request = Soup::RequestSync::create ("http://ws.audioscrobbler.com/1.0/rw/xmlrpc.php", true) ;
        m_soup_request->add_header("User-Agent", "BMP-2.0");
        m_soup_request->add_header("Accept-Charset", "utf-8");
        m_soup_request->add_header("Connection", "close");
      }
      void
      XmlRpcCallSync::setMethod (const ustring &method)
      { 
        ustring xmlData = formatXmlRpc (method, m_v);
        m_soup_request->add_request ("text/xml", xmlData);
      }
      XmlRpcCallSync::~XmlRpcCallSync ()
      {
      }


      ArtistMetadataRequest::ArtistMetadataRequest (ustring const& artist) 
      : XmlRpcCall ()
      , m_artist (artist)
      {
        m_v.push_back (m_artist);
        m_v.push_back (ustring ("en"));
        setMethod ("artistMetadata");
      }

      ArtistMetadataRequestRefPtr
      ArtistMetadataRequest::create (ustring const& artist)
      {
        return ArtistMetadataRequestRefPtr (new ArtistMetadataRequest (artist));
      }

      void
      ArtistMetadataRequest::run ()
      {
        m_soup_request->request_callback().connect (sigc::mem_fun (*this, &ArtistMetadataRequest::reply_cb));
        m_soup_request->run ();
      }

      void
      ArtistMetadataRequest::reply_cb (char const* data, guint size, guint status_code)
      {
        std::string chunk;

        xmlDocPtr             doc;
        xmlXPathObjectPtr     xpathobj;
        xmlNodeSetPtr         nv;

        doc = xmlParseMemory (data, size); 
        if (!doc)
        {  
          s_.emit (chunk, status_code);
          return;
        }

        xpathobj = xpath_query (doc, BAD_CAST "//member[name = 'wikiText']/value/string", NULL);
        if (!xpathobj)
        {
          s_.emit (chunk, status_code);
          return;
        }

        nv = xpathobj->nodesetval;
        if (!nv || !nv->nodeNr)
        {
          xmlXPathFreeObject (xpathobj);
          s_.emit (chunk, status_code);
          return;
        }

        xmlNodePtr node = nv->nodeTab[0];
        if (!node || !node->children)
        {
          xmlXPathFreeObject (xpathobj);
          s_.emit (chunk, status_code);
          return;
        }

        xmlChar * pcdata = XML_GET_CONTENT (node->children);

        if (pcdata)
        {
          chunk = (reinterpret_cast<char *>(pcdata));
          xmlFree (pcdata);
        }

        s_.emit (chunk, status_code);
      }



      RequestBase::RequestBase (ustring const& name)
      : m_name  (name)
      , m_pass  (mcs->key_get <std::string> ("lastfm", "password"))
      , m_user  (mcs->key_get <std::string> ("lastfm", "username"))
      , m_time  ((boost::format ("%llu") % time (NULL)).str())
      , m_key   (Util::md5_hex_string (m_pass.data(), m_pass.size()) + m_time) 
      , m_kmd5  (Util::md5_hex_string (m_key.data(), m_key.size()))
      {
        m_v.push_back (m_user);
        m_v.push_back (m_time);
        m_v.push_back (m_kmd5);
      }
  
      void
      RequestBase::run ()
      {
        setMethod (m_name);
        m_soup_request->run ();
      }



      TrackAction::TrackAction (ustring const& name, XSPF::Item const& item)
      : RequestBase (name)
      {
        m_v.push_back (item.creator);
        m_v.push_back (item.title);
      }



      TagAction::TagAction (ustring const& name, XSPF::Item const& item, UStrV const& strv)
      : RequestBase (name)
      {
        m_v.push_back (item.creator);

        if (m_name == "tagArtist")
        {
          /* nothing to do */
        }
    
        else

        if (m_name == "tagAlbum")
        {
          m_v.push_back (item.album);
        }

        else

        if (m_name == "tagTrack")
        {
          m_v.push_back (item.title);
        }

        m_v.push_back (strv);
        m_v.push_back (ustring ("append"));
      }


      RecommendAction::RecommendAction (RecommendItem item,
                                        ustring const& artist,
                                        ustring const& album,
                                        ustring const& title,
                                        ustring const& user,
                                        ustring const& message)
      : RequestBase ("recommendItem")
      {
        m_v.push_back (artist);

        if (item == RECOMMEND_ARTIST)
        {
          m_v.push_back (ustring());
          m_v.push_back ("artist");
        }

        else
        
        if (item == RECOMMEND_ALBUM)
        {
          m_v.push_back (album);
          m_v.push_back ("album");
        }

        else

        if (item == RECOMMEND_TRACK)
        {
          m_v.push_back (title);
          m_v.push_back ("track");
        }

        m_v.push_back (user);
        m_v.push_back (message);
        m_v.push_back ("en");
      }
} //LastFMXMLRPC 
} //MPX 


namespace MPX
{
    Signal&
    LastFMRadio::signal_tuned ()
    {
      return Signals.Tuned;
    }

    SignalTuneError&
    LastFMRadio::signal_tune_error ()
    {
      return Signals.TuneError;
    }

    SignalPlaylist&
    LastFMRadio::signal_playlist ()
    {
      return Signals.Playlist;
    }

    Signal&
    LastFMRadio::signal_no_playlist ()
    {
      return Signals.NoPlaylist;
    }

    Signal&
    LastFMRadio::signal_disconnected()
    {
      return Signals.Disconnected;
    }

    Signal&
    LastFMRadio::signal_connected()
    {
      return Signals.Connected;
    }

    LastFMRadio::LastFMRadio ()
    : m_IsConnected (false)
    {}

    LastFMRadio::~LastFMRadio ()
    {
      m_HandshakeRequest.clear ();
    }

    bool
    LastFMRadio::connected () const
    {
      return m_IsConnected;
    }

    void
    LastFMRadio::handshake_cb (char const * data, guint size, guint code)
    {
      if (code != 200)
      {
		m_IsConnected = false;
        Signals.Disconnected.emit ();
        return;
      }

      std::string response (data, size);
      StringMap m = parse_to_map (response);
      StringMap::iterator iter_session = m.find ("session");

      if (iter_session != m.end () && iter_session->second == "FAILED")
      {
          m_IsConnected = false;
          Signals.Disconnected.emit ();
          return;
      }

      m_Session.SessionId = iter_session->second;

      if (m.find ("subscriber") != m.end ())
          m_Session.Subscriber = bool(atoi(m.find ("subscriber")->second.c_str()));

      if (m.find ("base_url") != m.end ())
          m_Session.BaseUrl = m.find ("base_url")->second;

      if (m.find ("base_path") != m.end ())
          m_Session.BasePath = m.find ("base_path")->second;

      m_IsConnected = (!m_Session.SessionId.empty() && !m_Session.BaseUrl.empty() && !m_Session.BasePath.empty());

      if (m_IsConnected)
      {
        g_message( "%s: LastFM LastFMRadio Handshake OK. ID [%s], URL: [%s%s]",
          G_STRLOC,
          m_Session.SessionId.c_str(),
          m_Session.BaseUrl.c_str(),
          m_Session.BasePath.c_str()
        );
        Signals.Connected.emit ();
      }
      else
      {
        g_message( "%s: LastFM LastFMRadio Handshake unsucessful (session, url or url path are empty)", G_STRLOC );
        Signals.Disconnected.emit ();
      }
    }
    
    void
    LastFMRadio::disconnect ()
    {
      m_HandshakeRequest.clear();
      m_IsConnected = false;
      Signals.Disconnected.emit ();
    }

    void 
    LastFMRadio::handshake ()
    {
      if (!m_IsConnected)
      {
        std::string user (mcs->key_get <std::string> ("lastfm", "username"));
        std::string pass (mcs->key_get <std::string> ("lastfm", "password"));
        std::string pmd5 (Util::md5_hex_string (pass.data(), pass.size()));

        std::string uri ((boost::format ("http://ws.audioscrobbler.com/radio/handshake.php?version=%s&platform=%s&platformversion=%s&username=%s&passwordmd5=%s")
                          % MPX_LASTFMRADIO_VERSION
                          % MPX_LASTFMRADIO_PLATFORM 
                          % MPX_LASTFMRADIO_PLATFORM_VERSION
                          % user
                          % pmd5
                        ).str());

        URI u (uri, true); 
        m_HandshakeRequest = Soup::Request::create (ustring (u));
        m_HandshakeRequest->request_callback().connect (sigc::mem_fun (*this, &LastFMRadio::handshake_cb));
        m_HandshakeRequest->run ();
      }
    }

    LastFMRadio::Session const& 
    LastFMRadio::session () const
    {
      return m_Session;
    }

    void
    LastFMRadio::playurl_cb (char const * data, guint size, guint code, bool playlist) 
    {
      if (code != 200)
      {
        g_message("%s: Tune Error: %u", G_STRLOC, code);
        Signals.TuneError.emit ((boost::format (_( "Request Reply: %u")) % code).str());
        return;
      }

      if( playlist ) 
      {
        parse_playlist (data, size);
        Signals.Tuned.emit ();
        return;
      }
  
      std::string chunk;
      chunk.append (data, size);
      StringMap m (parse_to_map (chunk));

      g_message("%s: Response: '%s'", G_STRLOC, m["response"].c_str());

      if (m["response"] != std::string("OK"))
      {
        int error_code = g_ascii_strtoull (m["error"].c_str(), NULL, 10);
        std::string error_message;
        switch (error_code)
        {
            case LFM_ERROR_NOT_ENOUGH_CONTENT:
            {
                error_message = _( "There is not enough content to play this station." );
            }
            break;

            case LFM_ERROR_FEWGROUPMEMBERS:
            {
                error_message = _( "This group does not have enough members for radio." );
            }
            break;

            case LFM_ERROR_FEWFANS:
            {
                error_message = _( "This artist does not have enough fans for radio." );
            }
            break;

            case LFM_ERROR_UNAVAILABLE:
            {
                error_message = _( "This item is not available for streaming." );
            }
            break;

            case LFM_ERROR_SUBSCRIBE:
            {
                error_message = _( "This feature is only available to subscribers." );
            }
            break;

            case LFM_ERROR_FEWNEIGHBOURS:
            {
                error_message = _( "There are not enough neighbours for this radio." );
            }
            break;

            case LFM_ERROR_OFFLINE:
            {
                error_message = _( "The streaming system is offline for maintenance, please try again later." );
                m_IsConnected = false;
                Signals.Disconnected.emit ();
            }
            break;

            default:
            {
                error_message = _( "Starting radio failed, Unknown Error (Last.fm Servers might be down)" );
                m_IsConnected = false;
                Signals.Disconnected.emit ();
            }
        }
        Signals.TuneError.emit(error_message);
        return;
      }

      Signals.Tuned.emit();
    }

    void
    LastFMRadio::playurl (std::string const& url)
    {
      if (!m_IsConnected)
        return;

      std::string request_uri;
      bool playlist = is_playlist (url);

      if( playlist )
      {
        request_uri =((boost::format ("http://%s/1.0/webclient/getresourceplaylist.php?sk=%s&url=%s&desktop=1")
                % m_Session.BaseUrl
                % m_Session.SessionId
                % URI::escape_string (url)).str()
        );
      }
      else
      {
        request_uri =((boost::format ("http://%s%s/adjust.php?session=%s&url=%s&lang=en")
                % m_Session.BaseUrl
                % m_Session.BasePath
                % m_Session.SessionId
                % URI::escape_string (url)).str()
        );
      }

      g_message("%s: Creating request with URI: %s", G_STRLOC, request_uri.c_str());

      m_PlayURLRequest = Soup::Request::create (request_uri);
      m_PlayURLRequest->request_callback().connect (sigc::bind (sigc::mem_fun (*this, &LastFMRadio::playurl_cb), playlist));
      m_PlayURLRequest->run ();
    }

    void
    LastFMRadio::playlist_cb (char const * data, guint size, guint code)
    {
      if (code == SOUP_STATUS_CANCELLED)
      {
            return;
      }

      if (code == 401) // invalid session
      {
            m_IsConnected = false;
            Signals.Disconnected.emit ();
            Signals.NoPlaylist.emit ();
            return;
      }
    
      if (code == 503) // playlist service not responding
      {
            /* ... */
            Signals.NoPlaylist.emit ();
            return;
      }
     
      if (code == 200) 
      {
            parse_playlist (data, size);
            return;
      }
    }

    bool
    LastFMRadio::is_playlist (std::string const& url)
    {
      return (startsWith(url, "lastfm://play/") ||
              startsWith(url, "lastfm://preview/") ||
              startsWith(url, "lastfm://track/") ||
              startsWith(url, "lastfm://playlist/"));
    }

    void
    LastFMRadio::parse_playlist (char const* data, guint size)
    {
      XSPF::Playlist playlist;
      XSPF::XSPF_parse (playlist, data, size); 
      replace_all (playlist.Title, "+" , " ");
      trim (playlist.Title);
      playlist.Title = URI::unescape_string (playlist.Title);
      Signals.Playlist.emit( playlist );
    }
    
    void
    LastFMRadio::get_xspf_playlist_cancel ()
    {
      m_PlaylistRequest.clear();
    }

    void
    LastFMRadio::get_xspf_playlist ()
    {
      std::string uri ((boost::format ("http://%s%s/xspf.php?sk=%s&discovery=%d&desktop=1.3.0.58") 
                        % m_Session.BaseUrl
                        % m_Session.BasePath 
                        % m_Session.SessionId 
                        % int (mcs->key_get<bool>("lastfm","discoverymode"))).str());

      m_PlaylistRequest = Soup::Request::create (uri);
      m_PlaylistRequest->request_callback().connect (sigc::mem_fun (*this, &LastFMRadio::playlist_cb));
      m_PlaylistRequest->run ();
    }
}
