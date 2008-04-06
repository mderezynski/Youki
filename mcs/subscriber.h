#ifndef MCS_SUBSCRIBER_H
#define MCS_SUBSCRIBER_H

#include <sigc++/sigc++.h>
#include <mcs/types.h>

namespace Mcs
{
  class Subscriber
  {
    public:

      Subscriber  (SubscriberNotify const& notify);
	  Subscriber  ();
	  ~Subscriber ();

      void
	  notify (const std::string& domain, const std::string& key, const KeyVariant& key_value);

    private:

      SubscriberNotify m_notify;
  };
};

#endif // MCS_SUBSCRIBER_H
