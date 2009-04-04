libcppmod_mmkeys_LTLIBRARIES = libcppmod-mmkeys.la

libcppmod_mmkeys_la_LDFLAGS = -export-dynamic -fPIC -module

libcppmod_mmkeys_la_LIBADD =                    \
        $(BOOST_LDFLAGS)                        \
        $(DBUS_LIBS)                            \
        $(GLADEMM_LIBS)                         \
        $(GLIBMM_LIBS)                          \
        $(GTKMM_LIBS)                           \
        $(XLIB_LIBS)                            

libcppmod_mmkeys_la_CXXFLAGS =                  \
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


libcppmod_mmkeys_la_SOURCES =   \
        mmkeys.hh               \
        mmkeys.cc

libcppmod_mmkeysdir = $(PLUGIN_DIR)/cppmod
