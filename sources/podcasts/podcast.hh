//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
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
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.

#ifndef MPX_PODCAST_BACKEND_HH 
#define MPX_PODCAST_BACKEND_HH

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif //HAVE_CONFIG_H

#include <vector>

#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <glibmm.h>
#include <glibmm/markup.h>

#include "mpx/minisoup.hh"
#include "mpx/util-string.hh"

#include "podcast-v2-types.hh"

namespace MPX
{
  namespace PodcastV2
  {
    class PodcastManager;

    class OPMLParser
      : public Glib::Markup::Parser
    {
      public:

        OPMLParser (PodcastMap & casts, PodcastManager & manager); 
        virtual ~OPMLParser () {};

        bool check_sanity ();

      protected:

        virtual void
        on_start_element  (Glib::Markup::ParseContext & context,
                           Glib::ustring const& elementname,
                           AttributeMap const& attributes);
        virtual void
        on_end_element    (Glib::Markup::ParseContext & context,
                           Glib::ustring const& elementname);

      private:

        PodcastMap            & m_casts;
        PodcastManager        & m_manager;

        enum Element
        {
          E_NONE	              = 0, 
          E_OPML                = 1 << 0,
          E_HEAD                = 1 << 1,
          E_BODY                = 1 << 2,
          E_OUTLINE             = 1 << 3
        };

        int m_state;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    class PodcastManager
    {
      public:
        typedef sigc::signal<void, Glib::ustring const&> SignalPodcastUpdate;

      private:
      
        struct Signals
        {
          SignalPodcastUpdate SignalUpdated;
          SignalPodcastUpdate SignalNotUpdated;
        };
    
        Signals mSignals;

      public:

        SignalPodcastUpdate&
        signal_podcast_updated ()
        {
          return mSignals.SignalUpdated;
        }

        SignalPodcastUpdate&
        signal_podcast_not_updated ()
        {
          return mSignals.SignalNotUpdated;
        }

        PodcastManager ();
        ~PodcastManager ();

        void
        save_opml             (std::string const& filename);

        void
        podcast_get_list      (PodcastList        & list);

        Podcast &
        podcast_fetch         (Glib::ustring const& uri);

        Episode &
        podcast_fetch_item    (Glib::ustring const& uri, std::string const& guid);

        void  
        podcast_insert        (Glib::ustring const& uri, std::string const& uuid = std::string());

        void
        podcast_delete        (Glib::ustring const& uri);

        void
        podcast_update        (Glib::ustring const& uri);

        void
        podcast_update_async  (Glib::ustring const& uri);

        void
        clear_pending_requests ()
        {
          m_requests.clear();
        }

      private:

        typedef std::map<Glib::ustring, Soup::RequestRefP> RequestPool;

        friend class OPMLParser;

        void
        podcast_load (Podcast & cast);

        void
        save_state ();

        PodcastMap m_casts;
        RequestPool m_requests;

        void  podcast_updated_cb (char const* data, guint size, guint status_code, Glib::ustring const& uri);
    };
  }
}
#endif
