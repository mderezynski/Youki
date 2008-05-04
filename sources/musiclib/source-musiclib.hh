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
        class MusicLibPrivate;

        struct PlaylistColumnsT : public Gtk::TreeModel::ColumnRecord 
        {
            Gtk::TreeModelColumn<Glib::ustring> Artist;
            Gtk::TreeModelColumn<Glib::ustring> Album;
            Gtk::TreeModelColumn<guint64> Track;
            Gtk::TreeModelColumn<Glib::ustring> Title;
            Gtk::TreeModelColumn<guint64> Length;

            // These hidden columns are used for sorting
            // They don't contain sortnames, as one might 
            // think from their name, but instead the MB
            // IDs (if not available, then just the plain name)
            // They are used only for COMPARISON FOR EQUALITY.
            // Obviously, comparing them with compare() is
            // useless if they're MB IDs

            Gtk::TreeModelColumn<Glib::ustring> ArtistSort;
            Gtk::TreeModelColumn<Glib::ustring> AlbumSort;
            Gtk::TreeModelColumn<gint64> RowId;
            Gtk::TreeModelColumn<Glib::ustring> Location;
            Gtk::TreeModelColumn< ::MPX::Track> MPXTrack;
            Gtk::TreeModelColumn<gint64> Rating;
            Gtk::TreeModelColumn<bool> IsMPXTrack;

            PlaylistColumnsT ()
            {
                add (Artist);
                add (Album);
                add (Track);
                add (Title);
                add (Length);
                add (ArtistSort);
                add (AlbumSort);
                add (RowId);
                add (Location);
                add (MPXTrack);
                add (Rating);
                add (IsMPXTrack);
            };
        };

        struct PluginMenuData
        {
            Glib::RefPtr<Gdk::Pixbuf> m_Icon;
            gint64                    m_Id;
            std::string               m_Name;
        };

        typedef std::vector<PluginMenuData> VisiblePlugsT;
        
namespace Source
{
        class PlaybackSourceMusicLib
            :  public Glib::Object, public PlaybackSource
        {
                enum CSignals
                {
                    PSM_SIGNAL_PLAYLIST_TOOLTIP,
                    PSM_SIGNAL_PLAYLIST_END,
                    N_SIGNALS
                };

        	    guint signals[N_SIGNALS];
                static bool m_signals_installed;

                Glib::RefPtr<Gtk::UIManager> m_MainUIManager;
                Glib::RefPtr<Gtk::ActionGroup> m_MainActionGroup;
                guint m_UIID;

                MusicLibPrivate * m_Private;
                VisiblePlugsT m_VisiblePlugs; 

                PAccess<MPX::Library> m_Lib;
                PAccess<MPX::HAL> m_HAL;
                PAccess<MPX::Covers> m_Covers;
    

                void
                on_sort_column_change ();

                void
                on_plist_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

                bool
                on_plist_query_tooltip(int,int,bool,const Glib::RefPtr<Gtk::Tooltip>&);

            public:

                PlaybackSourceMusicLib (const Glib::RefPtr<Gtk::UIManager>&, MPX::Player&);
                ~PlaybackSourceMusicLib ();


                Glib::RefPtr<Gtk::ListStore> 
                get_playlist_model ();

                PyObject*
                get_playlist_current_iter ();


                void
                play_album(gint64);

                void
                append_album(gint64);


                void
                play_tracks(IdV const&);

                void
                append_tracks(IdV const&);



                void
                action_cb_run_plugin (gint64);

                void
                action_cb_play ();

                void
                action_cb_go_to_album(gint64);

                void
                check_caps ();

                void
                set_play ();

                void
                clear_play ();




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

                virtual guint
                add_menu ();

                virtual PyObject*
                get_py_obj ();

                virtual std::string
                get_guid ();

                // UriHandler
                virtual UriSchemes 
                get_uri_schemes (); 

                virtual void    
                process_uri_list (Util::FileList const&, bool);

        }; // end class PlaybackSourceMusicLib 
} // end namespace Source
} // end namespace MPX 
  
#endif
