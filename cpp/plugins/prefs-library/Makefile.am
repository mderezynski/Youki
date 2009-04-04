libcppmod_libraryprefs_LTLIBRARIES = libcppmod-libraryprefs.la

libcppmod_libraryprefs_la_LDFLAGS = -export-dynamic -fPIC -module

libcppmod_libraryprefs_la_LIBADD =              \
        $(ALSA_LIBS)                            \
        $(GST_LIBS)                             \
        $(BOOST_LDFLAGS)                        \
        $(DBUS_LIBS)                            \
        $(GLADEMM_LIBS)                         \
        $(GLIBMM_LIBS)                          \
        $(HAL_LIBS)                             \
        $(GTKMM_LIBS)                           \
        $(SOUP_LIBS)                            \
        $(XLIB_LIBS)                            

libcppmod_libraryprefs_la_CXXFLAGS =            \
        $(ALSA_CFLAGS)                          \
        $(GST_CFLAGS)                           \
        $(BOOST_CPPFLAGS)                       \
        $(DBUS_CFLAGS)                          \
        $(GLADEMM_CFLAGS)                       \
        $(GLIBMM_CFLAGS)                        \
        $(GTKMM_CFLAGS)                         \
        $(HAL_CFLAGS)                           \
        $(SOUP_CFLAGS)                          \
        $(X_CFLAGS)                             \
        $(XPROTO_CFLAGS)                        \
        -I$(top_srcdir)                         \
        -I$(top_srcdir)/include                 \
        -I$(top_srcdir)/libhal++                \
        -DPLUGIN_BUILD=1            


libcppmod_libraryprefs_la_SOURCES =     \
        prefs-library.hh                \
        prefs-library.cc

libcppmod_libraryprefsdir = $(PLUGIN_DIR)/cppmod
