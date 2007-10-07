//
// MPX (C) MPX Project 2007
//
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <gst/gst.h>
#include <glib/gstdio.h>
#include <glibmm/miscutils.h>
#include <gtkmm/main.h>

#include "library.hh"
#include "hal.hh"
#include "tasks.hh"
#include "util-file.hh"
#include "mpx/types.hh"

using namespace MPX;

int 
main (int argc, char ** argv)
{
    using namespace Glib;
    g_mkdir(build_filename(g_get_user_data_dir(), "mpx").c_str(), 0700);
    g_mkdir(build_filename(g_get_user_cache_dir(), "mpx").c_str(), 0700);
    g_mkdir(build_filename(g_get_user_config_dir(), "mpx").c_str(), 0700);

    gst_init(&argc, &argv);

    MPX::TaskKernel * obj_task_kernel = new MPX::TaskKernel;
    MPX::HAL * obj_hal = new MPX::HAL();
    MPX::Library * obj_library = new MPX::Library(obj_hal, obj_task_kernel);

    obj_library->scanURI (Glib::filename_to_uri( argv[1] ) );

    Gtk::Main kit (argc, argv);
    kit.run ();

    delete obj_library;
    delete obj_hal;
    delete obj_task_kernel;

    return EXIT_SUCCESS;
}
