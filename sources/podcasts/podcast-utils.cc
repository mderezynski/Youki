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

#include <glib/gstdio.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <iostream>
#include <fstream>

#include "mpx/main.hh"
#include "mpx/uri.hh"

#include "podcast-v2-types.hh"
#include "podcast-utils.hh"

using namespace boost::algorithm;
using namespace MPX; 

namespace MPX
{
  namespace PodcastV2
  {
    std::string
    cast_filename (Podcast & cast)
    {
      return (build_filename (build_filename(g_get_user_cache_dir(), "mpx" G_DIR_SEPARATOR_S "podcasts"), cast.uuid));
    }

    std::string
    cast_image_filename (Podcast & cast)
    {
      return cast_filename (cast) + ".png";
    }
   
    std::string
    cast_item_path (Podcast & cast)
    {
      if( cast.podcast.count( "title" ) == 0)
        throw PodcastInvalidError();
    
      std::string title = filename_from_utf8 (cast.podcast.find ("title")->second);
      return build_filename (mcs->key_get<std::string>("podcasts","download-dir"), title);
    }

    std::string
    cast_item_file (Podcast & cast, Episode & item)
    {
      if( item.attributes.count( "pub-date-unix" ) == 0)
        throw EpisodeInvalidError();

      if( item.attributes.count( "enclosure" ) == 0)
        throw EpisodeInvalidError();

      time_t item_time = item.getAsInt ("pub-date-unix");
      struct tm atm;
      localtime_r (&item_time, &atm);
      char date[128];
      strftime (date, 128, "(%d-%m-%Y %H%M)", &atm);

      URI u (item.attributes["enclosure"]);

      std::string type;
      StrV subs;

      if( u.path.find('.') != ustring::npos )
      {
          split (subs, u.path, is_any_of ("."));
          type = subs[subs.size()-1]; 
      }
      else
      {
          split (subs, item.attributes["enclosure-type"], is_any_of ("/"));
          if( !subs.empty() )
          {
            type = (subs.size() == 2) ? subs[1] : subs[0]; 
          }
          else
          {
            type = ""; // FIXME: What to do here now?
          }
      }

      std::string title = filename_from_utf8 (item.item.find("title")->second);
      replace_all (title, "/", "-");
      replace_all (title, ":", "-");
      title = std::string (date) + std::string(" ") + title;

      return build_filename (cast_item_path (cast), title) + "." + type; 
    }
  }
}
