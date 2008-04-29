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

#ifndef MPX_PODCAST_V2_TYPES_HH 
#define MPX_PODCAST_V2_TYPES_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <glibmm/ustring.h>
#include <boost/format.hpp>

#include "mpx/util-string.hh"

namespace glib = Glib;

namespace MPX
{
  namespace PodcastV2
  {
      struct Policy
      {
        enum DownloadPolicy
        {
          DOWNLOAD_MANUAL,
          DOWNLOAD_MOST_RECENT
        };

        enum UpdateInterval
        {
          UPDATE_STARTUP,
          UPDATE_HOURLY,  
          UPDATE_30MINS,
        };

        enum CachePolicy
        {
          CACHE_ALL,
          CACHE_CURRENT
        };
      };
    
      typedef std::map<std::string, std::string> ElementMap;
      typedef ElementMap::value_type ElementPair;

      typedef std::set<std::string> TextSet;

      struct Podcast;
      struct Episode;

      struct Category
      {
        Category (const std::string &arg_data, bool arg_user_category = false)
        : data (arg_data)
        , user_category (arg_user_category)
        {}
        std::string data;
        bool        user_category;
      };

      struct AttributeProvider
      {
        ElementMap attributes;

        guint64
        getAsInt (const std::string &name)
        {
          std::string attr = attributes[name];
          return g_ascii_strtoull (attr.c_str(), NULL, 10);
        }

        std::string&
        getAsString (const std::string &name)
        {
          return attributes[name];
        }

        bool
        getAsBool (const std::string &name)
        {
          std::string attr = attributes[name];
          return ( attr == "true" || attr == "1" );
        }

        time_t
        getTime (const std::string &name)
        {
          std::string attr = attributes[name]; 
          return MPX::Util::parseRFC822Date (attr.c_str());
        }

        std::string
        getTimeAsString (const std::string &name)
        {
          std::string attr = attributes[name]; 
          return (boost::format ("%llu") % MPX::Util::parseRFC822Date (attr.c_str())).str();
        }
      };

      struct Attribute
      {
        Attribute (const std::string &arg_name,
                   const std::string &arg_data)
        : name  (arg_name)
        , data  (arg_data)
        {}
        std::string name;
        std::string data;
      };

      struct Element
      {
        Element (const std::string &arg_name,
                 const std::string &arg_data)
        : name  (arg_name)
        , data  (arg_data)
        {}
        std::string name;
        std::string data;
      };

      struct Episode
        : public AttributeProvider
      {
        ElementMap item; 

        virtual void operator<< (const Element &e)
        {
          item[e.name] += e.data;
        }

        virtual void operator<< (const Attribute &a)
        {
          attributes[a.name] += a.data;
        }

        virtual ~Episode () {}
      };

      typedef std::map<std::string, Episode> EpisodeMap;
      typedef EpisodeMap::value_type EpisodePair;

#if 0
      bool operator< (EpisodePair &, EpisodePair &);
      bool operator< (Episode&, Episode&);
#endif

      struct Podcast
        : public AttributeProvider
      {

#include "mpx/exception.hh"

        EXCEPTION(ParseError)

        Podcast ()
        : most_recent_item (0)
        , most_recent_is_played (false)
        , new_items (false)
        , downloaded (0)
        , downloaded_and_new (0)
        {}

        Podcast (std::string const& data) throw (ParseError)
        : most_recent_item (0)
        , most_recent_is_played (false)
        , new_items (false)
        , downloaded(0)
        , downloaded_and_new (0)
        {
          deserialize (data); 
        }

        virtual ~Podcast () {}

        ElementMap    podcast; 
        ElementMap    attributes;
        EpisodeMap    item_map;
        TextSet       categories;

        // Auxilliary Data
        std::string   uuid;
        time_t        most_recent_item;
        bool          most_recent_is_played;
        bool          new_items;
        guint64      downloaded;
        guint64      downloaded_and_new;

        virtual void operator<< (const Element &e)
        {
          podcast[e.name] += e.data;
        }

        virtual void operator<< (const Attribute &a)
        {
          attributes[a.name] += a.data;
        }

        virtual void operator<< (const Category &e)
        {
          categories.insert (e.data);
        }

        Episode&
        get_most_recent_item ();

        void 
        parse (const std::string &data) throw (ParseError);

        std::string
        serialize ();

        void
        deserialize (const std::string &data);
      };

      typedef std::list <std::string>           PodcastList;
      typedef std::map  <std::string, Podcast>  PodcastMap;
  }
}
#endif
