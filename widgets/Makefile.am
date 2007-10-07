noinst_LTLIBRARIES = libmpxwidgets.la

libmpxwidgets_la_CFLAGS = 	\
	$(GLIB_CFLAGS) $(GTK_CFLAGS) $(SEXYMM_CFLAGS) -I$(top_srcdir)/include

libmpxwidgets_la_CXXFLAGS = \
	$(GLIBMM_CFLAGS) $(GTKMM_CFLAGS) $(SEXYMM_CFLAGS) -I$(top_srcdir)/include

libmpxwidgets_la_LIBADD =	\
	$(GLIBMM_LIBS)			\
	$(GTKMM_LIBS)					

libmpxwidgets_la_SOURCES = 		\
								\
	    widget-marshalers.c   	\
								\
		cairoextensions.cc		\
		cairoextensions.hh		\
								\
		bmp_status_icon.c     	\
		bmp_status_icon.h	    \
								\
    	bmp_tray_icon.c       	\
	    bmp_tray_icon.h	      	\
								\
		taskdialog.hh			\
		taskdialog.cc			\
								\
		cell-renderer-cairo-surface.hh	\
		cell-renderer-cairo-surface.cc	

widget-marshalers.c: widget-marshalers.list 
	@echo "* Generating widget-marshalers.c"
	@glib-genmarshal --prefix=widget --body   $(srcdir)/widget-marshalers.list > widget-marshalers.c

widget-marshalers.h: widget-marshalers.list
	@echo "* Generating widget-marshalers.h"
	@glib-genmarshal --prefix=widget --header $(srcdir)/widget-marshalers.list > widget-marshalers.h


EXTRA_DIST = \
      widget-marshalers.list

BUILT_SOURCES = \
      widget-marshalers.c \
      widget-marshalers.h

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(CLEANFILES)
