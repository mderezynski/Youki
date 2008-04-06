//  MCS
//  Copyright (C) 2005-2006 M. Derezynski 
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2 as published
//  by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
      Mcs::Mcs (std::string const& xml_filename, std::string const& root_node_name, double version)
          : xml_filename (xml_filename), root_node_name (root_node_name), version (version)
      {
      }

      Mcs::~Mcs ()
      {
        //Serialize the configuration into XML
        xmlDocPtr	  doc;
        xmlNodePtr	root_node;
        std::stringstream version_str;

        doc = xmlNewDoc (BAD_CAST "1.0");
        version_str << this->version;

        //root node
        root_node = xmlNewNode (0, BAD_CAST root_node_name. c_str ());
        xmlSetProp (root_node, BAD_CAST "version", BAD_CAST (version_str.str().c_str()));
        xmlDocSetRootElement (doc, root_node);
          
        for (MDomains::iterator iter = domains. begin (); iter != domains. end (); iter++)
        {
          std::string domain = (iter->first); 
          MKeys &keys = (iter->second);
          xmlNodePtr domain_node;

          //domain node
          domain_node = xmlNewNode (0, BAD_CAST "domain"); 
          xmlSetProp (domain_node, BAD_CAST("id"), BAD_CAST(domain. c_str ()));
          xmlAddChild (root_node, domain_node);
          
          for (MKeys::iterator iter = keys. begin (); iter !=  keys. end (); iter ++)
          {
                std::string key = iter->first; 
                const KeyVariant &variant = (iter->second). get_value ();
                KeyType type = (iter->second). get_type ();

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
                                            BAD_CAST (value_str. str (). c_str ())); 

                xmlSetProp (key_node, BAD_CAST ("id"),
                                      BAD_CAST (key. c_str ()));

                xmlSetProp (key_node, BAD_CAST ("type"),
                                      BAD_CAST (prop. c_str()));
          }
        }

        xmlThrDefIndentTreeOutput (1);
        xmlKeepBlanksDefault (0);

        g_message ("Saving XML to %s", xml_filename.c_str());
        int ret = xmlSaveFormatFileEnc (xml_filename.c_str(), doc, "utf-8", 1);
        g_message ("Saved %d characters", ret);
        xmlFreeDoc (doc);
      } 

      void
      Mcs::load (VersionIgnore version_ignore)
      {
        if (!Glib::file_test (xml_filename, Glib::FILE_TEST_EXISTS))
          return;
  
        xmlDocPtr doc;
        doc = xmlParseFile (xml_filename. c_str ());
      
        //Traverse all registered keys and set the default value
        for (MDomains::iterator iter = domains. begin (); iter != domains. end (); iter++)
        {
          std::string domain = (iter->first); 
          MKeys &keys = (iter->second);

          for (MKeys::iterator iter = keys. begin (); iter !=  keys. end (); iter ++)
            {
                KeyType type = iter->second.get_type ();
                std::string const& key  (iter->first); 
                Key & mkey (iter->second);
                KeyVariant variant;

                std::stringstream xpath;
                xmlXPathObjectPtr xpathObj;

                std::string nodeval;
                char * nodeval_C;
                
                xpath << "/" << root_node_name << "/domain[@id='" << domain << "']/key[@id='" << key << "']";
                xpathObj = xml_execute_xpath_expression (doc,
                              BAD_CAST (xpath. str (). c_str ()),
                              BAD_CAST ("mcs=http://beep-media-player.org/ns/mcs"));

                if (!xpathObj)
                  {
                    mkey.unset ();
                    continue;
                  }

                if (!xpathObj->nodesetval)
                  {
                    mkey.unset ();
                    continue;
                  }

                if (!xpathObj->nodesetval->nodeNr)
                  {
                    mkey.unset ();
                    continue;
                  }

                if (xpathObj->nodesetval->nodeNr > 1) //XXX: There can only be ONE node!
                  {
                    mkey.unset ();
                    continue;
                  }

                if (!xpathObj->nodesetval->nodeTab[0]->children)
                  {
                    mkey.unset ();
                    continue;
                  }

                nodeval_C = reinterpret_cast<char*>(XML_GET_CONTENT(xpathObj->nodesetval->nodeTab[0]->children));
                nodeval = nodeval_C;
                g_free (nodeval_C);

                switch (type)
                {
                  case KEY_TYPE_INT:
                      variant = boost::lexical_cast<int>(nodeval);
                      break;

                  case KEY_TYPE_BOOL:
                      variant = (!nodeval. compare ("TRUE")) ? true : false;
                      break;

                  case KEY_TYPE_STRING:
                      variant = std::string(nodeval);
                      break;

                  case KEY_TYPE_FLOAT:
                      variant = boost::lexical_cast<double>(nodeval);
                      break;

                } // switch (type)	

              mkey.set_value (variant);
              xmlXPathFreeObject (xpathObj);
          }
        } 

        //xmlFreeDoc (doc);
      }

      bool
      Mcs::domain_key_exist (std::string const& domain, std::string const& key)
      {
        if (domains.find (domain) == domains.end())
            return false;
        if (domains.find (domain)->second.find (key) == domains.find (domain)->second.end ())
            return false;

        return true;
      }

      void 
      Mcs::domain_register (std::string const& domain)
      {
        g_return_if_fail (domains.find (domain) == domains.end());
		domains[domain] = MKeys();
      }

      void
      Mcs::key_register (std::string const& domain, //Must be registered
                         std::string const& key,
                         KeyVariant const& key_default)
      {
      	g_return_if_fail(domains.find(domain) != domains.end());
      	domains.find (domain)->second[key] = Key( domain, key, key_default, KeyType(key_default.which()) );
      }

      void 
      Mcs::key_unset (std::string const& domain,
                      std::string const& key)
      {
        g_return_if_fail (domain_key_exist(domain, key));
        return domains.find(domain)->second.find(key)->second.unset ();
      } 

      void 
      Mcs::subscribe (std::string const& name,   //Must be unique
                      std::string const& domain, //Must be registered 
                      std::string const& key,    //Must be registered,
                      SubscriberNotify const& notify)
      {
        g_return_if_fail (domain_key_exist(domain, key));
        return domains.find (domain)->second.find (key)->second.add_subscriber(name, notify);
      }

      void 
      Mcs::unsubscribe (std::string const& name,   //Must be unique
                        std::string const& domain, //Must be registered 
                        std::string const& key)    //Must be registered,
                        
      {
        g_return_if_fail (domain_key_exist(domain, key));
        return domains.find(domain)->second.find(key)->second.remove_subscriber(name);
      }
}
