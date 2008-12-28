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


#include <glib/gi18n.h>
#include <glibmm/miscutils.h>
#include <giomm.h>
#include <gst/gst.h>
#include <gtkmm/main.h>
#include <gtkglmm.h>
#include <cstdlib>
#include <string>

#include "mpx.hh"
#include "paths.hh"
#include "signals.hh"

#include "mpx/mpx-artist-images.hh"
#include "mpx/mpx-covers.hh"
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/mpx-network.hh"
#include "mpx/mpx-play.hh"
#include "mpx/mpx-preferences.hh"
#include "mpx/mpx-services.hh"
#include "mpx/mpx-types.hh"
#include "mpx/util-file.hh"
#ifdef HAVE_HAL
#include "mpx/mpx-hal.hh"
#endif // HAVE_HAL

#include "mlibmanager.hh"

#include "mpx/metadatareader-taglib.hh"

#include "mpx/com/mb-import-album.hh"
#include "mpx/mpx-markov-analyzer-thread.hh"

#include "plugin.hh"
#include "plugin-manager-gui.hh"

#include "splash-screen.hh"

using namespace MPX;
using namespace Glib;

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
        mcs->key_register ("mpx", "window-plugins-w", 500);
        mcs->key_register ("mpx", "window-plugins-h", 400);
        mcs->key_register ("mpx", "window-plugins-x", 140);
        mcs->key_register ("mpx", "window-plugins-y", 140);
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

        mcs->domain_register ("musicbrainz");
        mcs->key_register ("musicbrainz", "username", std::string ());
        mcs->key_register ("musicbrainz", "password", std::string ());

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
#ifdef HAVE_HAL
        mcs->key_register ("library", "use-hal", true);
#endif // HAVE_HAL

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

        mcs->domain_register("Preferences-FileFormatPriorities");
        mcs->key_register("Preferences-FileFormatPriorities", "Format0", std::string("audio/x-flac")); 
        mcs->key_register("Preferences-FileFormatPriorities", "Format1", std::string("audio/x-ape"));
        mcs->key_register("Preferences-FileFormatPriorities", "Format2", std::string("audio/x-vorbis+ogg"));
        mcs->key_register("Preferences-FileFormatPriorities", "Format3", std::string("audio/x-musepack"));
        mcs->key_register("Preferences-FileFormatPriorities", "Format4", std::string("audio/mp4"));
        mcs->key_register("Preferences-FileFormatPriorities", "Format5", std::string("audio/mpeg"));
        mcs->key_register("Preferences-FileFormatPriorities", "Format6", std::string("audio/x-ms-wma"));
        mcs->key_register("Preferences-FileFormatPriorities", "prioritize-by-filetype", false); 
        mcs->key_register("Preferences-FileFormatPriorities", "prioritize-by-bitrate", false); 

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

    Splashscreen * splash = new Splashscreen;

    splash->set_message(_("Starting Services..."), 0.5);

    services = new Service::Manager;

#ifdef HAVE_HAL
    try{
        services->add(boost::shared_ptr<HAL>(new MPX::HAL));
#endif //HAVE_HAL
        services->add(boost::shared_ptr<Covers>(new MPX::Covers));
        services->add(boost::shared_ptr<MetadataReaderTagLib>(new MPX::MetadataReaderTagLib));
        services->add(boost::shared_ptr<Library>(new MPX::Library));
        services->add(boost::shared_ptr<ArtistImages>(new MPX::ArtistImages));
        services->add(boost::shared_ptr<MarkovAnalyzer>(new MarkovAnalyzer));
        services->add(boost::shared_ptr<Play>(new MPX::Play));
        services->add(boost::shared_ptr<Preferences>(MPX::Preferences::create()));
#ifdef HAVE_HAL
        services->add(boost::shared_ptr<MLibManager>(MPX::MLibManager::create()));
#endif // HAVE_HAL
        services->add(boost::shared_ptr<MB_ImportAlbum>(MPX::MB_ImportAlbum::create()));
        services->add(boost::shared_ptr<Player>(MPX::Player::create()));
        services->add(boost::shared_ptr<PluginManager>(new MPX::PluginManager));
        services->add(boost::shared_ptr<PluginManagerGUI>(MPX::PluginManagerGUI::create()));

        splash->set_message(_("Done"), 1.0);
        delete splash;

        gtk->run (*(services->get<Player>("mpx-service-player").get()));

#ifdef HAVE_HAL
    }
    catch( HAL::NotInitializedError& cxe )
    {
        g_message("%s: Critical! HAL Error: %s", G_STRLOC, cxe.what());
    }
#endif

    delete services;
    delete gtk;
    delete mcs;

    return EXIT_SUCCESS;
}

