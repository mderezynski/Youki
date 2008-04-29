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
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <glib/gstdio.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <gdkmm/pixbuf.h>
#include <iostream>
#include <fstream>
#include <cstring>

#include "podcast.hh"
#include "podcast-utils.hh"
#include "podcast-libxml2-sax.hh"
#include "podcast-cache-libxml2-sax.hh"

#include "mpx/minisoup.hh"
#include "mpx/util-string.hh"
#include "mpx/util-ui.hh"
#include "mpx/uri.hh"

using namespace Glib;

namespace
{
  static boost::format time_f ("%llu");

  std::string
  get_uuid (ustring const& uri)
  {
    return MPX::Util::md5_hex_string (uri.c_str(), strlen(uri.c_str())); 
  }
}

namespace MPX
{
  namespace PodcastV2
  {
    PodcastManager::PodcastManager ()
    {
      std::string filename (build_filename (build_filename(g_get_user_data_dir(), "mpx"), "feedlist.opml"));
      if (file_test (filename, FILE_TEST_EXISTS))
      {
        try{
            OPMLParser parser (m_casts, *this);
            Markup::ParseContext context (parser);
            std::string data = file_get_contents (filename);
            context.parse (data);
            context.end_parse ();
            if (!parser.check_sanity ())
            {
              throw MPX::PodcastV2::ParsingError();
            }
          }
        catch (MarkupError & cxe)
          {
            g_critical ("%s: Catch exception: %s", G_STRLOC, cxe.what().c_str());
          }
        catch (std::exception & cxe)
          {
            g_critical ("%s: Catch exception: %s", G_STRLOC, cxe.what());
          }
      }
    }

    void
    PodcastManager::save_state ()
    {
      save_opml (build_filename (build_filename(g_get_user_data_dir(), "mpx"), "feedlist.opml"));
      for (PodcastMap::iterator i = m_casts.begin(); i != m_casts.end(); ++i)
      {
        std::ofstream o (cast_filename (i->second).c_str()); 
        o << i->second.serialize (); 
        o.close ();
      }
    }

    void
    PodcastManager::save_opml (std::string const& filename)
    {
      xmlDocPtr doc = xmlNewDoc (BAD_CAST "1.0");
        
      xmlNodePtr opml = xmlNewNode (NULL, BAD_CAST "opml");
      xmlNodePtr head = xmlNewNode (NULL, BAD_CAST "head");
      xmlNodePtr body = xmlNewNode (NULL, BAD_CAST "body");

      xmlSetProp (opml, BAD_CAST "version", BAD_CAST "1.0"); 
      xmlNewTextChild (head, NULL, BAD_CAST "title", BAD_CAST "MPX Podcast List");
      xmlDocSetRootElement (doc, opml);

      xmlAddChild (opml, head);
      xmlAddChild (opml, body);

      for (PodcastMap::iterator i = m_casts.begin() ; i != m_casts.end() ; ++i)
      {
        Podcast & cast (i->second);
        xmlNodePtr o = xmlNewNode (NULL, BAD_CAST "outline");
        xmlAddChild (body, o);

        xmlSetProp (o, BAD_CAST "text",
                       BAD_CAST  cast.podcast["title"].c_str());

        xmlSetProp (o, BAD_CAST "title",
                       BAD_CAST  cast.podcast["title"].c_str());

        xmlSetProp (o, BAD_CAST "type",  
                       BAD_CAST "rss");

        xmlSetProp (o, BAD_CAST "description", 
                       BAD_CAST  cast.podcast["description"].c_str());

        xmlSetProp (o, BAD_CAST "htmlUrl", 
                       BAD_CAST  cast.podcast["link"].c_str()); 

        xmlSetProp (o, BAD_CAST "xmlUrl", 
                       BAD_CAST  cast.podcast["source"].c_str()); 

        xmlSetProp (o, BAD_CAST "updateInterval", 
                       BAD_CAST (time_f % cast.getAsInt("ttl")).str().c_str()); 

        xmlSetProp (o, BAD_CAST "id", 
                       BAD_CAST  cast.uuid.c_str()); 

        xmlSetProp (o, BAD_CAST "lastPollTime", 
                       BAD_CAST  (time_f % cast.getAsInt("last-poll-time")).str().c_str()); 

        xmlSetProp (o, BAD_CAST "lastFaviconPollTime", 
                       BAD_CAST "0"); 

        xmlSetProp (o, BAD_CAST "sortColumn", 
                       BAD_CAST "time"); 
      }

      xmlKeepBlanksDefault (0);
      xmlChar * data;
      int size;
      xmlDocDumpFormatMemoryEnc (doc, &data, &size, "UTF-8", 1);

      std::ofstream o (filename.c_str());
      o << data;
      o.close ();

      xmlFreeDoc (doc);
      g_free (data);
    }

    PodcastManager::~PodcastManager ()
    {
      save_state ();
    }

    void
    PodcastManager::podcast_delete (ustring const& uri)
    {
      Podcast & cast = m_casts["uri"];

      g_unlink (cast_filename (cast).c_str());
      if (cast.podcast.count("image") != 0)
        g_unlink (cast_image_filename (cast).c_str());

      m_casts.erase (uri);
    }

    void
    PodcastManager::podcast_get_list (PodcastList & list)
    {
      for (PodcastMap::iterator i = m_casts.begin() ; i != m_casts.end() ; ++i)
      {
        list.push_back (i->second.podcast["source"]);
      }
    }

    Podcast&
    PodcastManager::podcast_fetch (ustring const& uri)
    {
      return m_casts[uri];
    }

    Episode&
    PodcastManager::podcast_fetch_item (ustring const& uri, std::string const& guid)
    {
      return m_casts[uri].item_map[guid];
    }

    void
    PodcastManager::podcast_load (Podcast & cast)
    {
      try{
        if (file_test (cast_filename (cast), FILE_TEST_EXISTS))
        {
          try{
              cast.deserialize (Glib::file_get_contents( cast_filename (cast) ) );
            }
         catch (ConvertError & cxe)
            {
              throw PodcastV2::ParsingError(cxe.what()); 
            }
         catch (MarkupError & cxe)
            {
              throw PodcastV2::ParsingError(cxe.what()); 
            }
        }
        else
        {
          podcast_insert (cast.podcast["source"], cast.uuid);
        }
      }
     catch (URI::ParseError & cxe)
        {
          throw PodcastV2::PodcastInvalidError(cxe.what()); 
        }
     catch (ConvertError & cxe)
        {
          throw PodcastV2::ParsingError(cxe.what()); 
        }
    }

    void
    PodcastManager::podcast_updated_cb (char const* data, guint size, guint code, ustring const& uri)
    {
      if (code != 200)
      {
        mSignals.SignalNotUpdated.emit( uri );
        return;
      }

      std::string response (data, size);

      Podcast & cast = m_casts[uri];

      ElementMap attr_save = cast.attributes;
      ElementMap cast_save = cast.podcast;

      cast.attributes.clear ();
      cast.podcast.clear ();

      try{
          cast.parse (response);

          cast.attributes["last-poll-time"] = (time_f % time (NULL)).str();
          cast.podcast["source"] = uri;

          std::ofstream o (cast_filename (cast).c_str()); 
          o << cast.serialize (); 
          o.close ();

          if (cast.podcast.count ("image") != 0)
          {
            RefPtr<Gdk::Pixbuf> cast_image = Util::get_image_from_uri (cast.podcast["image"]);

            if (cast_image)
              cast_image->save (cast_image_filename (cast), "png");
            else
              cast.podcast["image"] = "";
          }
          mSignals.SignalUpdated.emit( uri );
          return;
        }
      catch (Podcast::ParseError & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Parse Error '%s': %s", uri.c_str(), cxe.what()); 
        }
      catch (std::exception & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Stdexception: %s", cxe.what()); 
        }

      cast.attributes = attr_save;
      cast.podcast = cast_save;
      mSignals.SignalNotUpdated.emit( uri );
    }

    void
    PodcastManager::podcast_update_async (ustring const& uri)
    {
      try{
          MPX::URI u;
          try{
            u = MPX::URI (uri, true);
          }
          catch (URI::ParseError & cxe)
          {
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Invalid URI '%s': %s", uri.c_str(), cxe.what()); 
            throw PodcastV2::InvalidUriError(cxe.what()); 
          }

          m_requests[uri] = Soup::Request::create (ustring (u));
          m_requests[uri]->request_callback().connect( sigc::bind( sigc::mem_fun( *this, &PodcastManager::podcast_updated_cb ), uri));
          m_requests[uri]->run ();
        }
     catch (URI::ParseError & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Invalid URI '%s': %s", uri.c_str(), cxe.what()); 
          throw PodcastV2::PodcastInvalidError(cxe.what()); 
        }
     catch (ConvertError & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Convert Error: %s", cxe.what().c_str()); 
          throw PodcastV2::ParsingError(cxe.what()); 
        }
      catch (std::exception & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Stdexception: %s", cxe.what()); 
          throw PodcastV2::ParsingError(cxe.what()); 
        }
    }

    void
    PodcastManager::podcast_update (ustring const& uri)
    {
      Podcast & cast = m_casts[uri];

      cast.attributes.clear ();
      cast.podcast.clear ();

      std::string data; 
      try{
          MPX::URI u;
          try{
            u = MPX::URI (uri, true);
          }
          catch (URI::ParseError & cxe)
          {
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Invalid URI '%s': %s", uri.c_str(), cxe.what()); 
            throw PodcastV2::InvalidUriError(cxe.what()); 
          }

          Soup::RequestSyncRefP request = Soup::RequestSync::create (ustring (u));
          guint code = request->run ();

          if (code != 200)
          {
            throw PodcastV2::ParsingError(_("Bad HTTP Server Reply"));
          }
            
          data = request->get_data (); 
          if (data.empty())
            throw PodcastV2::ParsingError(_("Empty data in Reply"));

          cast.parse (data);
          cast.attributes["last-poll-time"] = (time_f % time (NULL)).str();
          cast.podcast["source"] = uri;
        }
     catch (Podcast::ParseError & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Parse Error '%s': %s", uri.c_str(), cxe.what()); 
          throw PodcastV2::PodcastInvalidError(cxe.what()); 
        }
     catch (URI::ParseError & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Invalid URI '%s': %s", uri.c_str(), cxe.what()); 
          throw PodcastV2::PodcastInvalidError(cxe.what()); 
        }
     catch (ConvertError & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Convert Error: %s", cxe.what().c_str()); 
          throw PodcastV2::ParsingError(cxe.what()); 
        }
      catch (std::exception & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Stdexception: %s", cxe.what()); 
          throw PodcastV2::ParsingError(cxe.what()); 
        }

      std::ofstream o (cast_filename (cast).c_str()); 
      o << cast.serialize (); 
      o.close ();

      if (cast.podcast.count ("image") != 0)
      {
        RefPtr<Gdk::Pixbuf> cast_image = Util::get_image_from_uri (cast.podcast["image"]);

        if (cast_image)
          cast_image->save (cast_image_filename (cast), "png");
        else
          cast.podcast["image"] = "";
      }
    }

    void
    PodcastManager::podcast_insert (ustring const& uri, std::string const& uuid)
    {
      Podcast cast;
      cast.uuid = uuid.empty() ? get_uuid (uri) : uuid;

      std::string data; 
      try{
          MPX::URI u;
          try{
            u = MPX::URI (uri, true);
          }
          catch (URI::ParseError & cxe)
          {
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Invalid URI '%s': %s", uri.c_str(), cxe.what()); 
            throw PodcastV2::InvalidUriError(cxe.what()); 
          }

          Soup::RequestSyncRefP request = Soup::RequestSync::create (ustring (u));
          guint code = request->run ();

          if (code != 200)
          {
            throw PodcastV2::ParsingError(_("Bad HTTP Server Reply"));
          }
            
          data = request->get_data (); 
          if (data.empty())
            throw PodcastV2::ParsingError(_("Empty data in Reply"));

          try{
            cast.parse (data);
            cast.attributes["last-poll-time"] = (time_f % time (NULL)).str();
            cast.podcast["source"] = uri;
          }
          catch (PodcastV2::Podcast::ParseError & cxe)
          {
            throw PodcastV2::ParsingError(_("Malformed feed data; couldn't parse XML."));
          }
        }
     catch (URI::ParseError & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Invalid URI '%s': %s", uri.c_str(), cxe.what()); 
          throw PodcastV2::PodcastInvalidError(cxe.what()); 
        }
     catch (ConvertError & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Convert Error: %s", cxe.what().c_str()); 
          throw PodcastV2::ParsingError(cxe.what()); 
        }
      catch (std::exception & cxe)
        {
          g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "RSS: Stdexception: %s", cxe.what()); 
          throw PodcastV2::ParsingError(cxe.what()); 
        }

      std::string filename = cast_filename (cast);

      std::ofstream o (filename.c_str());
      o << cast.serialize (); 
      o.close ();

      if (cast.podcast.count ("image") != 0)
      {
        RefPtr<Gdk::Pixbuf> cast_image = Util::get_image_from_uri (cast.podcast["image"]);

        if (cast_image)
          cast_image->save (cast_image_filename (cast), "png");
        else
          cast.podcast["image"] = "";
      }

      m_casts.insert (std::make_pair (uri, cast));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

#define STATE(e) ((m_state & e) != 0)
#define SET_STATE(e) ((m_state |= e))
#define CLEAR_STATE(e) ((m_state &= ~ e))

    OPMLParser::OPMLParser (PodcastMap & casts, PodcastManager & manager)
    : m_casts   (casts)
    , m_manager (manager)
    , m_state   (0) {}


    bool OPMLParser::check_sanity  ()
    {
      if (m_state)
      {
        g_warning (G_STRLOC ": State should be 0, but is %d", m_state);
        return false;
      }
      return true;
    }

    void OPMLParser::on_start_element  (Markup::ParseContext  & context,
                                        ustring          const& name,
                                        AttributeMap     const& attributes)
	  {
      if (name == "opml") 
      {
        SET_STATE(E_OPML);
      }

      if (name == "head") 
      {
        SET_STATE(E_HEAD);
      }

      if (name == "body") 
      {
        SET_STATE(E_BODY);
      }

      if (name == "outline") 
      {
        SET_STATE(E_OUTLINE);

        try{
          if (attributes.find ("xmlUrl") != attributes.end())
          {
                Podcast cast; 
                cast.uuid = attributes.find ("id")->second;
                cast.podcast["source"] = attributes.find ("xmlUrl")->second;

                if( !cast.uuid.empty() && !cast.podcast["source"].empty())
                {
                  m_manager.podcast_load (cast);
                  if (attributes.count ("lastPollTime") != 0) 
                  {
                    cast.attributes["last-poll-time"] = attributes.find ("lastPollTime")->second;
                  }
                  m_casts.insert (std::make_pair (cast.podcast["source"], cast)); 
                }
          }
        }
        catch (MPX::PodcastV2::ParsingError & cxe) {}
        catch (std::exception & cxe) {}
      }
    }

    void OPMLParser::on_end_element    (Markup::ParseContext    & context,
                                        ustring            const& name)
	{
      if (name == "opml") 
      {
        CLEAR_STATE(E_OPML);
      }

      if (name == "head") 
      {
        CLEAR_STATE(E_HEAD);
      }

      if (name == "body") 
      {
        CLEAR_STATE(E_BODY);
      }

      if (name == "outline") 
      {
        CLEAR_STATE(E_OUTLINE);
      }
    }
  }
}
