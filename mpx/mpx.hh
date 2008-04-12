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
#include "mpx/library.hh"
#include "mpx/paccess.hh"
#include "mpx/util-file.hh"
#include "mpx/widgetloader.h"

#include "audio-types.hh"
#include "play.hh"
#include "plugin.hh"
#include "plugin-manager-gui.hh"

#include "mpx/i-playbacksource.hh"

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


    class Sources;
    class InfoArea;
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
		get_object (PAccess<MPX::Library>&);

		void	
		get_object (PAccess<MPX::Covers>&);

        void
        get_object (PAccess<MPX::Play>&);

		Metadata const&
		get_metadata ();

		void
		play ();

		void
		pause ();

		void
		prev ();

		void
		next ();

		void
		stop ();

        virtual ~Player ();

      protected:

        Player (const Glib::RefPtr<Gnome::Glade::Xml>&, MPX::Library&, MPX::Covers&); //BLEH: We need to pass pointers here for the Python API

	  public:

        static Player*
        create (MPX::Library&, MPX::Covers&);

		class Root; 
		class DBusMPX; 

      protected:

        virtual bool on_key_press_event (GdkEventKey*);

      private:

		enum PlayerCSignals
		{
			PSIGNAL_NEW_TRACK,
			PSIGNAL_TRACK_PLAYED,
			PSIGNAL_INFOAREA_CLICK,
            PSIGNAL_STATE_CHANGED,
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
			Root		*root;
			DBusMPX		*mpx;
		};
		DBusObjectsT DBusObjects;
		DBusGConnection * m_SessionBus;


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
		PluginManager *m_PluginManager;

		PluginManagerGUI *m_PluginManagerGUI;
        Sources *m_Sources;
        InfoArea *m_InfoArea;
        Gtk::Statusbar *m_Statusbar;
		Gtk::Notebook *m_MainNotebook;
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
		on_infoarea_uris (Util::FileList const&);
		
		void
		on_show_plugins ();

		void
		on_play_files ();

        void
        on_show_info_toggled();

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
		on_controls_play ();

		void
		on_controls_pause ();

		void
		on_controls_next ();

		void
		on_controls_prev ();


		
		void
		on_sources_toggled ();

		void
		on_source_changed (int source_id);

		void
		install_source (int source_id, int tab);

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

		void
		track_played ();


		void
		reparse_metadata ();

        // Importing related
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



		void
		init_dbus ();

		bool
		load_source_plugin (std::string const& path);
    };
}
#endif
