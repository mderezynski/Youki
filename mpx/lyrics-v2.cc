//  BMP
//  Copyright (C) 2005-2007 BMP development.
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
//  The BMPx project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and BMPx. This
//  permission is above and beyond the permissions granted by the GPL license
//  BMPx is covered by.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <iostream>
#include <fstream>

#include <boost/format.hpp>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <glibmm.h>

#include "mpx/uri.hh"
#include "mpx/util-string.hh"

using namespace Glib;
using boost::format;
using std::string;

#include "lyrics-v2.hh"

namespace
{
  enum ElementLyrics
  {
    E_LYRICS_NONE                = 0,
    E_LYRICS_LYRICS              = 1 << 0,
  };

  struct LyricsParserContext
  {
    std::string  & m_lyrics;
    int            m_state;
    bool           m_element;

    LyricsParserContext (std::string & lyrics)
        : m_lyrics  (lyrics),
          m_state   (0),
          m_element (false)
    {}
  };

  typedef std::map < std::string, std::string > ElementAttributes;

#define SET_STATE(e)    ((context->m_state |= e))
#define CLEAR_STATE(e)  ((context->m_state &= ~e))
#define STATE(e)        ((context->m_state & e) != 0)

  //////////////////////////////////////////////////////////////////////////////

  void
  lyrics_start_element (void*           ctxptr,
                        xmlChar const*  _name,
                        xmlChar const** _attributes)
  {
    LyricsParserContext* context = static_cast<LyricsParserContext*> (ctxptr);
    std::string name (reinterpret_cast<char const*> (_name));
    if (name == "lyrics")
    {
      SET_STATE(E_LYRICS_LYRICS);
      return;
    }
  }

  void
  lyrics_end_element (void*          ctxptr,
                      xmlChar const* _name)
  {
    LyricsParserContext* context = static_cast<LyricsParserContext *> (ctxptr);
    std::string name (reinterpret_cast<char const*> (_name));
    if (name == "lyrics")
    {
      CLEAR_STATE(E_LYRICS_LYRICS);
      return;
    }
  }

  void
  lyrics_pcdata (void*          ctxptr,
                 xmlChar const* _text,
                 int            length)
  {
    LyricsParserContext* context = static_cast<LyricsParserContext *> (ctxptr);

    if (STATE(E_LYRICS_LYRICS))
      context->m_lyrics.append ((const char*)(_text), length);
  }

  ////// Common /////////////////////////////////////////////////////////////////

  xmlEntityPtr
  get_entity (void*          ctxptr,
              xmlChar const* name)
  {
    return xmlGetPredefinedEntity (name);
  }

  void
  whitespace (void*          ctxptr,
              xmlChar const* _text,
              int             length)
  {}

  void
  warning (void*       ctxptr,
           char const* message,
           ...)
  {
    va_list args;
    va_start(args, message);
    g_logv("LyricWiki", G_LOG_LEVEL_WARNING, message, args);
    va_end(args);
  }

  void
  error (void*       ctxptr,
         char const* message,
         ...)
  {
    va_list args;
    va_start(args, message);
    g_logv("LyricWiki", G_LOG_LEVEL_CRITICAL, message, args);
    va_end(args);
  }

  void
  fatal_error (void*       ctxptr,
               char const* message,
               ...)
  {
    va_list args;
    va_start(args, message);
    g_logv("LyricWiki", G_LOG_LEVEL_CRITICAL, message, args);
    va_end(args);
  }

  xmlSAXHandler LyricsParser =
  {
      (internalSubsetSAXFunc)0,                           // internalSubset
      (isStandaloneSAXFunc)0,                             // isStandalone
      (hasInternalSubsetSAXFunc)0,                        // hasInternalSubset
      (hasExternalSubsetSAXFunc)0,                        // hasExternalSubset
      (resolveEntitySAXFunc)0,                            // resolveEntity
      (getEntitySAXFunc)get_entity,                       // getEntity
      (entityDeclSAXFunc)0,                               // entityDecl
      (notationDeclSAXFunc)0,                             // notationDecl
      (attributeDeclSAXFunc)0,                            // attributeDecl
      (elementDeclSAXFunc)0,                              // elementDecl
      (unparsedEntityDeclSAXFunc)0,                       // unparsedEntityDecl
      (setDocumentLocatorSAXFunc)0,                       // setDocumentLocator
      (startDocumentSAXFunc)0,                            // startDocument
      (endDocumentSAXFunc)0,                              // endDocument
      (startElementSAXFunc)lyrics_start_element,          // startElement
      (endElementSAXFunc)lyrics_end_element,              // endElement
      (referenceSAXFunc)0,                                // reference
      (charactersSAXFunc)lyrics_pcdata,                   // characters
      (ignorableWhitespaceSAXFunc)whitespace,             // ignorableWhitespace
      (processingInstructionSAXFunc)0,                    // processingInstruction
      (commentSAXFunc)0,                                  // comment
      (warningSAXFunc)warning,                            // warning
      (errorSAXFunc)error,                                // error
      (fatalErrorSAXFunc)fatal_error,                     // fatalError
  };
}

namespace MPX
{
  namespace LyricWiki
  {
    TextRequest::TextRequest (const ustring &artist,
                              const ustring &title)
    : m_soup_request  (Soup::RequestSyncRefP (0))
    , m_artist        (artist)
    , m_title         (title)
    {
      string collation = artist.lowercase() + "_" + title.lowercase();
      string collation_md5_hex = Util::md5_hex_string (collation.data (), collation.size ());
    }
    
	std::string
    TextRequest::run ()
    {
        format lyrics_url_f ("http://lyricwiki.org/api.php?fmt=xml&artist=%s&song=%s");
        m_soup_request = Soup::RequestSync::create ((lyrics_url_f % m_artist.c_str() % m_title.c_str()).str());
        m_soup_request->add_header("User-Agent", "BMP-2.0");
        m_soup_request->add_header("Content-Type", "text/xml"); 
        m_soup_request->add_header("Accept-Charset", "utf-8");
        m_soup_request->add_header("Connection", "close");
        int code = m_soup_request->run ();
		if( code != 200)
			throw LyricsReturnNotOK();

		std::string data, lyrics;
		m_soup_request->get_data(data);

		LyricsParserContext context (lyrics);
		int rc = xmlSAXUserParseMemory (&LyricsParser, reinterpret_cast<void *>(&context), data.c_str(), data.size()); 

		if(rc)
			throw LyricsReturnNotOK();

		return lyrics;
    }

    TextRequest::~TextRequest ()
    {
    }

  }
}
