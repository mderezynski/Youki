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
#include <gio/gio.h>
#include <gtkmm.h>
#include <gtkmm/volumebutton.h>
#include <libglademm/xml.h>
#include <dbus/dbus-glib.h>

#include "mpx/amazon.hh"
#include "mpx/library.hh"
#include "mpx/paccess.hh"
#include "mpx/playbacksource.hh"
#include "mpx/widgetloader.h"

#include "audio-types.hh"
#include "play.hh"

#include <boost/python.hpp>
#include "plugin.hh"

using namespace Gnome::Glade;

namespace Gtk
{
    class Statusbar;
}

namespace MPX
{
    class Sources;
    class InfoArea;
    class Player
      : public WidgetLoader<Gtk::Window>
    {
      public:

		void
		get_object (PAccess<MPX::Library> & pa);

		void	
		get_object (PAccess<MPX::Amazon::Covers> & pa);

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

        Player (const Glib::RefPtr<Gnome::Glade::Xml>&, MPX::Library&, MPX::Amazon::Covers&); //BLEH: We need to pass pointers here for the Python API

	  public:

        static Player*
        create (MPX::Library&, MPX::Amazon::Covers&);

		class Root; 
		class DBusMPX; 

      protected:

        virtual bool on_key_press_event (GdkEventKey*);

      private:

		enum PlayerCSignals
		{
			PSIGNAL_NEW_TRACK,
			PSIGNAL_INFOAREA_CLICK,
			N_SIGNALS
		};

	    guint signals[N_SIGNALS];

        struct SourcePlugin
        {
			PlaybackSource*	(*get_instance)       (MPX::Player & player);
			void			(*del_instance)       (MPX::PlaybackSource * source);
        };

        typedef boost::shared_ptr<SourcePlugin> SourcePluginPtr;
        typedef std::vector<SourcePluginPtr> SourcePluginsKeeper;
		typedef std::vector<PlaybackSource*> VectorSources;
        SourcePluginsKeeper m_SourcePlugins;

        Glib::RefPtr<Gnome::Glade::Xml>   m_ref_xml;
        Glib::RefPtr<Gtk::ActionGroup>    m_actions;
        Glib::RefPtr<Gtk::UIManager>      m_ui_manager;

		struct DBusObjectsT
		{
			Root		*root;
			DBusMPX		*mpx;
		};

		std::string m_TrackInfoScript;
		int m_Seeking;
		std::vector<int> m_SourceTabMapping;
		int m_SourceCtr;
		int m_PageCtr;

		DBusObjectsT DBusObjects;
		DBusGConnection * m_SessionBus;

		Play *m_Play;
        Library &m_Library;
        Amazon::Covers &m_Covers;
		PluginManager *m_PluginManager;

        Sources *m_Sources;
        InfoArea *m_InfoArea;

        Gtk::Statusbar *m_Statusbar;
		Gtk::Notebook *m_MainNotebook;
		Gtk::VolumeButton *m_Volume;
		Gtk::HScale *m_Seek;
		Gtk::Label *m_TimeLabel;

		VectorSources m_SourceV;
		PlaybackSource::Flags m_source_flags[16];
		PlaybackSource::Caps m_source_caps[16];

		Metadata m_Metadata;
		Glib::RefPtr<Gdk::Pixbuf> m_DiscDefault;



		bool
		load_source_plugin (std::string const& path);


		void
		on_volume_value_changed(double);

		
		void
		on_cover_clicked();



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
		on_source_track_metadata (Metadata const&);

		void
		on_source_play_request (int);

		void
		on_source_message_set (Glib::ustring const&);

		void
		on_source_message_clear ();

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
		reparse_metadata ();

		// Importing related
        GFile *m_MountFile;
        GMountOperation *m_MountOperation;
        Glib::ustring m_Share, m_ShareName;

        static void
        mount_ready_callback (GObject*,
                              GAsyncResult*,
                              gpointer);

        static void
        unmount_ready_callback (GObject*,
                              GAsyncResult*,
                              gpointer);


        static gboolean
        ask_password_cb (GMountOperation *op,
                 const char      *message,
                 const char      *default_user,
                 const char      *default_domain,
                 GAskPasswordFlags   flags);

        void
        on_import_folder();

        void
        on_import_share();

        void
        on_got_cover (Glib::ustring);

		void
		init_dbus ();
    };
}
#endif
