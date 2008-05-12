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
#include "mpx/source-playlist.hh"

#define NO_IMPORT
#include <pygobject.h>
#include <pygtk/pygtk.h>

#include "mpx/covers.hh"
#include "mpx/library.hh"
#include "mpx/mpx-public.hh"
#include "mpx/types.hh"
#include "mpx/i-playbacksource.hh"
#include "mpx/paccess.hh"

namespace MPX
{
        struct MusicLibPrivate;

namespace Source
{
        class PlaybackSourceMusicLib
            :  public Glib::Object, public PlaybackSource
        {
                Glib::RefPtr<Gtk::UIManager>        m_MainUIManager;
                Glib::RefPtr<Gtk::ActionGroup>      m_MainActionGroup;
                guint                               m_UIID;
                MusicLibPrivate                   * m_Private;
                PAccess<MPX::Library>               m_Lib;
                PAccess<MPX::HAL>                   m_HAL;
                PAccess<MPX::Covers>                m_Covers;
                MPX::Player                       & m_Player;
   
                typedef int PlaylistID; 
                typedef std::map<PlaylistID, PlaybackSourcePlaylist*> PlaylistMap;

                PlaylistID                          m_PlaylistID;
                PlaylistMap                         m_Playlists;

            public:

                PlaybackSourceMusicLib (const Glib::RefPtr<Gtk::UIManager>&, MPX::Player&);
                ~PlaybackSourceMusicLib ();


                void
                check_caps ();

        
                // i-playbacksource
                virtual void
                send_metadata ();

                virtual std::string
                get_uri (); 
            
                virtual std::string
                get_type ();

                virtual bool
                play ();

                virtual bool
                go_next ();

                virtual bool
                go_prev ();

                virtual void
                stop (); 
          
                virtual void
                play_post (); 

                virtual void
                next_post (); 

                virtual void
                prev_post ();

                virtual void
                restore_context ();

                virtual Glib::RefPtr<Gdk::Pixbuf>
                get_icon ();

                virtual Gtk::Widget*
                get_ui ();

                virtual guint
                add_menu ();

                virtual PyObject*
                get_py_obj ();

                virtual std::string
                get_guid ();
    
                virtual std::string
                get_class_guid ();

                // UriHandler
                virtual UriSchemes 
                get_uri_schemes (); 

                virtual void    
                process_uri_list (Util::FileList const&, bool);

                virtual void
                post_install (); 

            private:

                void
                on_sort_column_change ();
        }; // end class PlaybackSourceMusicLib 
} // end namespace Source
} // end namespace MPX 
  
#endif
