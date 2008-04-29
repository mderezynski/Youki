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

#include "podcast-utils.hh"
#include "podcast-v2-types.hh"
#include "podcast-cache-libxml2-sax.hh"

using namespace MPX::PodcastV2;
using namespace std;
using namespace Glib;

#include "mpx/parser/libxml2-sax-base.hh"
using MPX::XPath;

namespace
{
  struct PodcastParserContext
    : public ParseContextBase
  {
    Podcast         & m_cast;
    Episode           m_item;
    std::string       m_current_attribute; // NOTE This is currently shared by both cast and items

    PodcastParserContext (Podcast & cast): m_cast (cast) {}
  };

#define DEFAULT_REFS                                                              \
  PodcastParserContext &context (static_cast<PodcastParserContext&>(_context));   \
  Podcast &cast G_GNUC_UNUSED (context.m_cast);                                   \
  Episode &item G_GNUC_UNUSED (context.m_item);                                  

  //////////////////////////////////////////////////////////////////////////////

  namespace Handlers
  {
    HANDLER(attribute)
    {
      DEFAULT_REFS
      context.m_current_attribute = props["name"];
    }
  }

  namespace Handlers_End
  {
    HANDLER_END(attribute)
    {
      DEFAULT_REFS
      context.m_current_attribute = ""; 
    }

    namespace Episodes
    {
      HANDLER_END(item)
      {
        DEFAULT_REFS

        if (item.attributes["enclosure"].empty())
          return;
      
        try{
            if( !cast.uuid.empty() && file_test( MPX::PodcastV2::cast_item_file(cast, item), FILE_TEST_EXISTS ) )
            {
              item.attributes["downloaded"] = "1";
              item.attributes["filename"] = MPX::PodcastV2::cast_item_file(cast, item); 
            }
            else
            {
              item.attributes["downloaded"] = "0";
              item.attributes["filename"] = "";
            }
      
            if (item.getAsInt("pub-date-unix") > cast.most_recent_item)
            {
              cast.most_recent_item = item.getAsInt("pub-date-unix");
              cast.most_recent_is_played = item.getAsBool("played");
            }

            cast.item_map.insert (std::make_pair (item.attributes["guid"], item));

          }
        catch (EpisodeInvalidError & cxe)
          {
          }
        catch (ConvertError & cxe)
          {
          }

        if(item.getAsBool("downloaded"))
        {
          cast.downloaded++;

          if (!item.getAsBool("played"))
          {
            cast.downloaded_and_new++;
          }
        }
 
        item = Episode();
      }
    }
  }

  namespace Handlers_Text
  {
    void element (std::string const& text, ParseContextBase & _context, std::string const& element)
    {
      DEFAULT_REFS
      cast << Element (element, text);
    }

    void attribute (std::string const& text, ParseContextBase & _context)
    {
      DEFAULT_REFS
      cast << Attribute (context.m_current_attribute, text);
    }

    namespace Episodes
    {
      void element (std::string const& text, ParseContextBase & _context, std::string const& element)
      {
        DEFAULT_REFS
        item << Element (element, text);
      }

      void attribute (std::string const& text, ParseContextBase & _context)
      {
        DEFAULT_REFS
        g_assert( !context.m_current_attribute.empty() );
        item << Attribute (context.m_current_attribute, text);
      }
    }
  }

  HandlerPair
  handlers_start[] = 
  {
    HandlerPair(    XPath("podcast/attributes/attribute"),
                      sigc::ptr_fun( &Handlers::attribute )),

    HandlerPair(    XPath("podcast/items/item/attributes/attribute"),
                      sigc::ptr_fun( &Handlers::attribute ))
  };
  
  HandlerEndPair 
  handlers_end[] = 
  {
    HandlerEndPair( XPath("podcast/attributes/attribute"),
                      sigc::ptr_fun( &Handlers_End::attribute )),

    HandlerEndPair( XPath("podcast/items/item/attributes/attribute"),
                      sigc::ptr_fun( &Handlers_End::attribute )),

    HandlerEndPair( XPath("podcast/items/item"),
                      sigc::ptr_fun( &Handlers_End::Episodes::item ))
  };

  HandlerTextPair
  handlers_text[] = 
  {
    HandlerTextPair( XPath("podcast/uuid"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::element ), "uuid")),

    HandlerTextPair( XPath("podcast/title"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::element ), "title")),

    HandlerTextPair( XPath("podcast/source"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::element ), "source")),

    HandlerTextPair( XPath("podcast/link"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::element ), "link")),

    HandlerTextPair( XPath("podcast/description"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::element ), "description")),

    HandlerTextPair( XPath("podcast/image"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::element ), "image")),

    HandlerTextPair( XPath("podcast/itunes-image"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::element ), "itunes-image")), 

    HandlerTextPair( XPath("podcast/attributes/attribute"),
      sigc::ptr_fun( &Handlers_Text::attribute )),

    HandlerTextPair( XPath("podcast/items/item/title"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::Episodes::element ), "title")),

    HandlerTextPair( XPath("podcast/items/item/link"),
      sigc::bind (sigc::ptr_fun( &Handlers_Text::Episodes::element ), "link")),

    HandlerTextPair( XPath("podcast/items/item/attributes/attribute"),
      sigc::ptr_fun( &Handlers_Text::Episodes::attribute ))
  };
}

namespace MPX
{
  namespace PodcastV2
  {
    int podcast_cache_parse (Podcast & cast, std::string const& data)
    {
      PodcastParserContext context (cast);

      for (unsigned int n = 0; n < G_N_ELEMENTS(handlers_start); context << handlers_start[n++]); 
      for (unsigned int n = 0; n < G_N_ELEMENTS(handlers_end); context << handlers_end[n++]); 
      for (unsigned int n = 0; n < G_N_ELEMENTS(handlers_text); context << handlers_text[n++]); 

      return SaxParserBase::xml_base_parse(data, context);
    }
  }
}
