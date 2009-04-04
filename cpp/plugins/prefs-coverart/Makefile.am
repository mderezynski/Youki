libcppmod_coverartprefs_LTLIBRARIES = libcppmod-coverartprefs.la

libcppmod_coverartprefs_la_LDFLAGS = -export-dynamic -fPIC -module

libcppmod_coverartprefs_la_LIBADD =             \
        $(BOOST_LDFLAGS)                        \
        $(GLADEMM_LIBS)                         \
        $(GLIBMM_LIBS)                          \
        $(GTKMM_LIBS)                           

libcppmod_coverartprefs_la_CXXFLAGS =           \
        $(BOOST_CPPFLAGS)                       \
        $(GLADEMM_CFLAGS)                       \
        $(GLIBMM_CFLAGS)                        \
        $(GTKMM_CFLAGS)                         \
        -I$(top_srcdir)                         \
        -I$(top_srcdir)/include                 \
        -DPLUGIN_BUILD=1            


libcppmod_coverartprefs_la_SOURCES =     \
        prefs-coverart.hh                \
        prefs-coverart.cc

libcppmod_coverartprefsdir = $(PLUGIN_DIR)/cppmod
