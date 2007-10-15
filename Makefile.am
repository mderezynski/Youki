ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = dist-bzip2
DISTCHECK_CONFIGURE_FLAGS = --with-dbus-services-dir=/tmp --enable-hal --enable-mp4v2 --enable-sid --enable-ofa

# Main Service
servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.beepmediaplayer.mpx.service.in
service_DATA = org.beepmediaplayer.mpx.service
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

# Sentinel
sentineldir = $(DBUS_SERVICES_DIR)
sentinel_in_files = org.beepmediaplayer.mpxsentinel.service.in
sentinel_DATA = org.beepmediaplayer.mpxsentinel.service
$(sentinel_DATA): $(sentinel_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

CLEANFILES =  $(service_DATA) \
              $(sentinel_DATA) 

DISTCLEANFILES = $(CLEANFILES) intltool-extract intltool-merge intltool-update

SUBDIRS =		\
	glade		\
	icons		\
	include		\
	po			\
	taglib		\
	widgets		\
	jnetlib		

if HAVE_HAL
SUBDIRS += libhal++
endif

SUBDIRS += mpx

man_MANS = mpx.1

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = mpx.pc

EXTRA_DIST =									\
												\
	autogen.sh									\
												\
	org.beepmediaplayer.mpx.service.in			\
	org.beepmediaplayer.mpxsentinel.service.in	\
												\
	mpx.1.in									\
	mpx.pc.in									\
												\
	intltool-extract.in							\
	intltool-merge.in							\
	intltool-update.in
