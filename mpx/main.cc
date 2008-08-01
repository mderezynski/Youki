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
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glibmm/miscutils.h>
#include <giomm.h>
#include <gst/gst.h>
#include <gtkmm/main.h>
#include <gtkglmm.h>
#include <clutter/clutter.h>

#include "mpx.hh"

#include "mpx/mpx-covers.hh"
#ifdef HAVE_HAL
#include "mpx/mpx-hal.hh"
#endif // HAVE_HAL
#include "mpx/mpx-library.hh"
#include "mpx/mpx-main.hh"
#include "mpx/metadatareader-taglib.hh"
#include "mpx/mpx-network.hh"
#include "play.hh"
#include "mpx/mpx-services.hh"
#include "mpx/mpx-types.hh"

#include "mpx/util-file.hh"

using namespace MPX;

namespace MPX
{
	Mcs::Mcs         * mcs      = 0;
    Service::Manager * services = 0; 
}

namespace 
{
    Gtk::Main * gtk = 0;
    Glib::StaticMutex gtkLock;
    
    namespace
    {
      const int SIGNAL_CHECK_INTERVAL = 1000;

      typedef void (*SignalHandler) (int);

      int terminate = 0;

      SignalHandler
      install_handler_full (int           signal_number,
                            SignalHandler handler,
                            int*          signals_to_block,
                            std::size_t   n_signals)
      {
          struct sigaction action, old_action;

          action.sa_handler = handler;
          action.sa_flags = SA_RESTART;

          sigemptyset (&action.sa_mask);

          for (std::size_t i = 0; i < n_signals; i++)
              sigaddset (&action.sa_mask, signals_to_block[i]);

          if (sigaction (signal_number, &action, &old_action) == -1)
          {
              g_warning("%s: Failed to install handler for signal %d", G_STRLOC, signal_number);
              return 0;
          }

          return old_action.sa_handler;
      }

      // A version of signal() that works more reliably across different
      // platforms. It:
      // a. restarts interrupted system calls
      // b. does not reset the handler
      // c. blocks the same signal within the handler
      //
      // (adapted from Unix Network Programming Vol. 1)

      static SignalHandler
      install_handler (int           signal_number,
                       SignalHandler handler)
      {
          return install_handler_full (signal_number, handler, NULL, 0);
      }

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
    } // anonymous namespace

    void
    signal_handlers_install ()
    {
        install_handler (SIGPIPE, empty_handler);
        install_handler (SIGSEGV, sigsegv_handler);
        install_handler (SIGINT, sigterm_handler);
        install_handler (SIGTERM, sigterm_handler);

        Glib::signal_timeout ().connect( sigc::ptr_fun (&process_signals), SIGNAL_CHECK_INTERVAL );
    }

	void
	setup_mcs ()
	{
	  try{
            MPX::mcs = new Mcs::Mcs (Glib::build_filename (Glib::build_filename (Glib::get_user_config_dir (), "mpx"), "config.xml"), "mpx", 0.01);
	  } catch( Mcs::Mcs::Exceptions & cxe )
	  {
            if (cxe == Mcs::Mcs::PARSE_ERROR)
            {
                g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, _("Unable to parse configuration file!"));
            }
	  }

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
      mcs->key_register ("mpx", "window-h", 5000);

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

	  mcs->domain_register ("playlist");
	  mcs->key_register ("playlist", "rootpath", Glib::get_home_dir ());

	  mcs->domain_register ("library");
	  mcs->key_register ("library", "rootpath", std::string (Glib::build_filename (Glib::get_home_dir (), "Music")));

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
      mcs->key_register("Preferences-CoverArtSources", "SourceActive0", bool(true));
      mcs->key_register("Preferences-CoverArtSources", "SourceActive1", bool(true));
      mcs->key_register("Preferences-CoverArtSources", "SourceActive2", bool(true));
      mcs->key_register("Preferences-CoverArtSources", "SourceActive3", bool(true));

      mcs->domain_register("PlaybackSourceMusicLib");
      mcs->key_register("PlaybackSourceMusicLib", "divider-position", 250);

	  mcs->load (Mcs::Mcs::VERSION_IGNORE);
	}
} // anonymous namespace

int 
main (int argc, char ** argv)
{
    using namespace Glib;

    Glib::thread_init(0);
    Glib::init();
    Gio::init();

    signal_handlers_install ();

    setup_mcs ();

    g_mkdir(build_filename(g_get_user_data_dir(), "mpx").c_str(), 0700);
    g_mkdir(build_filename(g_get_user_cache_dir(), "mpx").c_str(), 0700);
    g_mkdir(build_filename(g_get_user_config_dir(), "mpx").c_str(), 0700);

    GOptionContext * context_c = g_option_context_new (_(" - run AudioSource Player"));
    //g_option_context_add_main_entries (context_c, options, "mpx");
    g_option_context_add_group (context_c, gst_init_get_option_group ());
    OptionContext context (context_c, true);

    try{
        gtk = new Gtk::Main (argc, argv, context);
    } catch( OptionError & cxe )
    {
        g_warning("%s: %s", G_STRLOC, cxe.what().c_str());
        std::exit(EXIT_FAILURE);
    }

    gst_init(&argc, &argv);
	clutter_init(&argc, &argv);
    Gtk::GL::init(argc, argv);

    MPX::services = new Service::Manager;

#ifdef HAVE_HAL
    try{
        boost::shared_ptr<HAL>
            ptr_halobj
            (new MPX::HAL(*services));
        services->add(ptr_halobj);
#endif
 //       MPX::NM *obj_netman = new MPX::NM;

        boost::shared_ptr<Covers>
            ptr_covers
            (new MPX::Covers(*services));
        services->add(ptr_covers);

        boost::shared_ptr<MetadataReaderTagLib>
            ptr_taglib
            (new MPX::MetadataReaderTagLib(*services));
        services->add(ptr_taglib);

#ifdef HAVE_HAL
        boost::shared_ptr<Library>
            ptr_library
            (new MPX::Library(*services, true));
        services->add(ptr_library);
#else
        boost::shared_ptr<Library>
            ptr_library
            (new MPX::Library(*services));
        services->add(ptr_library);
#endif

        boost::shared_ptr<Play>
            ptr_play
            (new MPX::Play(*services));
        services->add(ptr_play);

        boost::shared_ptr<Player>
            ptr_player
            (MPX::Player::create(*services));
        services->add(ptr_player);

        Glib::signal_idle().connect( sigc::bind_return( sigc::mem_fun( gtkLock, &Glib::StaticMutex::unlock ), false ) );

        gtkLock.lock();
        gtk->run (*ptr_player.get());

//        delete obj_mpx;
//        delete obj_library;
//        delete obj_covers;
//        delete ptr_taglib;
//        delete obj_netman;
#ifdef HAVE_HAL
//        delete obj_hal;
      }
    catch( HAL::NotInitializedError )
      {
      }
#endif

    delete gtk;
	delete mcs;

    return EXIT_SUCCESS;
}
