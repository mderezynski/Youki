CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(audiosource)
SET(PACKAGE "audiosource")
SET(PACKAGE_VERSION "0.01")

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)

#SUBDIRS(
  #scripts
  #desktop
  #glade
  #icons
  #images
  #po
  #include
  #xsd
  #widgets
  #musicbrainz
  #com
  #xmlcpp
  #taglib

  #if HAVE_HAL
  #endif

  #jnetlib
  #mcs
  #remote
  #python
  #sources
  #api
  #mpx
#)

CHECK_INCLUDE_FILES(
  argz.h
  arpa/inet.h
  fcntl.h
  float.h
  inttypes.h
  langinfo.h
  libintl.h
  limits.h
  locale.h
  malloc.h
  netdb.h
  netinet/in.h
  stddef.h
  stdint.h
  stdio_ext.h
  stdlib.h
  string.h
  strings.h
  sys/param.h
  sys/socket.h
  sys/time.h
  unistd.h
  wchar.h
  HAVE_MISC_HEADERS
)

CHECK_LIBRARY_EXISTS(socket connect "" HAVE_SOCKET_LIB)
CHECK_LIBRARY_EXISTS(nsl getrpcbyname "" HAVE_NSL)

FIND_PACKAGE(Gettext)

# Profilling
OPTION(ENABLE_PROFILE "Profiling support [DEVELOPERS only] (default=disabled)" "NO")
MARK_AS_ADVANCED(ENABLE_PROFILE)

IF(ENABLE_PROFILE)
  SET(C_COMMON_FLAGS "${C_COMMON_FLAGS} -pg -fno-inline")
ENDIF(ENABLE_PROFILE)

# Debugging
OPTION(ENABLE_DEBUG "Enable Debugging support [DEVELOPERS only] (default=disabled)" "NO")
MARK_AS_ADVANCED(ENABLE_DEBUG)

IF(ENABLE_DEBUG)
  SET(C_COMMON_FLAGS "${C_COMMON_FLAGS} -ggdb3")
  SET(CFLAGS "${CFLAGS} -ggdb3")
ENDIF(ENABLE_DEBUG)

# CFLAGS/CXXFLAGS
IF(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  # NOTE: We cannot use -pedantic with g++ because GST headers ()@#!@#
  # adds extra commas after the last item in enums
  SET(C_COMMON_FLAGS "${C_COMMON_FLAGS} -fno-strict-aliasing -fmessage-length=0 -Wall -D_FORTIFY_SOURCE=2")
  SET(CFLAGS "${CFLAGS} -std=c99 -pedantic")
ELSE(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  SET(C_COMMON_FLAGS "${C_COMMON_FLAGS} -Wall -D_FORTIFY_SOURCE=2")
ENDIF(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

FIND_PACKAGE(Boost 1.33.1 COMPONENTS filesystem regex iostreams python)

# C++ TR1 extensions

SET(USE_TR1 "YES")
IF(USE_TR1)
  FIND_PACKAGE(CxxTR1)
  IF(NOT TR1_FOUND)
    MESSAGE(FATAL_ERROR "C++ TR1 extensions not found")
  ENDIF(NOT TR1_FOUND)
ENDIF(USE_TR1)

FIND_PACKAGE(PkgConfig)

FIND_PACKAGE(X11)

# AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(SIGC       REQUIRED sigc++-2.0)
PKG_CHECK_MODULES(SIGX       REQUIRED sigx-2.0)
PKG_CHECK_MODULES(GLIB       REQUIRED glib-2.0>=2.10.0 gobject-2.0 gthread-2.0 gmodule-2.0)
PKG_CHECK_MODULES(GLIBMM     REQUIRED glibmm-2.4>=2.8.3)
PKG_CHECK_MODULES(GIOMM      REQUIRED giomm-2.4)
PKG_CHECK_MODULES(GTK        REQUIRED gtk+-2.0>=2.10.0)
PKG_CHECK_MODULES(GTKMM      REQUIRED gtkmm-2.4>=2.6.0)
PKG_CHECK_MODULES(GTKGLEXT   REQUIRED gtkglextmm-1.2)
PKG_CHECK_MODULES(LIBGLADEMM REQUIRED libglademm-2.4>=2.6.0)
PKG_CHECK_MODULES(CAIROMM    REQUIRED cairomm-1.0>=0.6.0)
PKG_CHECK_MODULES(SEXYMM     REQUIRED libsexymm>=0.1.9)
#PKG_CHECK_MODULES(CLUTTER   REQUIRED clutter-0.6 clutter-cairo-0.6 clutter-gtk-0.6)
PKG_CHECK_MODULES(XML        REQUIRED libxml-2.0>=2.6.1)
PKG_CHECK_MODULES(SOUP       REQUIRED libsoup-2.4>=2.2.10)
PKG_CHECK_MODULES(SQLITE     REQUIRED sqlite3>=3.4)
PKG_CHECK_MODULES(OFA        REQUIRED libofa>=0.9.3)
PKG_CHECK_MODULES(GDL        REQUIRED gdl-1.0)

# Python
FIND_PACKAGE(PythonLibs)

# PyGTK
PKG_CHECK_MODULES(PYGTK REQUIRED pygtk-2.0>=2.12.0)
MESSAGE(STATUS "checking for pygtk defs")
EXECUTE_PROCESS(
  COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=defsdir pygtk-2.0
  OUTPUT_VARIABLE PYGTK_DEFSDIR
)
MESSAGE(STATUS "  pygtk defs path at ${PYGTK_DEFSDIR}")

# Network Manager
OPTION(ENABLE_NM "With GNOME NetworkManager" "NO")

IF(ENABLE_NM)
  PKG_CHECK_MODULES(NM REQUIRED NetworkManager>=0.6.0)
ENDIF(ENABLE_NM)

# Xerces C
FIND_PACKAGE(XercesC)
IF(NOT XERCESC_FOUND)
  MESSAGE(FATAL_ERROR "Xerces not found")
ENDIF(NOT XERCESC_FOUND)

PKG_CHECK_MODULES(GST              REQUIRED gstreamer-0.10>=0.10.14 gstreamer-base-0.10>=0.10.14)
PKG_CHECK_MODULES(GST_PLUGINS_BASE REQUIRED gstreamer-plugins-base-0.10>=0.10.9)

# D-Bus

OPTION(DBUS_SERVICES_DIR "D-BUS services file destination dir" "${DEFAULT_DBUS_PREFIX}/share/dbus-1/services")

PKG_CHECK_MODULES(DBUS REQUIRED dbus-1>=0.93 dbus-glib-1>=0.61)
EXECUTE_PROCESS(
  COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=prefix dbus-1
  OUTPUT_VARIABLE DEFAULT_DBUS_PREFIX
)

FIND_PROGRAM(DBUSBINDINGTOOL "dbus-binding-tool")

EXECUTE_PROCESS(
  COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=glib_genmarshal glib-2.0
  OUTPUT_VARIABLE GLIB_GENMARSHAL
)

# FIXME: Temporarily placed here
ADD_SUBDIRECTORY(musicbrainz)
ADD_SUBDIRECTORY(xmlcpp)

# HAL
OPTION(ENABLE_HAL "Storage management with HAL (default=enabled)" "YES")

IF(ENABLE_HAL)
  FIND_PACKAGE(HAL)
  IF(NOT HAL_FOUND)
    MESSAGE(FATAL_ERROR "HAL not found")
  ENDIF(NOT HAL_FOUND)

  ADD_SUBDIRECTORY(libhal++)

  SET(HAVE_HAL        ${HAL_FOUND})
  SET(HAVE_HAL_058    ${HAL_058_FOUND})
  SET(HAVE_HAL_NEWPSI ${HAL_PSI_FOUND})
ENDIF(ENABLE_HAL)

PKG_CHECK_MODULES(TAGLIB REQUIRED taglib-gio>=1.4)

# libsidplay (v1.x)
OPTION(ENABLE_SIDPLAY ".sid/.psid taglib support" "NO")
IF(ENABLE_SIDPLAY)
  FIND_PACKAGE(SIDPlay)
  IF(NOT SIDPLAY_FOUND)
    MESSAGE(FATAL_ERROR "libsidplay 1.x not found!")
  ENDIF(NOT SIDPLAY_FOUND)
ENDIF(ENABLE_SIDPLAY)

# ModPlug check
OPTION(ENABLE_MOD "modplug taglib support" "NO")
IF(ENABLE_MOD)
  PKG_CHECK_MODULES(MOD REQUIRED libmodplug>=0.7)
ENDIF(ENABLE_MOD)

# SMlib/ICE
OPTION(ENABLE_SM "Session management support with SMlib (default=enabled" "YES")
IF(ENABLE_SM)
  FIND_PACKAGE(SMlib)
  IF(NOT SM_FOUND)
    MESSAGE(FATAL_ERROR "Cannot find SMlib")
  ENDIF(NOT SM_FOUND)
ENDIF(ENABLE_SM)

# Startup Notifications
OPTION(ENABLE_SN "Enable Startup Notification (default=enabled)" "YES")
IF(ENABLE_SN)
  PKG_CHECK_MODULES(STARTUP_NOTIFY REQUIRED libstartup-notification-1.0>=0.8)
  SET(ENABLE_SN_DESKTOP "YES")
ELSE(ENABLE_SN)
  SET(ENABLE_SN_DESKTOP "NO")
ENDIF(ENABLE_SN)

# Zip
FIND_PROGRAM(ZIP "zip")
IF(NOT ZIP)
  SET(HAVE_ZIP "NO")
ENDIF(NOT ZIP)

# GAMIN
CHECK_LIBRARY_EXISTS(fam FAMOpen "" HAVE_FAM)
CHECK_LIBRARY_EXISTS(fam FAMNoExists "" HAVE_FAMNOEXISTS)
CHECK_INCLUDE_FILE(fam.h HAVE_FAM_H)

# FIXME: Determine default sink to use depending on target system
SET(DEFAULT_SINK "autoaudiosink")
SET(DEFAULT_DEVICE "/dev/dsp")

SET(DEFAULT_DEVICE_OSS  "/dev/dsp0")
SET(DEFAULT_DEVICE_ALSA "default")
SET(DEFAULT_DEVICE_SUN  "/dev/audio")
SET(DEFAULT_DEVICE_ESD  "localhost")

# CDDA
FIND_PACKAGE(CDSupport)
IF(NOT CDIO_FOUND AND NOT CDDA_FOUND)
  MESSAGE(FATAL_ERROR "No CDDA libraries found")
ENDIF(NOT CDIO_FOUND AND NOT CDDA_FOUND)

# Multimedia keys
PKG_CHECK_MODULES(XPROTO xproto>=7.0.10)

# TODO: Turn these into options
SET(MPX_PREFIX      ${CMAKE_INSTALL_PREFIX})
SET(MPX_EXEC_PREFIX ${CMAKE_INSTALL_PREFIX})
SET(MPX_INCLUDEDIR  ${CMAKE_INSTALL_PREFIX}/include)
SET(MPX_LIBDIR      ${CMAKE_INSTALL_PREFIX}/lib) # fix this for 64-bit platforms
SET(MPX_LIBEXECDIR  ${CMAKE_INSTALL_PREFIX}/libexec)
SET(PLUGIN_DIR      ${CMAKE_INSTALL_LIBDIR}/${PACKAGE}/plugins)
SET(LOCALE_DIR      ${CMAKE_INSTALL_LIBDIR}/share/locale)
SET(DATA_DIR        ${CMAKE_INSTALL_PREFIX}/share/${PACKAGE})

SET(MPX_DBUS_SERVICE_ "\"org.mpris.mpx\"")
SET(MPX_DBUS_INTERFACE__MPRIS_ "\"org.freedesktop.MediaPlayer\"")
SET(MPX_DBUS_PATH__MPRIS_ROOT_ "\"/\"")
SET(MPX_DBUS_PATH__MPRIS_PLAYER_ "\"/Player\"")
SET(MPX_DBUS_PATH__MPRIS_TRACKLIST_ "\"/TrackList\"")

SET(MPX_DBUS_INTERFACE__MPX_ "\"info.backtrace.mpx\"")
SET(MPX_DBUS_PATH__MPX_ "\"/MPX\"")

SET(PLUGIN_VERSION "10")

SET(HAVE_BMP "YES")

SET(CFLAGS "${CFLAGS} ${C_COMMON_FLAGS}")
SET(CXXFLAGS "${CXXFLAGS} ${C_COMMON_FLAGS}")

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/mpx.1.in ${PROJECT_BINARY_DIR}/mpx.1)
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/mpx.pc.in ${PROJECT_BINARY_DIR}/mpx.pc)
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/info.backtrace.mpx.service.in ${PROJECT_BINARY_DIR}/info.backtrace.mpx.service)
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/info.backtrace.sentinel.service.in ${PROJECT_BINARY_DIR}/info.backtrace.sentinel.service)
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/desktop/mpx.desktop.in ${PROJECT_BINARY_DIR}/desktop/mpx.desktop)
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/config.h.cmake ${PROJECT_BINARY_DIR}/config.h)
