#include <mcs/key.h>

namespace Mcs
{
    Key::Key (std::string const& domain,
              std::string const& key,
              const KeyVariant  & key_default,
              KeyType		          key_type)

        : domain	    (domain), 
          key		      (key),
          key_default	(key_default),
          key_value	  (key_default),
          key_type	  (key_type)
    {}

    void 
    Key::add_subscriber  (std::string const& name,   //Must be unique
                          SubscriberNotify   notify)
    {
      if (subscribers.find (name) != subscribers.end()) return;
      subscribers[name] = Subscriber (notify);
    }

    void 
    Key::remove_subscriber  (std::string const& name)
              
    {
      if (subscribers.find (name) == subscribers.end()) return;
      subscribers.erase(name);
    }

    KeyVariant 
    Key::get_value () const
    {
      return key_value;
    }

    void 
    Key::unset () 
    {
      return set_value (key_default);
    }

    KeyType
    Key::get_type () const
    {
      return key_type;
    }

};
