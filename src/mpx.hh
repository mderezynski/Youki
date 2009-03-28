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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// FIXME: Must be here for some damn reason
#include <giomm.h>

#include "mpx/mpx-audio.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-protected-access.hh"
#include "mpx/mpx-services.hh"

#include "mpx/util-file.hh"

#include "mpx/widgets/widgetloader.hh"
#include "mpx/widgets/rounded-layout.hh"

#include "mpx/i-playbacksource.hh"

#ifdef HAVE_HAL
#include "mpx/mpx-hal.hh"
#endif // HAVE_HAL

#include "sidebar.hh"
#include "video-widget.hh"

#include "mpx/mpx-audio-types.hh"
#include "dbus-marshalers.h"

#include <gtkmm.h>
#include <gtkmm/volumebutton.h>
#include <libglademm/xml.h>

#include <boost/python.hpp>
#include <dbus/dbus-glib.h>
#include <sigx/sigx.h>

namespace MPX
{
    class AboutDialog;
    class Equalizer;
    class ErrorManager;
    class InfoArea;
    class MLibManager;
    class Play;
    class Preferences;
    class VolumeControl;

    class Player
    : public Gnome::Glade::WidgetLoader<Gtk::Window>
    , public sigx::glib_auto_dispatchable
    , public Service::Base
    {
      public:

		void
		add_widget (Gtk::Widget*);

        void
        add_info_widget(Gtk::Widget*, std::string const&);

        void
        add_subsource(PlaybackSource*, ItemKey const&, gint64 id);

		void
		remove_widget (Gtk::Widget*);

		void
		remove_info_widget (Gtk::Widget*);

        PyObject* 
        get_source(std::string const& uuid);

        PyObject*
        get_sources_by_class(std::string const& uuid);

		Metadata& 
		get_metadata ();

        PlayStatus
        get_status ();

        Glib::RefPtr<Gtk::UIManager>&
        ui (); 

        GObject*
        get_gobj() ;

        void
        deactivate_plugin(gint64);

        void
        activate_plugin(gint64);

        void
        show_plugin(gint64);

        void
        push_message (const std::string&);

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

        void
        play_uri (std::string const&);
    
        virtual ~Player ();

    // XXX: Public API needed when we split off SourceController

        void
        metadata_reparse_with_lock ();

        void
        set_metadata(Metadata const&, ItemKey const&);

        void
        del_caps(Caps caps);
    
        void
        set_caps(Caps, bool = true);

        void
        translate_caps ();

      protected:

        Player (const Glib::RefPtr<Gnome::Glade::Xml>&);

	  public:

        static Player*
        create();

		class DBusRoot; 
		class DBusMPX; 
        class DBusPlayer;
        class DBusTrackList;

        friend class DBusMPX;
        friend class DBusPlayer;

		void
		init_dbus ();

        void
        info_set (const std::string&);

        void
        info_clear ();

      private:

        struct SourcePlugin
        {
			PlaybackSource*	(*get_instance)       (const Glib::RefPtr<Gtk::UIManager>&, MPX::Player&);
			void			(*del_instance)       (MPX::PlaybackSource*);
        };

        typedef boost::shared_ptr<SourcePlugin>          SourcePluginPtr;
        typedef std::vector<SourcePluginPtr>             SourcePluginsKeeper;

        SourcePluginsKeeper             m_SourcePlugins;
        FlagsMap_t                      m_source_f;
        CapsMap_t                       m_source_c;


		typedef std::map<std::string, ItemKey>           UriSchemeMap;
		typedef std::map<ItemKey, PlaybackSource*>       SourcesMap;

		UriSchemeMap                    m_UriMap;
		SourcesMap                      m_Sources;

        typedef std::map<Gtk::Widget*, Gtk::Widget*>     WidgetWidgetMap;
        typedef Caps                                     EffectiveCaps;

		struct DBusObjectsT
		{
			DBusRoot        *root;
			DBusMPX		    *mpx;
            DBusPlayer      *player;
            DBusTrackList   *tracklist;
		};

        Glib::RefPtr<Gtk::ActionGroup>  m_Actions;
        Glib::RefPtr<Gtk::UIManager>    m_UIManager;

        guint                           m_MainUI;


		DBusObjectsT                    DBusObjects;
		DBusGConnection               * m_SessionBus;
        bool                            m_startup_complete;

		int                             m_Seeking;
        int                             m_LastSeeked;
		gdouble                         m_TrackPlayedSeconds;
		gdouble                         m_TrackSeekedSeconds;
		gdouble                         m_PreSeekPosition;
		gdouble                         m_TrackDuration;

    // source related stuff

        EffectiveCaps                   m_Caps;
        gint64                          m_NextSourceId;
        boost::optional<ItemKey>        m_ActiveSource;
        boost::optional<ItemKey>        m_PreparingSource;
		Glib::Mutex                     m_SourceCFLock;
		guint                           m_SourceUI;

    // objects

		Play                          * m_Play;
        Sidebar                       * m_Sidebar;
        InfoArea                      * m_InfoArea;
        VideoWidget                   * m_VideoWidget; 

    // widgets

        Gtk::Statusbar                * m_Statusbar ;

		Gtk::Notebook                 * m_MainNotebook ;
        Gtk::Notebook                 * m_OuterNotebook ;

		Gtk::VolumeButton             * m_Volume ;
		Gtk::HScale                   * m_Seek ;
        Gtk::Label                    * m_TimeLabel ;

        Gtk::Notebook                 * m_InfoNotebook ;
        Gtk::Expander                 * m_InfoExpander ;

        WidgetWidgetMap                 m_InfoWidgetMap ;

        AboutDialog                   * m_AboutDialog ;
        Equalizer                     * m_Equalizer ;
        VolumeControl                 * m_VolumeControl ;
            
        Gtk::ToggleButton             * m_ButtonPause ;

    // metadata

		boost::optional<Metadata>       m_Metadata;
        Glib::Mutex                     m_MetadataLock;
        int                             m_NewTrack;

        enum PlayDirection
        {
            PD_NONE,
            PD_PLAY,
            PD_NEXT,
            PD_PREV,
        };

        PlayDirection                   m_PlayDirection;

    
		void
		on_cb_album_cover_clicked ();

		void
		on_action_cb_show_plugins ();

		void
		on_action_cb_play_files ();

		void
		on_volume_value_changed(double);



		bool
		on_seek_event(GdkEvent*);

		bool
		on_seek_button_press(GdkEventButton*);

		bool
		on_seek_button_release(GdkEventButton*);



		void
		on_play_status_changed ();

		void
		on_play_position (guint64);

		void
		on_play_metadata (GstMetadataField);

        void
        on_play_buffering (double);

		void
		on_play_eos ();

        void
        on_play_update_spectrum (Spectrum const& spectrum);

		void
		on_play_stream_switched ();



		void
		on_controls_play ();

		void
		on_controls_pause ();

		void
		on_controls_next ();

		void
		on_controls_prev ();


		
		void
		on_source_changed (ItemKey const&);

		void
		on_source_caps (Caps, ItemKey const&);

		void
		on_source_flags (Flags, ItemKey const&);

		void
		on_source_track_metadata (Metadata const&, ItemKey const&);

        void
        on_source_items(gint64 count, ItemKey const& key);

		void
		on_source_play_request (ItemKey const&);

		void
		on_source_segment (ItemKey const&);

		void
		on_source_stop (ItemKey const&);

		void
		play_async_cb (ItemKey const&);

		void
		prev_async_cb (ItemKey const&);

		void
		next_async_cb (ItemKey const&);

        void
        play_tracks (Util::FileList const&, bool);

        void
        switch_stream (std::string const&, std::string const&);

		void
		track_played ();
    


        ::Window
        on_gst_set_window_id ();

        void
        on_gst_set_window_geom (int width, int height, GValue const* par);



		void
		source_install (ItemKey const& source_id);

		bool
		source_load (std::string const& path);



		void
		metadata_reparse ();

        bool
        metadata_updated ();

        void
        on_got_cover(const std::string&);


        void
        check_py_error ();

        void
        on_import_album ();

      protected:

        virtual bool
        on_key_press_event (GdkEventKey*);

        virtual bool
        on_delete_event (GdkEventAny*);

      private:

    // mmkeys stuff

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

        sigc::connection mWindowFocusConn;
    };
}

#endif
