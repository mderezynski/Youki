//
// MPX (C) MPX Project 2007
//
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
#include <gst/gst.h>
#include <gtkmm/main.h>

#include "library.hh"
#include "hal.hh"
#include "tasks.hh"
#include "util-file.hh"
#include "mpx/types.hh"

using namespace MPX;

namespace 
{
    Gtk::Main * gtk = 0;
    Glib::StaticMutex gtkLock;
    
    namespace
    {
      const int SIGNAL_CHECK_INTERVAL = 1000;

      typedef void (*SignalHandler) (int);

      bool terminate = false;

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
          terminate = true;
      }

      bool
      process_signals ()
      {
          if (terminate)
          {
              g_warning(_("%s: SIGTERM received, quitting."), G_STRLOC);

              gtkLock.lock ();
              bool gtk_running = Gtk::Main::level();
              gtkLock.unlock ();

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

        Glib::signal_timeout ().connect (sigc::ptr_fun (&process_signals), SIGNAL_CHECK_INTERVAL);
    }

} // anonymous namespace

int 
main (int argc, char ** argv)
{
    using namespace Glib;

    signal_handlers_install ();

    g_mkdir(build_filename(g_get_user_data_dir(), "mpx").c_str(), 0700);
    g_mkdir(build_filename(g_get_user_cache_dir(), "mpx").c_str(), 0700);
    g_mkdir(build_filename(g_get_user_config_dir(), "mpx").c_str(), 0700);

    gst_init(&argc, &argv);

    MPX::TaskKernel * obj_task_kernel = new MPX::TaskKernel;
    MPX::HAL * obj_hal = new MPX::HAL();
    MPX::Library * obj_library = new MPX::Library(obj_hal, obj_task_kernel);

    Glib::signal_idle().connect( sigc::bind_return( sigc::mem_fun( gtkLock, &Glib::StaticMutex::unlock ), false ) );

    obj_library->scanURI(filename_to_uri( argv[1] ));

    gtkLock.lock();
    gtk = new Gtk::Main (argc, argv);
    gtk->run ();

    delete gtk;
    delete obj_library;
    delete obj_hal;
    delete obj_task_kernel;

    return EXIT_SUCCESS;
}
