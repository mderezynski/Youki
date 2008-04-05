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
#include <gtkmm/widget.h>
#include <gtkmm/uimanager.h>
#include <sigc++/signal.h>
#include <boost/format.hpp>
#include <Python.h>
#include <boost/python.hpp>
#include <vector>

#define NO_IMPORT
#include <pygobject.h>

#include "mpx/types.hh"
#include "mpx/util-file.hh"

namespace MPX
{
	typedef std::vector<std::string> UriSchemes;

    struct Metadata
    :   public Track
    {
        Glib::RefPtr<Gdk::Pixbuf> Image;
		PyObject *PyImage;

		PyObject*	
		get_image ()
		{
			if(!Image)
				return NULL;

			PyImage = pygobject_new((GObject*)(Image->gobj()));
			return PyImage; 
		}

		Metadata ()
		{
		}

		Metadata (Track const& track)
		: Track (track)
		{
		}
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
        typedef sigc::signal<void>                          SignalSegment;
        typedef sigc::signal<void>                          SignalPlaybackRequest;
        typedef sigc::signal<void>                          SignalStopRequest;
        typedef sigc::signal<void>                          SignalNextRequest;
        typedef sigc::signal<void>                          SignalPlayAsync;
        typedef sigc::signal<void>                          SignalNextAsync;
        typedef sigc::signal<void>                          SignalPrevAsync;

        PlaybackSource (const Glib::RefPtr<Gtk::UIManager>&, const std::string&, Caps = C_NONE, Flags = F_NONE);
		virtual ~PlaybackSource ();

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

        SignalNextAsync&
        signal_next_async ();

        SignalPrevAsync&
        signal_prev_async ();

        SignalPlayAsync&
        signal_play_async ();

        virtual std::string
        get_uri () = 0; 
    
        virtual std::string
        get_type ();

        virtual bool
        play () = 0;

        virtual bool
        go_next () = 0;

        virtual bool
        go_prev () = 0; 

        virtual void
        stop () = 0;
  
        virtual void
        play_async ();

        virtual void
        go_next_async ();
  
        virtual void
        go_prev_async (); 

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

		virtual void
		send_metadata () = 0;

        std::string
        get_name ();

        virtual Glib::RefPtr<Gdk::Pixbuf>
        get_icon () = 0;

        virtual Gtk::Widget*
        get_ui () = 0;

		virtual guint
		add_menu ();

        virtual UriSchemes 
        Get_Schemes ();

        virtual void    
        Process_URI_List (Util::FileList const&); 

      protected:

        struct SignalsT
        {
          SignalCaps                      Caps;
          SignalFlags                     Flags;
          SignalTrackMetadata             Metadata;
          SignalSegment                   Segment;
          SignalPlaybackRequest           PlayRequest;
          SignalStopRequest               StopRequest;
          SignalNextRequest               NextRequest;
          SignalPlayAsync                 PlayAsync;
          SignalNextAsync                 NextAsync;
          SignalPrevAsync                 PrevAsync;
        };

        SignalsT        Signals;
        Caps            m_Caps;
        Flags           m_Flags;
        std::string     m_Name;

    }; // end class PlaybackSource 

    typedef PlaybackSource::Caps SourceCaps;
    typedef PlaybackSource::Flags SourceFlags;

    inline SourceCaps operator|(SourceCaps lhs, SourceCaps rhs)
      { return static_cast<SourceCaps>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

    inline SourceFlags operator|(SourceFlags lhs, SourceFlags rhs)
      { return static_cast<SourceFlags>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

} // end namespace MPX 
  
#endif
