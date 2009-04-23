lib_LTLIBRARIES = libmpx-widgets.la

libmpx_widgets_la_CFLAGS = 	\
	$(CLUTTERMM_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS) $(SEXYMM_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/include/mpx

libmpx_widgets_la_CXXFLAGS = \
	$(CLUTTERMM_CFLAGS) $(SIGX_CFLAGS) $(GIOMM_CFLAGS) $(GLADEMM_CFLAGS) $(GLIBMM_CFLAGS) $(GTKMM_CFLAGS) $(SEXYMM_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/include/mpx

libmpx_widgets_la_LIBADD =	\
    $(CLUTTERMM_LIBS)       \
	$(GLIBMM_LIBS)			\
	$(GTKMM_LIBS)			\
	$(GLADEMM_LIBS)

libmpx_widgets_la_SOURCES = 			\
										\
		youki-status-icon.c    			\
		youki-status-icon.h	    		\
    	youki-tray-icon.c      			\
	    youki-tray-icon.h      			\
                                        \
        youki-clutter-widget.cc         \
										\
		cairo-extensions.cc				\
										\
		cell-renderer-album-data.cc		\
		cell-renderer-cairo-surface.cc	\
		cell-renderer-count.cc			\
		cell-renderer-expander.c		\
		cell-renderer-source.cc			\
		cell-renderer-vbox.cc			\
										\
		dialog-simple-entry.cc			\
										\
		rounded-layout.cc				\
        rounded-alignment.cc            \
										\
		task-dialog.cc					\
        tidy-actor.c                    \
        tidy-actor.h                    \
        tidy-debug.h                    \
        tidy-marshal.c                  \
        tidy-marshal.h                  \
        tidy-private.h                  \
        tidy-stylable.c                 \
        tidy-stylable.h                 \
        tidy-style.c                    \
        tidy-style.h                    \
        tidy-texture-reflection.c       \
        tidy-texture-reflection.h       \
        tidy-types.h                    \
        timed-confirmation.cc			\
										\
	    widget-marshalers.c   			

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

