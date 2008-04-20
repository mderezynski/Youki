#include <mcs/key.h>
#include <glib.h>

namespace Mcs
{
    Key::Key (std::string const& domain,
              std::string const& key,
              KeyVariant  const& key_default,
              KeyType		     key_type)

	: m_domain(domain) 
    , m_key(key)
    , m_default(key_default)
    , m_value(key_default)
    , m_type(key_type)
    {
	}

	Key::Key ()
	{
	}

	Key::~Key ()
	{
	}

    void 
    Key::subscriber_add(std::string const& name, SubscriberNotify const& notify)
    {
        g_return_if_fail(m_subscribers.find (name) == m_subscribers.end());
        m_subscribers[name] = Subscriber(notify);
    }

    void 
    Key::subscriber_del(std::string const& name)
              
    {
        g_return_if_fail (m_subscribers.find (name) != m_subscribers.end());
        m_subscribers.erase(name);
    }

    KeyVariant 
    Key::get_value () const
    {
        return m_value;
    }

    KeyType
    Key::get_type () const
    {
        return m_type;
    }

    void 
    Key::unset () 
    {
        m_value = m_default;
    }
}
