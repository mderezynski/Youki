libcppmod_audioprefs_LTLIBRARIES = libcppmod-audioprefs.la

libcppmod_audioprefs_la_LDFLAGS = -export-dynamic -fPIC -module

libcppmod_audioprefs_la_LIBADD =               \
        $(ALSA_LIBS)                            \
        $(GST_LIBS)                             \
        $(BOOST_LDFLAGS)                        \
        $(DBUS_LIBS)                            \
        $(GLADEMM_LIBS)                         \
        $(GLIBMM_LIBS)                          \
        $(GTKMM_LIBS)                           \
        $(XLIB_LIBS)                            

libcppmod_audioprefs_la_CXXFLAGS =             \
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


libcppmod_audioprefs_la_SOURCES =   \
        prefs-audio.hh               \
        prefs-audio.cc

libcppmod_audioprefsdir = $(PLUGIN_DIR)/cppmod
