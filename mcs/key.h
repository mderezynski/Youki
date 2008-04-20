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
             KeyVariant  const& m_default,
             KeyType m_type);

		Key ();
		~Key ();

        void subscriber_add (std::string const& name, SubscriberNotify const& notify);  
        void subscriber_del (std::string const& name);

        template <typename T>
        class adaptor
        {
            public:

                adaptor (Key & key)
                : m_key(key)
                {
                }

                operator T ()
                {
                    return T(m_key);
                }

                void operator=(const T & value)
                {
                    return m_key.set_value<T>(value);
                }

            private:

                Key & m_key;
        };

      private: 

        void notify ()
        {
          for (Subscribers::iterator i = m_subscribers.begin(); i != m_subscribers.end(); i++->second.notify (m_domain, m_key, m_value));
        }

      public:

        template <typename T>
        void
        set_value (T const& val)
        {
          m_value = val; 
          notify ();
        }
      
        template <typename T> 
        void
        set_value_silent (T const& val)
        {
          m_value = val; 
        }

        void push ()
        {
          notify ();
        }

        KeyVariant get_value () const;
        KeyType get_type () const;

        void unset ();

        operator bool		 () { return boost::get<bool>(m_value); }
        operator int		 () { return boost::get<int>(m_value); }
        operator std::string () { return boost::get<std::string>(m_value); }
        operator double	     () { return boost::get<double>(m_value); }

      private:
      
        typedef std::map<std::string, Subscriber> Subscribers;
      
        std::string	m_domain;
        std::string	m_key;

        KeyVariant	m_default; 
        KeyVariant	m_value;
        KeyType     m_type;
        Subscribers m_subscribers;

    };

};

#endif // MCS_KEY_H
