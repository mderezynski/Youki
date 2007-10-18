AUTOMAKE_OPTIONS = foreign

if HAVE_BMP
libhalcc_la_LDFLAGS = --version-info $(LIBVERSION) -release $(RELEASE)
noinst_LTLIBRARIES = libhalcc.la
else
libhalcc_la_LDFLAGS = -export-dynamic -fPIC -version-info $(LIBVERSION) -release $(RELEASE)
lib_LTLIBRARIES = libhalcc.la
endif

INCLUDES = $(SIGC_CFLAGS) $(DBUS_CFLAGS) $(HAL_CFLAGS) $(GLIB_CFLAGS)

LIBVERSION=3:0:0
RELEASE=1.0

libhalcc_la_LIBADD = $(SIGC_LIBS) $(DBUS_LIBS) $(HAL_LIBS) $(GLIB_LIBS)

libhalcc_la_SOURCES = \
    context.cc        \
    device.cc         \
    drive.cc          \
    volume.cc         \
    util.cc

HCC_COMMONHEADERS   = \
    context.hh        \
    device.hh         \
    exception.hh      \
    refptr.hh         \
    types.hh          \
                      \
    storage.hh        \
    drive.hh          \
    volume.hh         \
                      \
    hcc-macros.hh     \
    util.hh           \
    hal++.hh

if HAVE_BMP
noinst_HEADERS = $(HCC_COMMONHEADERS) 
else
apiheader_HEADERS = $(HCC_COMMONHEADERS) 
apiheaderdir = $(includedir)/hal++-1.0/hal++
endif

