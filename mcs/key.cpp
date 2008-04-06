#include <mcs/key.h>
#include <glib.h>

namespace Mcs
{
    Key::Key (std::string const& domain,
              std::string const& key,
              KeyVariant  const& key_default,
              KeyType		     key_type)

	: domain(domain) 
    , key(key)
    , key_default(key_default)
    , key_value(key_default)
    , key_type(key_type)
    {
	}

	Key::Key ()
	{
	}

	Key::~Key ()
	{
	}

    void 
    Key::add_subscriber(std::string const& name, SubscriberNotify const& notify)
    {
      g_return_if_fail(subscribers.find (name) == subscribers.end());
      subscribers[name] = Subscriber(notify);
    }

    void 
    Key::remove_subscriber(std::string const& name)
              
    {
      g_return_if_fail (subscribers.find (name) != subscribers.end());
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
}
