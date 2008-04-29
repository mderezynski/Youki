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
#ifndef MPX_HH
#define MPX_HH
#include "config.h"
#include <giomm.h>
#include <gtkmm.h>
#include <gtkmm/volumebutton.h>
#include <libglademm/xml.h>
#include <dbus/dbus-glib.h>
#include <boost/python.hpp>

#include "mpx/covers.hh"
#ifdef HAVE_HAL
#include "mpx/hal.hh"
#endif // HAVE_HAL
#include "mpx/i-playbacksource.hh"
#include "mpx/library.hh"
#include "mpx/paccess.hh"
#include "mpx/util-file.hh"
#include "mpx/widgetloader.h"

#include "audio-types.hh"
#include "video-widget.hh"
#include <gdk/gdkx.h>

#include "dbus-marshalers.h"

using namespace Gnome::Glade;

namespace Gtk
{
    class Statusbar;
}

namespace MPX
{
    class ActionState
    {
      public:

        ActionState(Glib::RefPtr<Gtk::UIManager> & manager, bool state)
        : m_Manager(manager)
        , m_State(state)
        {
        }

        void
        operator()(std::string const& action)
        {
            m_Manager->get_action(action)->set_sensitive(m_State);
        }

      private:

        Glib::RefPtr<Gtk::UIManager> & m_Manager;
        bool m_State;
    };


    class ActionSet
    {
        public:

            ActionSet(Glib::RefPtr<Gtk::UIManager> & manager)
            : m_Manager(manager)
            {
            }

            void
            add (std::string const& name)
            {
                m_ActionNames.push_back(name);
            }

            void
            set_sensitive(bool state)
            {
               std::for_each(m_ActionNames.begin(), m_ActionNames.end(), ActionState(m_Manager, state)); 
            }

        private:

            std::vector<std::string> m_ActionNames;
            Glib::RefPtr<Gtk::UIManager> & m_Manager;
    };


    class InfoArea;
    class MLibManager;
    class Play;
    class PluginManager;
    class PluginManagerGUI;
    class Preferences;
    class Sources;

    class Player
      : public WidgetLoader<Gtk::Window>
    {
      public:

		void
		add_widget (Gtk::Widget*);

        void
        add_info_widget(Gtk::Widget*, std::string const&);

		void
		remove_widget (Gtk::Widget*);

		void
		remove_info_widget (Gtk::Widget*);

		void
		get_object (PAccess<MPX::Library>&);

		void	
		get_object (PAccess<MPX::Covers>&);

        void
        get_object (PAccess<MPX::Play>&);

#ifdef HAVE_HAL
        void
        get_object (PAccess<MPX::HAL>&);
#endif // HAVE_HAL

        PyObject* 
        get_source(std::string const& uuid);

		Metadata const& 
		get_metadata ();

        MPXPlaystatus
        get_status ();

		void
		play ();

		void
		pause ();
    
        void
        pause_ext ();

		void
		prev ();

		void
		next ();

		void
		stop ();

        void
        play_uri (std::string const&);

        virtual ~Player ();

      protected:

#ifdef HAVE_HAL
        Player (const Glib::RefPtr<Gnome::Glade::Xml>&, MPX::Library&, MPX::Covers&, MPX::HAL&);
#else
        Player (const Glib::RefPtr<Gnome::Glade::Xml>&, MPX::Library&, MPX::Covers&);
#endif // HAVE_HAL

	  public:

#ifdef HAVE_HAL
        static Player*
        create (MPX::Library&, MPX::Covers&, MPX::HAL&);
#else
        static Player*
        create (MPX::Library&, MPX::Covers&);
#endif // HAVE_HAL

		class DBusRoot; 
		class DBusMPX; 
        class DBusPlayer;
        class DBusTrackList;

        friend class DBusMPX;

		void
		init_dbus ();

      private:

		enum PlayerCSignals
		{
			PSIGNAL_NEW_TRACK,
			PSIGNAL_TRACK_PLAYED,
			PSIGNAL_INFOAREA_CLICK,
            PSIGNAL_STATUS_CHANGED,
			N_SIGNALS
		};

	    guint signals[N_SIGNALS];

        struct SourcePlugin
        {
			PlaybackSource*	(*get_instance)       (const Glib::RefPtr<Gtk::UIManager>&, MPX::Player&);
			void			(*del_instance)       (MPX::PlaybackSource*);
        };

        typedef boost::shared_ptr<SourcePlugin> SourcePluginPtr;
        typedef std::vector<SourcePluginPtr> SourcePluginsKeeper;
		typedef std::vector<PlaybackSource*> VectorSources;
		typedef std::map<std::string, int> UriSchemeMap;
		typedef std::vector<int> SourceTabMapping;

        Glib::RefPtr<Gnome::Glade::Xml> m_ref_xml;
        Glib::RefPtr<Gtk::ActionGroup>  m_actions;
        Glib::RefPtr<Gtk::UIManager>    m_ui_manager;

		struct DBusObjectsT
		{
			DBusRoot        *root;
			DBusMPX		    *mpx;
            DBusPlayer      *player;
            DBusTrackList   *tracklist;
		};
		DBusObjectsT DBusObjects;
		DBusGConnection * m_SessionBus;
        bool m_startup_complete;


		int m_Seeking;
		gdouble m_TrackPlayedSeconds;
		gdouble m_PreSeekPosition;
		gdouble m_TrackDuration;

		int m_SourceCtr;
		int m_PageCtr;
        int m_ActiveSource;
		Glib::Mutex m_SourceCFLock;
        SourceTabMapping m_SourceTabMapping;
		guint m_SourceUI;
		UriSchemeMap m_UriMap;
        SourcePluginsKeeper m_SourcePlugins;

		Play *m_Play;
        Library &m_Library;
        Covers &m_Covers;
        HAL &m_HAL;
		PluginManager *m_PluginManager;

        Preferences *m_Preferences;
        MLibManager *m_MLibManager;
		PluginManagerGUI *m_PluginManagerGUI;
        Sources *m_Sources;
        InfoArea *m_InfoArea;
        VideoWindow *m_VideoWindow; 

        Gtk::Statusbar *m_Statusbar;
		Gtk::Notebook *m_MainNotebook;
        Gtk::Notebook *m_OuterNotebook;
		Gtk::VolumeButton *m_Volume;
		Gtk::HScale *m_Seek;
		Gtk::Label *m_TimeLabel;
        Gtk::Notebook *m_InfoNotebook;
        Gtk::Expander *m_InfoExpander;

		VectorSources m_SourceV;
		PlaybackSource::Flags m_source_flags[16];
		PlaybackSource::Caps m_source_caps[16];

		boost::optional<Metadata> m_Metadata;
        Glib::Mutex m_MetadataLock;
		Glib::RefPtr<Gdk::Pixbuf> m_DiscDefault;

        ActionSet * m_StopActions;
        ActionSet * m_ControlActions; 

		void
		on_cover_clicked ();

		void
		on_show_plugins ();

		void
		on_play_files ();

        void
        on_info_toggled();

		void
		on_sources_toggled ();

        void
        on_video_toggled ();

		void
		on_volume_value_changed(double);

        void
        on_lastfm_love_track ();



		bool
		on_seek_event(GdkEvent*);

		bool
		on_seek_button_press(GdkEventButton*);

		bool
		on_seek_button_release(GdkEventButton*);



        void
        on_library_scan_start();

        void
        on_library_scan_run(gint64,gint64);

        void
        on_library_scan_end(gint64,gint64,gint64,gint64,gint64);

		

		void
		on_play_status_changed ();

		void
		on_play_position (guint64);

		void
		on_play_metadata (MPXGstMetadataField);

		void
		reparse_metadata ();




		void
		on_controls_play ();

		void
		on_controls_pause ();

		void
		on_controls_next ();

		void
		on_controls_prev ();


		
		void
		on_source_changed (int source_id);

		void
		on_source_caps (PlaybackSource::Caps, int);

		void
		on_source_flags (PlaybackSource::Flags, int);

		void
		on_source_track_metadata (Metadata const&, int);

		void
		on_source_play_request (int);

		void
		on_source_segment (int);

		void
		on_source_stop (int);

		void
		on_source_next (int);

		void
		play_async_cb (int);

		void
		play_post_internal (int);

		void
		safe_pause_unset ();

		void
		on_play_eos ();

		void
		prev_async_cb (int);

		void
		next_async_cb (int);



        ::Window
        on_gst_set_window_id ();

        void
        on_gst_set_window_geom (int width, int height, GValue const* par);



		void
		install_source (int source_id, int tab);

		bool
		load_source_plugin (std::string const& path);

        void
        play_tracks (Util::FileList const& uris);

		void
		track_played ();





        enum grab_type
        {
          NONE = 0,
          SETTINGS_DAEMON,
          X_KEY_GRAB
        };

        DBusGProxy * m_mmkeys_dbusproxy;
        grab_type m_mmkeys_grab_type;

        static void
        media_player_key_pressed (DBusGProxy *proxy,
                                  const gchar *application,
                                  const gchar *key,
                                  gpointer data);

        bool
        window_focus_cb (GdkEventFocus *event);

        void grab_mmkey (int key_code,
                         int mask,
                         GdkWindow *root);
        void grab_mmkey (int key_code,
                         GdkWindow *root);
        void ungrab_mmkeys (GdkWindow *root);
        static GdkFilterReturn
        filter_mmkeys (GdkXEvent *xevent,
                       GdkEvent *event,
                       gpointer data);
        void
        mmkeys_grab (bool grab);

        void
        mmkeys_get_offending_modifiers ();

        guint m_capslock_mask, m_numlock_mask, m_scrolllock_mask;

        void
        mmkeys_activate ();

        void 
        mmkeys_deactivate ();

        void
        on_mm_edit_begin (); 

        void
        on_mm_edit_done (); 

        bool mm_active;
        sigc::connection mWindowFocusConn;



        Glib::RefPtr<Gio::File> m_MountFile;
        Glib::RefPtr<Gio::MountOperation> m_MountOperation;
        Glib::ustring m_Share, m_ShareName;

        void
        mount_ready_callback (Glib::RefPtr<Gio::AsyncResult>&);

        void
        unmount_ready_callback (Glib::RefPtr<Gio::AsyncResult>&);

        void
        ask_password_cb (const Glib::ustring& message,
                         const Glib::ustring& default_user,
                         const Glib::ustring& default_domain,
                         Gio::AskPasswordFlags flags);

        void
        on_import_folder();

        void
        on_import_share();

      protected:

        virtual bool
        on_key_press_event (GdkEventKey*);
    };
}
#endif
