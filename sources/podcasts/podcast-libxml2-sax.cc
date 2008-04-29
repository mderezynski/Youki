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
//  The MPX project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <glibmm.h>
#include <boost/format.hpp>

#include "podcast-v2-types.hh"
#include "podcast-libxml2-sax.hh"
#include "podcast-utils.hh"

using namespace MPX::PodcastV2;
using namespace std;
using namespace Glib;

#include "mpx/util-string.hh"
#include "mpx/parser/libxml2-sax-base.hh"
using MPX::XPath;

namespace
{
  struct PodcastParserContext
    : public ParseContextBase
  {
    Podcast       & m_cast;
    Episode         m_item;
    bool            m_block_parse;

    PodcastParserContext (Podcast & cast)
    : m_cast        (cast)
    , m_block_parse (false)
    {}
  };

  typedef std::map < ustring, ustring > ElementAttributes; 

#define DEFAULT_REFS                                                              \
  PodcastParserContext & context (static_cast<PodcastParserContext&>(_context));  \
  Podcast &cast G_GNUC_UNUSED (context.m_cast);                                   \
  if (context.m_block_parse)  \
  {                           \
    return;                   \
  }

#define GET_ITEM \
  MPX::PodcastV2::Episode & item (context.m_item);

  //////////////////////////////////////////////////////////////////////////////

  namespace Handlers
  {
    HANDLER(rss)
    {
      DEFAULT_REFS
      
      if (props.count ("version"))
      {
        ustring version = props.find ("version")->second;
        if (version != "" && (version.substr (0,1) != "2"))
        {
          g_warning ("%s: XML Is not RSS 2.0", G_STRFUNC);
          context.m_block_parse = true;
          return;
        }
      }
    }

    HANDLER(image)
    {
      DEFAULT_REFS
      if (prefix == "itunes")
      {
        cast << Element ("itunes-image", props["href"]);
      }
    }

    HANDLER(category)
    {
      DEFAULT_REFS
      if (prefix == "itunes")
      {
        cast << Category (props["text"]);
      }
    }

    namespace Episode
    {
      HANDLER(item)
      {
        DEFAULT_REFS
        GET_ITEM
        item = MPX::PodcastV2::Episode();
      }

      HANDLER(enclosure)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Attribute ("enclosure", props["url"]);
        item << Attribute ("enclosure-length", props["length"]);
        item << Attribute ("enclosure-type", props["type"]);
      }

      HANDLER(guid)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Attribute ("is-perma-link", props["isPermaLink"]);
      }
    }
  }

  namespace HandlersEnd
  {
    namespace Episode
    {
      HANDLER_END(item)
      {
        DEFAULT_REFS
        GET_ITEM

        if (item.attributes["enclosure"].empty())
          return;

        if (item.attributes["guid"].empty())
          item.attributes["guid"] = item.item["title"];

        try{
            if (cast.item_map.find (item.attributes["guid"]) == cast.item_map.end()) 
            {
              item << Attribute ("downloaded", "0"); 
              item << Attribute ("filename", "");
              item << Attribute ("played", "0");

              if( !cast.uuid.empty() && file_test( MPX::PodcastV2::cast_item_file(cast, item), FILE_TEST_EXISTS ) )
              {
                item.attributes["downloaded"] = "1";
                item.attributes["filename"] = MPX::PodcastV2::cast_item_file(cast, item); 
              }

              cast.item_map.insert (std::make_pair (item.attributes["guid"], item));
            }
            else
            {
              EpisodeMap::iterator i = cast.item_map.find (item.attributes["guid"]);
              if( !cast.uuid.empty() && !file_test( MPX::PodcastV2::cast_item_file(cast, item), FILE_TEST_EXISTS ) )
              {
                i->second.attributes["downloaded"] = "0";
                i->second.attributes["filename"] = "";
              }
            }

            if (time_t (item.getAsInt("pub-date-unix")) > cast.most_recent_item)
            {
              cast.most_recent_item = item.getAsInt("pub-date-unix");
              cast.most_recent_is_played = item.getAsBool("played");
            }

            if(item.getAsBool("downloaded"))
            {
              cast.downloaded++;

              if(!item.getAsBool("played"))
              {  
                cast.downloaded_and_new++;
              }
            }

          }
        catch (EpisodeInvalidError & cxe)
          {
          }
        catch (ConvertError & cxe)
          {
          }
      } 
    }
  }
 
  namespace HandlersText
  {
    HANDLER_Text(title)
    {
      DEFAULT_REFS
      cast << Element ("title", text);
    }

    HANDLER_Text(link)
    {
      DEFAULT_REFS
      cast << Element ("link", text);
    }

    HANDLER_Text(description)
    {
      DEFAULT_REFS
      cast << Element ("description", text);
    }

    HANDLER_Text(ttl)
    {
      DEFAULT_REFS
      cast << Attribute ("ttl", text);
    }

    HANDLER_Text(language)
    {
      DEFAULT_REFS
      cast << Attribute ("language", text); 
    }

    HANDLER_Text(copyright)
    {
      DEFAULT_REFS
      cast << Attribute ("copyright", text); 
    }

    HANDLER_Text(pubDate)
    {
      DEFAULT_REFS
      cast << Attribute ("pub-date", text);
      guint64 unix_date = MPX::Util::parseRFC822Date (text.c_str());
      if( unix_date )
      {
        cast << Attribute ("pub-date-unix", (boost::format ("%llu") % unix_date ).str() ); 
      }
    }

    HANDLER_Text(managingEditor)
    {
      DEFAULT_REFS
      cast << Attribute ("managing-editor", text);
    }

    HANDLER_Text(webMaster)
    {
      DEFAULT_REFS
      cast << Attribute ("web-master", text);
    }

    HANDLER_Text(category)
    {
      DEFAULT_REFS
    }

    namespace Episode
    {
      HANDLER_Text(title)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Element ("title", text);
      }

      HANDLER_Text(link)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Element ("link", text);
      }

      HANDLER_Text(comments)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Attribute ("comments", text);
      }

      HANDLER_Text(pubDate)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Attribute ("pub-date", text);
        guint64 unix_date = MPX::Util::parseRFC822Date (text.c_str());
        if( unix_date )
        {
          item << Attribute ("pub-date-unix", (boost::format ("%llu") % unix_date ).str() ); 
        }
      }

      HANDLER_Text(creator)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Attribute ("creator", text);
      }

      HANDLER_Text(description)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Attribute ("description", text);
      }

      HANDLER_Text(guid)
      {
        DEFAULT_REFS
        GET_ITEM
        item << Attribute ("guid", text);
      }
    }

    HANDLER_Text(image_url)
    {
      DEFAULT_REFS
      cast << Element ("image", text);
    }

    HANDLER_Text(image_title)
    {
      DEFAULT_REFS
    }

    HANDLER_Text(image_link)
    {
      DEFAULT_REFS
    }
  }

  HandlerPair
  handlers[] = 
  {
    HandlerPair(    XPath("rss"),
                      sigc::ptr_fun( &Handlers::rss )), 

    HandlerPair(    XPath("rss/channel/image"),
                      sigc::ptr_fun( &Handlers::image )),

    HandlerPair(    XPath("rss/channel/category"),
                      sigc::ptr_fun( &Handlers::category )),

    HandlerPair(    XPath("rss/channel/item"),
                      sigc::ptr_fun( &Handlers::Episode::item )),
     
    HandlerPair(    XPath("rss/channel/item/enclosure"),
                      sigc::ptr_fun( &Handlers::Episode::enclosure )),

    HandlerPair(    XPath("rss/channel/item/guid"),
                      sigc::ptr_fun( &Handlers::Episode::guid ))
  };

  HandlerEndPair
  handlers_end[] = 
  {
    HandlerEndPair( XPath("rss/channel/item"),
                      sigc::ptr_fun( &HandlersEnd::Episode::item )) 
  };

  HandlerTextPair
  handlers_text[] = 
  {
      HandlerTextPair( XPath("rss/channel/title"),
                      sigc::ptr_fun( &HandlersText::title )),

      HandlerTextPair( XPath("rss/channel/link"),
                      sigc::ptr_fun( &HandlersText::link )),

      HandlerTextPair( XPath("rss/channel/description"),
                      sigc::ptr_fun( &HandlersText::description )),

      HandlerTextPair( XPath("rss/channel/language"),
                      sigc::ptr_fun( &HandlersText::language )),

      HandlerTextPair( XPath("rss/channel/ttl"),
                      sigc::ptr_fun( &HandlersText::ttl )),

      HandlerTextPair( XPath("rss/channel/copyright"),
                      sigc::ptr_fun( &HandlersText::copyright )),

      HandlerTextPair( XPath("rss/channel/managingEditor"),
                      sigc::ptr_fun( &HandlersText::managingEditor )),

      HandlerTextPair( XPath("rss/channel/webMaster"),
                      sigc::ptr_fun( &HandlersText::webMaster )),

      HandlerTextPair( XPath("rss/channel/pubDate"),
                      sigc::ptr_fun( &HandlersText::pubDate )),

      HandlerTextPair( XPath("rss/channel/category"),
                      sigc::ptr_fun( &HandlersText::category )),



      HandlerTextPair( XPath("rss/channel/item/title"),
                      sigc::ptr_fun( &HandlersText::Episode::title )),
  
      HandlerTextPair( XPath("rss/channel/item/link"),
                      sigc::ptr_fun( &HandlersText::Episode::link )),

      HandlerTextPair( XPath("rss/channel/item/comments"),
                      sigc::ptr_fun( &HandlersText::Episode::comments )),

      HandlerTextPair( XPath("rss/channel/item/pubDate"),
                      sigc::ptr_fun( &HandlersText::Episode::pubDate )),

      HandlerTextPair( XPath("rss/channel/item/creator"),
                      sigc::ptr_fun( &HandlersText::Episode::creator )),

      HandlerTextPair( XPath("rss/channel/item/description"),
                      sigc::ptr_fun( &HandlersText::Episode::description )),

      HandlerTextPair( XPath("rss/channel/item/guid"),
                      sigc::ptr_fun( &HandlersText::Episode::guid )),



      HandlerTextPair( XPath("rss/channel/image/title"),
                      sigc::ptr_fun( &HandlersText::image_title )),

      HandlerTextPair( XPath("rss/channel/image/link"),
                      sigc::ptr_fun( &HandlersText::image_link )),

      HandlerTextPair( XPath("rss/channel/image/url"),
                      sigc::ptr_fun( &HandlersText::image_url )) 
  };
  //---------
}

namespace MPX
{
  namespace PodcastV2
  {
    int podcast_rss2_parse (Podcast & p, std::string const& data)
    {
      p.podcast["title"] = "";
      p.podcast["link"] = "";
      p.podcast["description"] = "";
      p.podcast["image"] = "";
      p.podcast["itunes-image"] = "";
      p.attributes.clear();

      PodcastParserContext context (p);

      // handler
      for (unsigned int n = 0; n < G_N_ELEMENTS(handlers); ++n)
      {
        context << handlers[n]; 
      }

      // handler/end 
      for (unsigned int n = 0; n < G_N_ELEMENTS(handlers_end); ++n)
      {
        context << handlers_end[n];
      }
       
      // handler/text
      for (unsigned int n = 0; n < G_N_ELEMENTS(handlers_text); ++n)
      {
        context << handlers_text[n];
      }
       
      int r = SaxParserBase::xml_base_parse(data, context);
      return r;
    }
  }
}
