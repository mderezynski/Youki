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

#include <cstring>
#include <cctype>
#include <vector>

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
#include "last-fm-xmlrpc.hh"

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

#define MPX_AS_USERAGENT      "MPX2"
#define MPX_LASTFMRADIO_VERSION   "2.0.0.0"
#define MPX_LASTFMRADIO_PLATFORM  "linux"

namespace
{
  typedef std::map < std::string, std::string > StringMap;

  StringMap 
  parse_to_map (std::string const& buffer)
  {
    using namespace MPX;

    StringMap map;
    StrV lines;
    split_regex (lines, buffer, boost::regex ("\\\r?\\\n"));

    for (StrV::size_type n = 0; n < lines.size(); ++n)
    {
      iterator_range<std::string::iterator> match = find_nth (lines[n], "=", 0);
      if (match)
      {
        map[std::string(lines[n].begin(), match.begin())] = std::string (match.end(), lines[n].end()); 
      }
    }
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
  namespace LastFM
  {
    namespace XMLRPC
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
              UStrV const& strv (boost::get < ::MPX::UStrV > (v));
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
        m_soup_request->add_header("User-Agent", "MPX-1.0");
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
        m_soup_request->add_header("User-Agent", "MPX-2.0");
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

      ArtistMetadataRequestSync::ArtistMetadataRequestSync (ustring const& artist) 
      : XmlRpcCallSync ()
      , m_artist (artist)
      {
        m_v.push_back (m_artist);
        m_v.push_back (ustring ("en"));
        setMethod ("artistMetadata");
      }

	  std::string
      ArtistMetadataRequestSync::run ()
      {
        int code = m_soup_request->run ();
		std::string text;
		if(code == 200)
		{
			xmlDocPtr             doc;
			xmlXPathObjectPtr     xpathobj;
			xmlNodeSetPtr         nv;

			std::string data;
			m_soup_request->get_data(data);
	
			doc = xmlParseMemory (data.c_str(), data.size()); 
			if (!doc)
			{
				g_message("%s: Couldn't parse document", G_STRLOC);
				return text;	
			}

			xpathobj = xpath_query (doc, BAD_CAST "//member[name = 'wikiText']/value/string", NULL);
			if (!xpathobj)
			{
				g_message("%s: Couldn't fetch wikiText (Xpath error)", G_STRLOC);
				return text;
			}

			nv = xpathobj->nodesetval;
			if (!nv || !nv->nodeNr)
			{
				g_message("%s: Xpath error: no nodes", G_STRLOC);
				xmlXPathFreeObject (xpathobj);
				return text;
			}

			xmlNodePtr node = nv->nodeTab[0];
			if (!node || !node->children)
			{
				g_message("%s: Xpath error: no child nodes", G_STRLOC);
				xmlXPathFreeObject (xpathobj);
				return text;
			}

			xmlChar * pcdata = XML_GET_CONTENT (node->children);

			if (pcdata)
			{
			  text = (reinterpret_cast<char *>(pcdata));
			  xmlFree (pcdata);
			}
			else
				g_message("%s: XML error: no text", G_STRLOC);

		  }
		  else
				g_message("%s: Request result not OK: %d", G_STRLOC, code);

	      return Util::sanitize_lastfm(text);
	}
}
}
}
