#ifndef MCS_KEY_H
#define MCS_KEY_H

#include <glibmm.h>
#include <boost/variant.hpp>

#include <mcs/types.h>
#include <mcs/subscriber.h>

namespace Mcs
{
    class Key
    {
      public:

        Key (std::string const& domain,
             std::string const& key,
             KeyVariant  const& key_default,
             KeyType	          key_type);
        Key () {};
        ~Key () {};
      
        void add_subscriber (std::string const& name,  SubscriberNotify notify);  
        void remove_subscriber (std::string const& name);

      private: 

        void notify_subscribers ()
        {
          for (Subscribers::iterator iter (subscribers.begin()); iter != subscribers.end(); 
            iter++->second.notify (domain, key, key_value));
        }

      public:

        template <typename T>
        void
        set_value (T const& _key_value)
        {
          key_value = _key_value; 
          notify_subscribers ();
        }
      
        template <typename T> 
        void
        set_value_silent (T const& _key_value)
        {
          key_value = _key_value; 
        }

        void push ()
        {
          notify_subscribers ();
        }

        KeyVariant get_value () const;
        KeyType get_type () const;

        void unset ();

        operator bool		 () { return boost::get<bool>(key_value); }
        operator int		 () { return boost::get<int>(key_value); }
        operator std::string () { return boost::get<std::string>(key_value); }
        operator double	 () { return boost::get<double>(key_value); }

      private:
      
        typedef std::map<std::string, Subscriber> Subscribers;
      
        std::string	domain;
        std::string	key;

        KeyVariant	key_default; 
        KeyVariant	key_value;
        KeyType     key_type;
        Subscribers subscribers;

    };
};

#endif // MCS_KEY_H
