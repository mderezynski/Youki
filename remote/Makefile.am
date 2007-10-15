bin_PROGRAMS = mpx 

dist_bin_SCRIPTS  = mpx-play-uris mpx-play-files

mpx_LDADD =			\
	$(GLIBMM_LIBS)	\
	$(GTK_LIBS)		\
	$(DBUS_LIBS)

mpx_CXXFLAGS =			\
	$(GLIBMM_CFLAGS)	\
	$(GTK_CFLAGS)		\
	$(DBUS_CFLAGS)		\
						\
	-DMPX_BINARY_PATH=\"$(libexecdir)/mpx-bin\"

mpx_SOURCES =	\
    mpx.cc
