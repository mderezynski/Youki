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
#ifndef MPX_PLAYBACK_SOURCE_PY_HH
#define MPX_PLAYBACK_SOURCE_PY_HH
#include "config.h"
#include <glib/ghash.h>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/widget.h>
#include <sigc++/signal.h>
#include <boost/format.hpp>
#include <Python.h>
#include <boost/python.hpp>
#include <vector>

#define NO_IMPORT
#include <pygobject.h>

#include "mpx/types.hh"
#include "mpx/util-file.hh"
#include "mpx/i-playbacksource.hh"
#include "mpx/mpx-public.hh"

namespace MPX
{
    class PlaybackSourcePy
		: public PlaybackSource
    {

	public:

        PlaybackSourcePy (const Glib::RefPtr<Gtk::UIManager>& ui_manager, const std::string& name, MPX::Player& player, Caps caps = C_NONE, Flags flags = F_NONE)
		: PlaybackSource(ui_manager, name, caps, flags)
		, m_Player(player)
		{
		}

        virtual ~PlaybackSourcePy ()
		{}

        virtual std::string
        get_uri () 
		{
			return std::string();
		}
    
        virtual std::string
        get_type ()
		{
			return std::string();
		}

        virtual bool
        play ()
		{
			return false;
		}

        virtual bool
        go_next ()
		{
			return false;
		}

        virtual bool
        go_prev ()
		{
			return false;
		}

        virtual void
        stop ()
		{
		}
  
        virtual void
        play_async ()
		{}

        virtual void
        go_next_async ()
		{}
  
        virtual void
        go_prev_async () 
		{}

        virtual void
        play_post ()
		{}

        virtual void
        next_post () 
		{}

        virtual void
        prev_post ()
		{}

        virtual void
        restore_context ()
		{}

        virtual void
        skipped () 
		{}

        virtual void
        segment () 
		{}

        virtual void
        buffering_done ()
		{}

		virtual void
		send_metadata ()
		{}

        virtual Glib::RefPtr<Gdk::Pixbuf>
        get_icon ()
		{
			return Glib::RefPtr<Gdk::Pixbuf>(0);
		}

        virtual Gtk::Widget*
        get_ui ()
		{
			return 0;
		}

        virtual UriSchemes 
        getSchemes ()
		{
			return UriSchemes();
		}

        virtual void    
        processURIs (Util::FileList const&) 
		{}

		MPX::Player&
		player () 
		{
			return m_Player;
		}

	private:

		MPX::Player & m_Player;

    }; // end class PlaybackSourcePy 
} // end namespace MPX 
  
#endif
