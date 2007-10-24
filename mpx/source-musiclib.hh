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
#ifndef MPX_PLAYBACK_SOURCE__MUSICLIB_HH_HH
#define MPX_PLAYBACK_SOURCE__MUSICLIB_HH_HH
#include "config.h"
#include <glib/ghash.h>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/widget.h>
#include <sigc++/signal.h>
#include <boost/format.hpp>
#include "mpx/types.hh"
#include "playbacksource.hh"

namespace MPX
{
    class MusicLibPrivate;

    class PlaybackSourceMusicLib
        :   public PlaybackSource
    {
    
            MusicLibPrivate * m_Private;

        public:

            virtual Glib::ustring
            get_uri (); 
        
            virtual Glib::ustring
            get_type ();

            virtual GHashTable *
            get_metadata ();

            virtual bool
            play ();

            virtual bool
            go_next ();

            virtual bool
            go_prev ();

            virtual void
            stop (); 
      

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
            play_post (); 

            virtual void
            next_post (); 

            virtual void
            prev_post ();


            virtual void
            restore_context ();


            virtual void
            skipped (); 

            virtual void
            segment (); 

            virtual void
            buffering_done (); 


            virtual Glib::RefPtr<Gdk::Pixbuf>
            get_icon ();

            virtual Gtk::Widget*
            get_ui ();

    }; // end class PlaybackSourceMusicLib 
} // end namespace MPX 
  
#endif
