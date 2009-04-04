libcppmod_audioqualityprefs_LTLIBRARIES = libcppmod-audioqualityprefs.la

libcppmod_audioqualityprefs_la_LDFLAGS = -export-dynamic -fPIC -module

libcppmod_audioqualityprefs_la_LIBADD =         \
        $(ALSA_LIBS)                            \
        $(GST_LIBS)                             \
        $(BOOST_LDFLAGS)                        \
        $(DBUS_LIBS)                            \
        $(GLADEMM_LIBS)                         \
        $(GLIBMM_LIBS)                          \
        $(GTKMM_LIBS)                           \
        $(XLIB_LIBS)                            

libcppmod_audioqualityprefs_la_CXXFLAGS =       \
        $(ALSA_CFLAGS)                          \
        $(GST_CFLAGS)                           \
        $(BOOST_CPPFLAGS)                       \
        $(DBUS_CFLAGS)                          \
        $(GLADEMM_CFLAGS)                       \
        $(GLIBMM_CFLAGS)                        \
        $(GTKMM_CFLAGS)                         \
        $(X_CFLAGS)                             \
        $(XPROTO_CFLAGS)                        \
        -I$(top_srcdir)                         \
        -I$(top_srcdir)/include                 \
        -DPLUGIN_BUILD=1            


libcppmod_audioqualityprefs_la_SOURCES =        \
        prefs-audio-quality.hh                  \
        prefs-audio-quality.cc

libcppmod_audioqualityprefsdir = $(PLUGIN_DIR)/cppmod
