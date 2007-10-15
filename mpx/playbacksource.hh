//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
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
#ifndef MPX_PLAYBACK_SOURCE_HH
#define MPX_PLAYBACK_SOURCE_HH
#include "config.h"
#include <glib/ghash.h>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <sigc++/signal.h>
#include <boost/format.hpp>
#include "mpx/types.hh"

namespace MPX
{
    class Metadata
    :   public Track
    {
        Glib::RefPtr<Gdk::Pixbuf>   Image;
    };

    class PlaybackSource
    {
      public:

        enum Flags
        {
          F_NONE                    = 0,
          F_ASYNC                   = 1 << 0,
          F_HANDLE_STREAMINFO       = 1 << 1,
          F_PHONY_NEXT              = 1 << 2,
          F_PHONY_PREV              = 1 << 3,
          F_ALWAYS_IMAGE_FRAME      = 1 << 4,
          F_HANDLE_LASTFM           = 1 << 5,
          F_HANDLE_LASTFM_ACTIONS   = 1 << 6,
          F_USES_REPEAT             = 1 << 7,
          F_USES_SHUFFLE            = 1 << 8,
        };

        enum Caps
        {
          C_NONE                    = 0,
          C_CAN_GO_NEXT             = 1 << 0,
          C_CAN_GO_PREV             = 1 << 1,
          C_CAN_PAUSE               = 1 << 2,
          C_CAN_PLAY                = 1 << 3,
          C_CAN_SEEK                = 1 << 4,
          C_CAN_RESTORE_CONTEXT     = 1 << 5, 
          C_CAN_PROVIDE_METADATA    = 1 << 6,
          C_CAN_BOOKMARK            = 1 << 7,
          C_PROVIDES_TIMING         = 1 << 8,
        };

        typedef sigc::signal<void, Caps>                    SignalCaps;
        typedef sigc::signal<void, Flags>                   SignalFlags;
        typedef sigc::signal<void, const Metadata&>         SignalTrackMetadata;
        typedef sigc::signal<void, const Glib::ustring&>    SignalMessage;
        typedef sigc::signal<void>                          SignalSegment;
        typedef sigc::signal<void>                          SignalMessageClear;
        typedef sigc::signal<void>                          SignalPlaybackRequest;
        typedef sigc::signal<void>                          SignalStopRequest;
        typedef sigc::signal<void>                          SignalNextRequest;
        typedef sigc::signal<void>                          SignalPlayAsync;
        typedef sigc::signal<void>                          SignalNextAsync;
        typedef sigc::signal<void>                          SignalPrevAsync;

        PlaybackSource (const Glib::ustring& name,
                        Caps  caps  = C_NONE,
                        Flags flags = F_NONE)
        : m_caps        (caps),
          m_flags       (flags),
          m_name        (name)
        {}

        virtual ~PlaybackSource ()
        {}

        SignalCaps&
        signal_caps ();

        SignalFlags&
        signal_flags ();

        SignalTrackMetadata&
        signal_track_metadata ();

        SignalPlaybackRequest&
        signal_playback_request ();

        SignalStopRequest&
        signal_stop_request ();

        SignalNextRequest&
        signal_next_request ();

        SignalSegment&
        signal_segment ();

        SignalMessage &
        signal_message ();

        SignalMessageClear &
        signal_message_clear ();



        SignalNextAsync&
        signal_next_async ();

        SignalPrevAsync&
        signal_prev_async ();

        SignalPlayAsync&
        signal_play_async ();



        virtual Glib::ustring
        get_uri () = 0; 
    
        virtual Glib::ustring
        get_type () 
        { return Glib::ustring(); }

        virtual GHashTable *
        get_metadata ();




        virtual bool
        play ()
        { return false; } 

        virtual bool
        go_next ()
        { return false; }

        virtual bool
        go_prev ()
        { return false; } 

        virtual void
        stop () = 0;
  


        virtual void
        play_async () {}

        virtual void
        go_next_async () {}
  
        virtual void
        go_prev_async () {}



        virtual void
        play_post () = 0;

        virtual void
        next_post (); 

        virtual void
        prev_post ();


        virtual void
        restore_context () = 0;


        virtual void
        skipped (); 

        virtual void
        segment (); 

        virtual void
        buffering_done (); 



        virtual void
        send_caps ();

        virtual void
        send_flags ();



        Glib::ustring
        get_name ();

        virtual Glib::RefPtr<Gdk::Pixbuf>
        get_icon () = 0;



        std::string
        new_error   (std::exception const& cxe);

        std::string
        new_message (std::string const& message_in);


      protected:

        struct SignalsT
        {
          SignalCaps                      Caps;
          SignalFlags                     Flags;
          SignalTrackMetadata             Metadata;
          SignalSegment                   Segment;
          SignalMessage                   Message;
          SignalMessageClear              MessageClear;
          SignalPlaybackRequest           PlayRequest;
          SignalStopRequest               StopRequest;
          SignalNextRequest               NextRequest;
          SignalPlayAsync                 PlayAsync;
          SignalNextAsync                 NextAsync;
          SignalPrevAsync                 PrevAsync;
        };

        SignalsT        Signals;
        Caps            m_caps;
        Flags           m_flags;
        Glib::ustring   m_name;

    }; // end class PlaybackSource 

    typedef PlaybackSource::Caps SourceCaps;
    typedef PlaybackSource::Flags SourceFlags;

    inline SourceCaps operator|(SourceCaps lhs, SourceCaps rhs)
      { return static_cast<SourceCaps>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

    inline SourceFlags operator|(SourceFlags lhs, SourceFlags rhs)
      { return static_cast<SourceFlags>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

} // end namespace MPX 
  
#endif
