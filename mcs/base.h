/*  MCS
 *  Copyright (C) 2005-2006 M. Derezynski 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef HAVE_BMP

#include <mcs/config.h>
#if (1 != MCS_HAVE_XML)
#  error "This MCS installation was built without XML backend support!"
#endif

#endif

#ifndef MCS_BASE_H 
#define MCS_BASE_H 

#include <boost/variant.hpp>
#include <sigc++/sigc++.h>
#include <glibmm.h>

#include <mcs/types.h>
#include <mcs/key.h>
#include <mcs/subscriber.h>

#define MCS_CB_DEFAULT_SIGNATURE \
      const ::std::string&    domain, \
      const ::std::string&    key,    \
      const ::Mcs::KeyVariant& value

namespace Mcs {

      class Mcs
      {
        public:

          enum VersionIgnore
          {
            VERSION_CHECK,      //Checks the version specified in the root node and reports a mismatch
            VERSION_IGNORE      //Ignores the version and just attempts to load the configuration 'as is'
          };

          enum Exceptions
          {
            PARSE_ERROR,
            NO_KEY,
          };

          Mcs (std::string const& xml_filename, std::string const& root_node_name, double version);
          ~Mcs ();

          void load (Mcs::VersionIgnore version_ignore = VERSION_CHECK); 
          void domain_register (std::string const& domain);
          bool domain_key_exist (std::string const& domain, std::string const& key);
          void key_register (std::string const& domain, std::string const& key, KeyVariant const& key_default);

          template <typename T>
          void 
          key_set (std::string const& domain,
                   std::string const& key, T value) 
          {
            if (!domain_key_exist (domain, key))
              throw NO_KEY;

            MKeys &keys (domains.find (domain)->second);
            Key &k (keys.find (key)->second);
            k.set_value<T>(value);
          }

          template <typename T>
          T 
          key_get (std::string const& domain,
                   std::string const& key)
          {
            if (!domain_key_exist (domain, key))
              throw NO_KEY;
            return T (domains. find (domain)->second. find (key)->second);
          }

          void key_push (std::string const& domain, std::string const& key)
          {
            if (!domain_key_exist (domain, key))
              throw NO_KEY;

            MKeys &keys (domains.find (domain)->second);
            Key &k (keys.find (key)->second);
            k.push ();
          }
          
          void key_unset (std::string const& domain,
                          std::string const& key);

          void subscribe  (std::string const& name,   //Must be unique
                           std::string const& domain, //Must be registered 
                           std::string const& key,  //Must be registered,
                           SubscriberNotify notify);  

          void unsubscribe (std::string const& name,   //Must be unique
                           std::string const& domain, //Must be registered 
                           std::string const& key); //Must be registered,

        private:

          typedef std::map<std::string /* Key Name */, Key>   MKeys;
          typedef std::map<std::string /* Domain name */, MKeys>  MDomains;

          MDomains domains;
          std::string xml_filename;
          std::string root_node_name;
          double version;
      };
};

#endif //MCS_BASE_H 
