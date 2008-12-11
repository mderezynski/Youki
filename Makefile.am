ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = dist-bzip2
DISTCHECK_CONFIGURE_FLAGS = --with-dbus-services-dir=/tmp --enable-hal

# Main Service
servicedir = $(DBUS_SERVICES_DIR)
service_in_files = info.backtrace.mpx.service.in
service_DATA = info.backtrace.mpx.service
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

# Sentinel
sentineldir = $(DBUS_SERVICES_DIR)
sentinel_in_files = info.backtrace.sentinel.service.in
sentinel_DATA = info.backtrace.sentinel.service
$(sentinel_DATA): $(sentinel_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

CLEANFILES =  $(service_DATA) \
              $(sentinel_DATA) 

DISTCLEANFILES = $(CLEANFILES) intltool-extract intltool-merge intltool-update

SUBDIRS =			\
	scripts			\
	desktop			\
	glade			\
	icons			\
	images			\
					\
	po				\
					\
	boost			\
	include			\
	xsd				\
	widgets			\
	musicbrainz		\
	com				\
	xmlcpp			\
	taglib

if HAVE_HAL
SUBDIRS += libhal++
endif

SUBDIRS +=		\
				\
	jnetlib		\
	mcs			\
				\
	remote		\
	python		\
	sources		\
				\
	api			\
	mpx

man_MANS = mpx.1

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = mpx.pc

EXTRA_DIST =									\
												\
	autogen.sh									\
												\
	info.backtrace.mpx.service.in				\
	info.backtrace.sentinel.service.in			\
												\
	mpx.1.in									\
	mpx.pc.in									\
												\
	intltool-extract.in							\
	intltool-merge.in							\
	intltool-update.in
