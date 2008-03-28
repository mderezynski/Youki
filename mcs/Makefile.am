AUTOMAKE_OPTIONS = foreign

noinst_LTLIBRARIES = libmconfig.la

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include $(GLIB_CFLAGS) $(GLIBMM_CFLAGS) $(GTKMM_CFLAGS)

libmconfig_la_LDFLAGS  = -export-dynamic -fPIC -no-undefined #-export-symbols $(top_srcdir)/libmconfig/libmconfig.sym


libmconfig_la_LIBADD   = 	\
		  $(GLIB_LIBS)				\
		  $(GLIBMM_LIBS)			\
		  $(GTKMM_LIBS)

libmconfig_la_SOURCES  = \
		  subscriber.cpp \
		  key.cpp

INCLUDES += $(XML_CFLAGS)

libmconfig_la_LIBADD  += \
		  $(XML_LIBS)

libmconfig_la_SOURCES += \
		  mcs.cpp 

INCLUDES += $(GTK_CFLAGS) $(GTKMM_CFLAGS)

libmconfig_la_LIBADD  += \
		  $(GTK_LIBS) \
		  $(GTKMM_LIBS)

libmconfig_la_SOURCES += \
		  mcs-gtk-bind.cpp

noinst_HEADERS  = \
	    mcs.h	  \
	    types.h	  \
	    subscriber.h  \
	    key.h	  \
	    base.h	  \
	    gtk-bind.h	  
