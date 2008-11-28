//  main.cc
//
//  Authors:
//      Milosz Derezynski <milosz@backtrace.info>
//
//  (C) 2008 MPX Project
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

#include "mpx.hh"

#include "mpx/mpx-covers.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/metadatareader-taglib.hh"
#include "mpx/mpx-network.hh"
#include "mpx/mpx-services.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-file.hh"
#ifdef HAVE_HAL
#include "mpx/mpx-hal.hh"
#endif // HAVE_HAL

#include "paths.hh"
#include "play.hh"
#include "signals.hh"

#if 0
#include <clutter/clutter.h>
#endif

#include <glib/gi18n.h>
#include <glibmm/miscutils.h>
#include <giomm.h>
#include <gst/gst.h>
#include <gtkmm/main.h>
#include <gtkglmm.h>

#include <cstdlib>
#include <string>

using namespace MPX;

namespace MPX
{
  Mcs::Mcs         * mcs      = 0;
  Mcs::Bind        * mcs_bind = 0;
  Service::Manager * services = 0;

  namespace
  {
    Gtk::Main * gtk = 0;

    const int signal_check_interval = 1000;

    int terminate = 0;

    void
    empty_handler (int signal_number)
    {
        // empty
    }

    void
    sigsegv_handler (int signal_number)
    {
#ifdef HANDLE_SIGSEGV
        std::exit (EXIT_FAILURE);
#else
        std::abort ();
#endif
    }

    void
    sigterm_handler (int signal_number)
    {
        g_atomic_int_set( &terminate, 1 );
    }

    bool
    process_signals ()
    {
        if( g_atomic_int_get(&terminate) )
        {
            g_message (_("Got SIGTERM: Exiting (It's all right!)"));

            bool gtk_running = Gtk::Main::level();

            if (gtk_running)
                Gtk::Main::quit ();
            else
                std::exit (EXIT_SUCCESS);

            return false;
        }

        return true;
    }

    void
    signal_handlers_install ()
    {
        install_handler (SIGPIPE, empty_handler);
        install_handler (SIGSEGV, sigsegv_handler);
        install_handler (SIGINT, sigterm_handler);
        install_handler (SIGTERM, sigterm_handler);

        Glib::signal_timeout ().connect( sigc::ptr_fun (&process_signals), signal_check_interval );
    }

    void
    setup_mcs ()
    {
        mcs = new Mcs::Mcs (Glib::build_filename (get_app_config_dir (), "config.xml"), "mpx", 0.01);
        mcs_bind = new Mcs::Bind(mcs);
        mcs->load (Mcs::Mcs::VERSION_IGNORE);

        mcs->domain_register ("main-window");
        mcs->key_register ("main-window", "width", 0); //FIXME:
        mcs->key_register ("main-window", "height", 0); //FIXME:
        mcs->key_register ("main-window", "pos_x", 0);
        mcs->key_register ("main-window", "pos_y", 0);

        mcs->domain_register ("ui");
        mcs->key_register ("ui", "show-statusbar", true);

        mcs->domain_register ("mpx");
        mcs->key_register ("mpx", "no-ui", false);
        mcs->key_register ("mpx", "ui-esc-trayconify", false);
        mcs->key_register ("mpx", "keep-above", false);
        mcs->key_register ("mpx", "file-chooser-close-on-open", true);
        mcs->key_register ("mpx", "file-chooser-close-on-add", false);
        mcs->key_register ("mpx", "file-chooser-path", Glib::get_home_dir ());
        mcs->key_register ("mpx", "icon-theme", std::string ("tango"));
        mcs->key_register ("mpx", "force-rgba-enable", false);
        mcs->key_register ("mpx", "display-notifications", true);
        mcs->key_register ("mpx", "no-remote", false);
        mcs->key_register ("mpx", "time-remaining", false);
        mcs->key_register ("mpx", "volume", 50);
        mcs->key_register ("mpx", "follow-current-track", false);
        mcs->key_register ("mpx", "shuffle", false);
        mcs->key_register ("mpx", "repeat", false);
        mcs->key_register ("mpx", "enable-autoplay", false);
        mcs->key_register ("mpx", "spm-listen", false);
        mcs->key_register ("mpx", "window-x", 20);
        mcs->key_register ("mpx", "window-y", 20);
        mcs->key_register ("mpx", "window-w", 400);
        mcs->key_register ("mpx", "window-h", 500);
        mcs->key_register ("mpx", "window-mlib-w", 600);
        mcs->key_register ("mpx", "window-mlib-h", 400);
        mcs->key_register ("mpx", "window-mlib-x", 100);
        mcs->key_register ("mpx", "window-mlib-y", 100);
        mcs->key_register ("mpx", "window-prefs-w", 700);
        mcs->key_register ("mpx", "window-prefs-h", 600);
        mcs->key_register ("mpx", "window-prefs-x", 120);
        mcs->key_register ("mpx", "window-prefs-y", 120);
        mcs->key_register ("mpx", "music-import-path", Glib::build_filename(Glib::get_home_dir (),"Music"));

        mcs->domain_register ("audio");
        mcs->key_register ("audio", "band0", 0.0);
        mcs->key_register ("audio", "band1", 0.0);
        mcs->key_register ("audio", "band2", 0.0);
        mcs->key_register ("audio", "band3", 0.0);
        mcs->key_register ("audio", "band4", 0.0);
        mcs->key_register ("audio", "band5", 0.0);
        mcs->key_register ("audio", "band6", 0.0);
        mcs->key_register ("audio", "band7", 0.0);
        mcs->key_register ("audio", "band8", 0.0);
        mcs->key_register ("audio", "band9", 0.0);

        mcs->key_register ("audio", "cdrom-device", std::string ("/dev/cdrom"));
        mcs->key_register ("audio", "sink", std::string (DEFAULT_SINK));
        mcs->key_register ("audio", "enable-eq", bool( false ));

#ifdef HAVE_HAL
        mcs->key_register ("audio", "hal-udi", std::string (""));
#endif // HAVE_HAL

#ifdef HAVE_ALSA
        mcs->key_register ("audio", "alsa-buffer-time", 200000);
        mcs->key_register ("audio", "device-alsa", std::string (DEFAULT_DEVICE_ALSA));
#endif // HAVE_ALSA

#ifdef HAVE_SUN
        mcs->key_register ("audio", "sun-buffer-time", 200000);
        mcs->key_register ("audio", "device-sun", std::string (DEFAULT_DEVICE_SUN));
#endif // HAVE_SUN

        mcs->key_register ("audio", "oss-buffer-time", 200000);
        mcs->key_register ("audio", "device-oss", std::string (DEFAULT_DEVICE_OSS));

        mcs->key_register ("audio", "esd-buffer-time", 200000);
        mcs->key_register ("audio", "device-esd", std::string (DEFAULT_DEVICE_ESD));

        mcs->key_register ("audio", "pulse-buffer-time", 200000);
        mcs->key_register ("audio", "pulse-device", std::string ());
        mcs->key_register ("audio", "pulse-server", std::string ());

        mcs->key_register ("audio", "jack-buffer-time", 200000);
        mcs->key_register ("audio", "jack-server", std::string ());

        mcs->key_register ("audio", "video-output", 0);

        mcs->domain_register ("lastfm");
        mcs->key_register ("lastfm", "username", std::string ());
        mcs->key_register ("lastfm", "password", std::string ());

#ifdef HAVE_OFA
        mcs->domain_register ("musicbrainz");
        mcs->key_register ("musicbrainz", "username", std::string ());
        mcs->key_register ("musicbrainz", "password", std::string ());
#endif //HAVE_OFA

        mcs->domain_register ("albums");
        mcs->key_register ("albums", "continuous-play", false);

        mcs->domain_register ("podcasts");
        mcs->key_register ("podcasts", "download-dir", Glib::get_home_dir ());
        mcs->key_register ("podcasts", "download-policy", 0);
        mcs->key_register ("podcasts", "update-interval", 0);
        mcs->key_register ("podcasts", "cache-policy", 0);

        mcs->domain_register ("radio");
        mcs->key_register ("radio", "minimal-bitrate", 96); 

        mcs->domain_register ("playlist");
        mcs->key_register ("playlist", "rootpath", Glib::get_home_dir ());

        mcs->domain_register ("library");
        mcs->key_register ("library", "rootpath", std::string (Glib::build_filename (Glib::get_home_dir (), "Music")));
        mcs->key_register ("library", "rescan-at-startup", true);
        mcs->key_register ("library", "rescan-in-intervals", true);
        mcs->key_register ("library", "rescan-interval", 30); // in minutes
        mcs->key_register ("library", "always-vacuum", false);

        mcs->domain_register ("hotkeys");
        mcs->key_register ("hotkeys", "enable", bool (true));
        mcs->key_register ("hotkeys", "system", int (1)); // GNOME Configured is default
        mcs->key_register ("hotkeys", "key-1", int (0)); // Play
        mcs->key_register ("hotkeys", "key-1-mask", int (0));
        mcs->key_register ("hotkeys", "key-2", int (0)); // Pause
        mcs->key_register ("hotkeys", "key-2-mask", int (0));
        mcs->key_register ("hotkeys", "key-3", int (0)); // Prev
        mcs->key_register ("hotkeys", "key-3-mask", int (0));
        mcs->key_register ("hotkeys", "key-4", int (0)); // Next
        mcs->key_register ("hotkeys", "key-4-mask", int (0));
        mcs->key_register ("hotkeys", "key-5", int (0)); // Stop
        mcs->key_register ("hotkeys", "key-5-mask", int (0));

        mcs->domain_register ("pyplugs");

        mcs->domain_register("Preferences-CoverArtSources");
        mcs->key_register("Preferences-CoverArtSources", "Source0", int(0));
        mcs->key_register("Preferences-CoverArtSources", "Source1", int(1));
        mcs->key_register("Preferences-CoverArtSources", "Source2", int(2));
        mcs->key_register("Preferences-CoverArtSources", "Source3", int(3));
        mcs->key_register("Preferences-CoverArtSources", "Source4", int(4));
        mcs->key_register("Preferences-CoverArtSources", "SourceActive0", bool(true));
        mcs->key_register("Preferences-CoverArtSources", "SourceActive1", bool(true));
        mcs->key_register("Preferences-CoverArtSources", "SourceActive2", bool(true));
        mcs->key_register("Preferences-CoverArtSources", "SourceActive3", bool(true));
        mcs->key_register("Preferences-CoverArtSources", "SourceActive4", bool(true));

        mcs->domain_register("PlaybackSourceMusicLib");
        mcs->key_register("PlaybackSourceMusicLib", "divider-position", 250);
    }

  } // anonymous namespace

} // MPX

int
main (int argc, char ** argv)
{
    Glib::thread_init(0);
    Glib::init();
    Gio::init();

    signal_handlers_install ();

    create_user_dirs ();

    setup_mcs ();

    GOptionContext * context_c = g_option_context_new (_(" - run AudioSource Player"));
    g_option_context_add_group (context_c, gst_init_get_option_group ());
    Glib::OptionContext context (context_c, true);

    try{
        gtk = new Gtk::Main (argc, argv, context);
    } catch( Glib::OptionError & cxe )
    {
        g_warning(G_STRLOC ": %s", cxe.what().c_str());
        std::exit(EXIT_FAILURE);
    }

    gst_init(&argc, &argv);
    //clutter_init(&argc, &argv);
    Gtk::GL::init(argc, argv);

    services = new Service::Manager;

#ifdef HAVE_HAL
/*    try{*/
        boost::shared_ptr<HAL> ptr_halobj
            (new MPX::HAL(*services)); services->add(ptr_halobj);
#endif
        //       MPX::NM *obj_netman = new MPX::NM;

        boost::shared_ptr<Covers> ptr_covers
            (new MPX::Covers(*services));
        services->add(ptr_covers);

        boost::shared_ptr<MetadataReaderTagLib> ptr_taglib
            (new MPX::MetadataReaderTagLib(*services));
        services->add(ptr_taglib);

#ifdef HAVE_HAL
        boost::shared_ptr<Library> ptr_library
            (new MPX::Library(*services, true));
        services->add(ptr_library);
#else
        boost::shared_ptr<Library> ptr_library
            (new MPX::Library(*services));
        services->add(ptr_library);
#endif

        boost::shared_ptr<Play> ptr_play
            (new MPX::Play(*services));
        services->add(ptr_play);

        boost::shared_ptr<Player> ptr_player
            (MPX::Player::create(*services));
        services->add(ptr_player);

        gtk->run (*ptr_player.get());

#ifdef HAVE_HAL
    /*
    }
    catch( HAL::NotInitializedError& cxe )
    {
        g_message("%s: Critical! HAL Error: %s", G_STRLOC, cxe.what());
    }
    */
#endif

#ifdef HAVE_HAL
    ptr_halobj.reset();
#endif
    ptr_covers.reset();
    ptr_taglib.reset();
    ptr_library.reset();
    ptr_play.reset();
    ptr_player.reset();

    delete services;
    delete gtk;
    delete mcs;

    return EXIT_SUCCESS;
}
