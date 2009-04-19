ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = dist-bzip2
DISTCHECK_CONFIGURE_FLAGS = --with-dbus-services-dir=/tmp

# Youki 
info_backtrace_youki_appdir = $(DBUS_SERVICES_DIR)
info_backtrace_youki_app_in_files = info.backtrace.youki.app.service.in
info_backtrace_youki_app_DATA = info.backtrace.youki.app.service
$(info_backtrace_youki_app_DATA): $(info_backtrace_youki_app_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

# Sentinel
info_backtrace_youki_sentineldir = $(DBUS_SERVICES_DIR)
info_backtrace_youki_sentinel_in_files = info.backtrace.youki.sentinel.service.in
info_backtrace_youki_sentinel_DATA = info.backtrace.youki.sentinel.service
$(info_backtrace_youki_sentinel_DATA): $(info_backtrace_youki_sentinel_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

# MLibMan
info_backtrace_youki_mlibmandir = $(DBUS_SERVICES_DIR)
info_backtrace_youki_mlibman_in_files = info.backtrace.youki.mlibman.service.in
info_backtrace_youki_mlibman_DATA = info.backtrace.youki.mlibman.service
$(info_backtrace_youki_mlibman_DATA): $(info_backtrace_youki_mlibman_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

CLEANFILES =  $(info_backtrace_youki_app_DATA)          \
              $(info_backtrace_youki_sentinel_DATA)     \
              $(info_backtrace_youki_mlibman_DATA)   

DISTCLEANFILES = $(CLEANFILES) intltool-extract intltool-merge intltool-update doltlibtool doltcompile

SUBDIRS =			\
	po				\
	scripts			\
                    \
	desktop			\
	glade			\
    dbus            \
                    \
	icons			\
	images			\
    themes          \
                    \
	include			\
	xsd				\
	xmlcpp			
                    
if HAVE_HAL
SUBDIRS += libhal++
endif

SUBDIRS +=          \
                    \
	musicbrainz		\
	widgets			\
	com				\
    api             \
	taglib		    \
    json            \
	jnetlib		    \
	mcs			    \
				    \
    mpxlib          \
	src             \
	remote		    \
    sentinel        \
	python		    \
    cpp    
			
man_MANS = mpx.1

EXTRA_DIST =                                        \
                                                    \
        autogen.sh                                  \
        mpx.1.in                                    \
        info.backtrace.youki.app.service.in         \
        info.backtrace.youki.sentinel.service.in    \
        info.backtrace.youki.mlibman.service.in
