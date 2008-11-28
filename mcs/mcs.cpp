//  MCS
//  Copyright (C) 2005-2006 M.Derezynski 
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2 as published
//  by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  --
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <mcs/types.h>
#include <mcs/key.h>
#include <mcs/subscriber.h>
#include <mcs/mcs.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/lexical_cast.hpp>

namespace
{

  int
  register_namespaces (xmlXPathContextPtr xpathCtx, xmlChar const * nsList)
  {
    xmlChar* nsListDup;
    xmlChar* prefix;
    xmlChar* href;
    xmlChar* next;
    
    nsListDup = xmlStrdup(nsList);
    if(nsListDup == 0)
    {
      fprintf(stderr, "Error: unable to strdup namespaces list\n");
      return(-1);	
    }
    
    next = nsListDup; 

    while (next != 0)
    {
      while ((*next) == ' ')
        next++;

      if ((*next) == '\0')
        break;

      prefix = next;
      next = (xmlChar*)xmlStrchr(next, '=');

      if (next == 0)
        {
          std::cerr << "Error: invalid namespaces list format" << std::endl;
          xmlFree(nsListDup);
          return(-1);	
        }

      *(next++) = '\0';	
      
      href = next;
      next = (xmlChar*)xmlStrchr(next, ' ');

      if (next != 0)  
        {
          *(next++) = '\0';	
        }

      if (xmlXPathRegisterNs(xpathCtx, prefix, href) != 0)
        {
          std::cerr << "Error: unable to register NS with prefix='"
                    << prefix << "'" 
                    << " and href='"
                    << href   << "'"
                    << std::endl; 
          xmlFree(nsListDup);
          return(-1);	
        }
    }
    xmlFree(nsListDup);
    return 0;
  }

  xmlXPathObjectPtr
  xml_execute_xpath_expression (xmlDocPtr	     doc, 
                                const xmlChar *xpathExpr,
                                const xmlChar *nsList)
  {
    xmlXPathContextPtr xpathCtx = 0;
    xmlXPathObjectPtr  xpathObj = 0;

    xpathCtx = xmlXPathNewContext (doc);

    if (xpathCtx == 0)
      return 0;

    if (nsList)
      register_namespaces (xpathCtx, nsList);

    xpathObj = xmlXPathEvalExpression (xpathExpr, xpathCtx);
    if (xpathObj == 0)
      {
        xmlXPathFreeContext (xpathCtx);
        return 0;
      }

    xmlXPathFreeContext (xpathCtx);
    return xpathObj;
  }

};

namespace Mcs
{
      Mcs::Mcs(
            std::string const& xml_filename_,
            std::string const& root_node_name_,
            double             version_
      )
      : xml_filename(xml_filename_)
      , root_node_name(root_node_name_)
      , version(version_)
      {
      }

      Mcs::~Mcs ()
      {
        save ();
      }

      void
      Mcs::save ()
      {
        //Serialize the configuration into XML
        xmlDocPtr	        doc;
        xmlNodePtr          root_node;
        std::stringstream   version_str;

        doc = xmlNewDoc (BAD_CAST "1.0");
        version_str << this->version;

        //root node
        root_node = xmlNewNode (0, BAD_CAST root_node_name.c_str ());
        xmlSetProp (root_node, BAD_CAST "version", BAD_CAST (version_str.str().c_str()));
        xmlDocSetRootElement (doc, root_node);
          
        for (DomainsT::iterator iter = domains.begin (); iter != domains.end (); iter++)
        {
          std::string domain = (iter->first); 
          KeysT & keys = (iter->second);

          xmlNodePtr domain_node;
          domain_node = xmlNewNode (0, BAD_CAST "domain"); 
          xmlSetProp (domain_node, BAD_CAST("id"), BAD_CAST(domain.c_str ()));
          xmlAddChild (root_node, domain_node);
          
          for (KeysT::iterator iter = keys.begin (); iter !=  keys.end (); iter ++)
          {
                std::string key = iter->first; 
                KeyVariant const& variant = (iter->second).get_value ();
                KeyType type = (iter->second).get_type ();

                xmlNodePtr key_node;
                std::stringstream value_str;
                std::string prop;

                switch (type)
                {
                  case KEY_TYPE_INT:
                    {
                      value_str << boost::get<int>(variant);
                      prop = "integer";
                      break;
                    }

                  case KEY_TYPE_BOOL:
                    {
                      value_str << (boost::get<bool>(variant) ? "TRUE" : "FALSE");
                      prop = "boolean";
                      break;
                    }

                  case KEY_TYPE_STRING:
                    {
                      value_str << boost::get<std::string>(variant);
                      prop = "string";
                      break;
                    }

                  case KEY_TYPE_FLOAT:
                    {
                      value_str << boost::get<double>(variant);
                      prop = "float";
                      break;
                    }

                  } // switch (type)	

                key_node = xmlNewTextChild (domain_node, 0,
                                            BAD_CAST ("key"),
                                            BAD_CAST (value_str.str ().c_str ())); 

                xmlSetProp (key_node, BAD_CAST ("id"),
                                      BAD_CAST (key.c_str ()));

                xmlSetProp (key_node, BAD_CAST ("type"),
                                      BAD_CAST (prop.c_str()));
          }
        }

        xmlThrDefIndentTreeOutput (1);
        xmlKeepBlanksDefault (0);

        g_message ("Saving XML to %s", xml_filename.c_str());
        g_message ("Saved %d characters", xmlSaveFormatFileEnc (xml_filename.c_str(), doc, "utf-8", 1));

        xmlFreeDoc (doc);
        xmlFreeDoc (m_doc);
      } 

      void
      Mcs::load(
        VersionIgnore version_ignore
      )
      {
        if (!Glib::file_test (xml_filename, Glib::FILE_TEST_EXISTS))
        {
            m_doc = 0;
            return;
        }
  
        m_doc = xmlParseFile(xml_filename.c_str());
      }

      bool
      Mcs::domain_key_exist(
        std::string const& domain,
        std::string const& key
      )
      {
        if (domains.find (domain) == domains.end())
            return false;
        if (domains.find (domain)->second.find (key) == domains.find (domain)->second.end ())
            return false;

        return true;
      }

      void 
      Mcs::domain_register(
        std::string const& domain
      )
      {
        if( domains.find (domain) != domains.end() )
        {
            g_message("MCS: domain_register() Domain [%s] has already been registered", domain.c_str());
            g_return_if_fail (domains.find (domain) == domains.end());
        }

		domains[domain] = KeysT();
      }

      void
      Mcs::key_register(
            std::string const& domain, //Must be registered
            std::string const& key,
            KeyVariant  const& key_default
      )
      {
        if( domains.find(domain) == domains.end() )
        {
            g_message("MCS: key_register() Domain [%s] has not yet been registered", domain.c_str());
      	    g_return_if_fail( domains.find(domain) != domains.end() );
        }

        if( domains.find(domain)->second.find(key) != domains.find(domain)->second.end() )
        {
            g_message("MCS: key_register() Domain [%s] Key [%s] has already been registered", domain.c_str(), key.c_str());
            g_return_if_fail( domains.find(domain)->second.find(key) == domains.find(domain)->second.end() );
        }

        domains.find( domain )->second[key] = Key( domain, key, key_default, KeyType(key_default.which()) );

        if( !m_doc )
        {
            return;
        }

        Key               & key_instance = domains.find( domain )->second[key];
        KeyType             type         = key_instance.get_type(); 
        KeyVariant          variant;
        std::stringstream   xpath;
        xmlXPathObjectPtr   xpathObj;
        std::string         nodeval;
        char*               nodeval_C;
                
        xpath << "/" << root_node_name << "/domain[@id='" << domain << "']/key[@id='" << key << "']";

        xpathObj = xml_execute_xpath_expression (m_doc,
                    BAD_CAST (xpath.str ().c_str ()),
                    BAD_CAST ("mcs=http://backtrace.info/ns/0/mcs"));

        if (!xpathObj)
        {
            g_message("No xpathobj");
            goto clear_xpathobj;
        }

        if (!xpathObj->nodesetval)
        {
            g_message("No nodes");
            goto clear_xpathobj;
        }

        if (!xpathObj->nodesetval->nodeNr)
        {
            g_message("No nodeNr");
            goto clear_xpathobj;
        }

        if (xpathObj->nodesetval->nodeNr > 1) //XXX: There can only be ONE node!
        {
            g_message("More than 1 key with the same name");
            goto clear_xpathobj;
        }

        if (!xpathObj->nodesetval->nodeTab[0]->children)
        {
            g_message("Key is empty");
            goto clear_xpathobj;
        }

        nodeval_C = reinterpret_cast<char*>(XML_GET_CONTENT(xpathObj->nodesetval->nodeTab[0]->children));
        nodeval = nodeval_C;
        g_free (nodeval_C);

        switch( type )
        {
          case KEY_TYPE_INT:
              variant = boost::lexical_cast<int>(nodeval);
              break;

          case KEY_TYPE_BOOL:
              variant = (!nodeval.compare ("TRUE")) ? true : false;
              break;

          case KEY_TYPE_STRING:
              variant = std::string(nodeval);
              break;

          case KEY_TYPE_FLOAT:
              variant = boost::lexical_cast<double>(nodeval);
              break;

        }

        key_instance.set_value_silent( variant );

        clear_xpathobj:
        xmlXPathFreeObject( xpathObj );
      }

      void 
      Mcs::key_unset(
            std::string const& domain,
            std::string const& key
      )
      {
        g_return_if_fail (domain_key_exist(domain, key));
        return domains.find(domain)->second.find(key)->second.unset ();
      } 

      void 
      Mcs::subscribe(
            std::string const& name,   //Must be unique
            std::string const& domain, //Must be registered 
            std::string const& key,    //Must be registered,
            SubscriberNotify const& notify
      )
      {
        g_return_if_fail (domain_key_exist(domain, key));
        return domains.find (domain)->second.find (key)->second.subscriber_add(name, notify);
      }

      void 
      Mcs::unsubscribe(
            std::string const& name,   //Must be unique
            std::string const& domain, //Must be registered 
            std::string const& key     //Must be registered,
      ) 
      {
        g_return_if_fail (domain_key_exist(domain, key));
        return domains.find(domain)->second.find(key)->second.subscriber_del(name);
      }
}
