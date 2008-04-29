//  MPX
//  Copyright (C) 2005-2007 <http://bmpx.backtrace.info> 
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

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <string>
#include <vector>
#include <map>
#include <set>

#include <glibmm/ustring.h>
#include <glibmm/utility.h>
#include "podcast-v2-types.hh"
#include "podcast-libxml2-sax.hh"
#include "podcast-cache-libxml2-sax.hh"

namespace 
{
  char const* PODCAST_DOC_HEAD_VERSION = "1";
}

namespace MPX
{
  namespace PodcastV2
  {
      class AttributeCreate
      {
        public:

          AttributeCreate (xmlNodePtr node)
          : mNode (node)
          {}

          void
          operator()(const ElementPair &a)
          {
            xmlNodePtr nodeAttribute = xmlNewTextChild (mNode, 0, BAD_CAST "attribute", BAD_CAST a.second.c_str());
            xmlSetProp (nodeAttribute, BAD_CAST "name", BAD_CAST a.first.c_str()); 
          }

        private:

          xmlNodePtr mNode;
      };

      class CategoryCreate
      {
        public:

          CategoryCreate (xmlNodePtr node)
          : mNode (node)
          {}

          void
          operator()(const Glib::ustring &data)
          {
            xmlNewTextChild (mNode, 0, BAD_CAST "category", BAD_CAST data.c_str());
          }

        private:

          xmlNodePtr mNode;
      };

      class EpisodeCreate
      {
        public:

          EpisodeCreate (xmlNodePtr node)
          : mNode (node)
          {}

          void
          operator()(EpisodePair & pair)
          {
            xmlNodePtr nodeEpisode = xmlNewChild (mNode, 0, BAD_CAST "item", 0); 

            // Create the primary nodes which aren't attributes first
            xmlNewTextChild (nodeEpisode, 0, BAD_CAST "title", BAD_CAST pair.second.item["title"].c_str());
            xmlNewTextChild (nodeEpisode, 0, BAD_CAST "link", BAD_CAST pair.second.item["link"].c_str()); 

            // Write attributes 
            xmlNodePtr nodeAttributes = xmlNewChild (nodeEpisode, 0, BAD_CAST "attributes", 0);    
            std::for_each (pair.second.attributes.begin(), pair.second.attributes.end(), AttributeCreate (nodeAttributes));
          }

        private:

          xmlNodePtr mNode;
      };

#if 0
      bool operator< (EpisodePair &a, EpisodePair &b)
      {
        time_t t1 = time_t (a.second.getAsInt ("pub-date-unix"));
        time_t t2 = time_t (b.second.getAsInt ("pub-date-unix"));

        return t1 < t2; 
      }

      bool operator< (Episode &a, Episode &b)
      {
        time_t t1 = time_t (a.getAsInt ("pub-date-unix"));
        time_t t2 = time_t (b.getAsInt ("pub-date-unix"));

        return t1 < t2; 
      }
#endif

      Episode&
      Podcast::get_most_recent_item ()
      {
        EpisodeMap::iterator i = item_map.begin();
        guint64 unixdate = 0;
        for (EpisodeMap::iterator n = item_map.begin(); n != item_map.end(); ++n)
        {
          if(n->second.getAsInt("pub-date-unix") > unixdate)
          {
            i = n;
            unixdate = i->second.getAsInt("pub-date-unix");
          }
        }
        return i->second; 
      }

      std::string
      Podcast::serialize ()
      {
        xmlNodePtr nodePodcast = xmlNewNode (0, BAD_CAST "podcast"); 
        xmlSetProp (nodePodcast, BAD_CAST "version", BAD_CAST PODCAST_DOC_HEAD_VERSION);

        xmlDocPtr document = xmlNewDoc (BAD_CAST "1.0");
        xmlDocSetRootElement (document, nodePodcast);

        // Create the primary nodes which aren't attributes first
        xmlNewTextChild (nodePodcast, 0, BAD_CAST "uuid", BAD_CAST uuid.c_str());
        xmlNewTextChild (nodePodcast, 0, BAD_CAST "title", BAD_CAST podcast["title"].c_str());
        xmlNewTextChild (nodePodcast, 0, BAD_CAST "source", BAD_CAST podcast["source"].c_str()); 
        xmlNewTextChild (nodePodcast, 0, BAD_CAST "link", BAD_CAST podcast["link"].c_str()); 
        xmlNewTextChild (nodePodcast, 0, BAD_CAST "description", BAD_CAST podcast["description"].c_str());
        xmlNewTextChild (nodePodcast, 0, BAD_CAST "image", BAD_CAST podcast["image"].c_str());
        xmlNewTextChild (nodePodcast, 0, BAD_CAST "itunes-image", BAD_CAST podcast["itunes-image"].c_str());
       
        // Write attributes 
        xmlNodePtr nodeAttributes = xmlNewChild (nodePodcast, 0, BAD_CAST "attributes", 0);    
        std::for_each (attributes.begin(), attributes.end(), AttributeCreate (nodeAttributes));

        // Write categories
        xmlNodePtr nodeCategories = xmlNewChild (nodePodcast, 0, BAD_CAST "categories", 0);
        std::for_each (categories.begin(), categories.end(), CategoryCreate (nodeCategories));

        // Create the <items> node
        xmlNodePtr nodeEpisodes = xmlNewChild (nodePodcast, 0, BAD_CAST "items", 0); 
        std::for_each (item_map.begin(), item_map.end(), EpisodeCreate (nodeEpisodes));

        xmlThrDefIndentTreeOutput (1);
        xmlKeepBlanksDefault (0);

        Glib::ScopedPtr<xmlChar> out; 
        int len;
    
        xmlDocDumpFormatMemoryEnc (document, out.addr(), &len, "utf-8", 1);
        xmlFreeDoc (document);

        return std::string (reinterpret_cast<char*>(out.get()), len);
      }

      void
      Podcast::parse (const std::string &data) throw (ParseError)
      {
        guint64 date_cur = 0; 

        if( !item_map.empty() )
        {
          date_cur = get_most_recent_item().getAsInt("pub-date-unix");
        }

        if( podcast_rss2_parse( *this, data ) )
        {
          throw ParseError();
        }

        if (!podcast["itunes-image"].empty())
          podcast["image"] = podcast["itunes-image"];
        
        guint64 date_new = 0; 
        if( !item_map.empty() )
        {
          date_new = get_most_recent_item().getAsInt("pub-date-unix");
          new_items = bool (date_new > date_cur); 
        }
      }

      void
      Podcast::deserialize (const std::string &data)
      {
        podcast.clear();
        attributes.clear();

        if( podcast_cache_parse (*this, data) )
        {
          throw ParseError();
        }
      }
  }
}
