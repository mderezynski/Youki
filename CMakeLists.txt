cmake_minimum_required (VERSION 2.6)

project (audiosource)
set (VERSION "0.01")

include (CheckIncludeFile)
include (CheckIncludeFiles)
include (CheckLibraryExists)

#subdirs (
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
#)

check_include_files ("argz.h arpa/inet.h fcntl.h float.h inttypes.h langinfo.h libintl.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdio_ext.h stdlib.h string.h strings.h sys/param.h sys/socket.h sys/time.h unistd.h wchar.h" HAVE_MISC_HEADERS)

check_library_exists (socket connect "" HAVE_SOCKET_LIB)
check_library_exists (nsl getrpcbyname "" HAVE_NSL)

find_package (Gettext)

# Profilling
option (ENABLE_PROFILE "Profiling support [DEVELOPERS only] (default=disabled)" "NO")
mark_as_advanced (FORCE ENABLE_PROFILE)

if (ENABLE_PROFILE)
  set (C_COMMON_FLAGS "${C_COMMON_FLAGS} -pg -fno-inline")
endif (ENABLE_PROFILE)

# Debugging
#AC_DEFINE([WITH_DEBUG], [], [Building with debug information])

option (ENABLE_DEBUG "Enable Debugging support [DEVELOPERS only] (default=disabled)" "NO")
mark_as_advanced (FORCE ENABLE_DEBUG)

if (ENABLE_DEBUG)
  set (C_COMMON_FLAGS "${C_COMMON_FLAGS} -ggdb3")
  set (CFLAGS "${CFLAGS} -ggdb3")
endif (ENABLE_DEBUG)

# CFLAGS/CXXFLAGS
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  # NOTE: We cannot use -pedantic with g++ because GST headers ()@#!@#
  # adds extra commas after the last item in enums
  set (C_COMMON_FLAGS "${C_COMMON_FLAGS} -fno-strict-aliasing -fmessage-length=0 -Wall -D_FORTIFY_SOURCE=2")
  set (CFLAGS "${CFLAGS} -std=c99 -pedantic")
else (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set (C_COMMON_FLAGS "${C_COMMON_FLAGS} -Wall -D_FORTIFY_SOURCE=2")
endif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

find_package (X11)

find_package (Boost)
#AX_BOOST_BASE([1.33.1])
#AX_BOOST_FILESYSTEM
#AX_BOOST_REGEX
#AX_BOOST_IOSTREAMS
#AX_BOOST_PYTHON

# C++ TR1 extensions
#AC_DEFINE([HAVE_TR1], [], [Define when having C++ tr1 extensions])

set (USE_TR1 "YES")
if (USE_TR1)
  include (${PROJECT_SOURCE_DIR}/cmake/TestForTR1.cmake)

  if (NOT TR1_FOUND)
    message (FATAL_ERROR "C++ TR1 extensions not found")
  endif (NOT TR1_FOUND)
endif (USE_TR1)

find_package (PkgConfig)

find_package (PythonLibs)

pkg_check_modules (PYGTK REQUIRED pygtk-2.0>=2.12.0)
message (STATUS "checking for pygtk defs")
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=defsdir pygtk-2.0
   OUTPUT_VARIABLE PYGTK_DEFSDIR)
message (STATUS "  pygtk defs path at ${PYGTK_DEFSDIR}")

pkg_check_modules (XML REQUIRED libxml-2.0>=2.6.1)

pkg_check_modules (SOUP REQUIRED libsoup-2.4>=2.2.10)

pkg_check_modules (SQLITE REQUIRED sqlite3>=3.4)

pkg_check_modules (GLIB   REQUIRED glib-2.0>=2.10.0 gobject-2.0 gthread-2.0 gmodule-2.0)
pkg_check_modules (GLIBMM REQUIRED glibmm-2.4>=2.8.3)
pkg_check_modules (SIGC   REQUIRED sigc++-2.0)

# AM_GLIB_GNU_GETTEXT

#pkg_check_modules (CLUTTER    REQUIRED clutter-0.6 clutter-cairo-0.6 clutter-gtk-0.6)
pkg_check_modules (GIOMM      REQUIRED giomm-2.4)
pkg_check_modules (GTK        REQUIRED gtk+-2.0>=2.10.0)
pkg_check_modules (GTKMM      REQUIRED gtkmm-2.4>=2.6.0)
pkg_check_modules (LIBGLADEMM REQUIRED libglademm-2.4>=2.6.0)
pkg_check_modules (CAIROMM    REQUIRED cairomm-1.0>=0.6.0)
pkg_check_modules (SEXYMM     REQUIRED libsexymm>=0.1.9)

# Network Manager
#AC_DEFINE([HAVE_NM], [], [Network Manager support])

option (ENABLE_NM "With GNOME NetworkManager" "NO")

if (ENABLE_NM)
  pkg_check_modules (NM REQUIRED NetworkManager>=0.6.0)
endif (ENABLE_NM)

# Xerces C
include (${PROJECT_SOURCE_DIR}/cmake/FindXercesC.cmake)
if (NOT XERCESC_FOUND)
  message (FATAL_ERROR "Xerces not found")
endif (NOT XERCESC_FOUND)

pkg_check_modules (GST              REQUIRED gstreamer-0.10>=0.10.14 gstreamer-base-0.10>=0.10.14)
pkg_check_modules (GST_PLUGINS_BASE REQUIRED gstreamer-plugins-base-0.10>=0.10.9)

pkg_check_modules (DBUS REQUIRED dbus-1>=0.93 dbus-glib-1>=0.61)
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=prefix dbus-1
  OUTPUT_VARIABLE DEFAULT_DBUS_PREFIX)

option (DBUS_SERVICES_DIR "D-BUS services file destination dir" "${DEFAULT_DBUS_PREFIX}/share/dbus-1/services")

find_program (DBUSBINDINGTOOL "dbus-binding-tool")

execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=glib_genmarshal glib-2.0
  OUTPUT_VARIABLE GLIB_GENMARSHAL)


# HAL
#AC_DEFINE([HAVE_HAL], [], [Define if building with HAL support])
#AC_DEFINE([HAVE_HAL_058], [], [Defined when building with HAL >= 0.5.8.1])

option (ENABLE_HAL "Storage management with HAL (default=enabled)" "YES")

if (ENABLE_HAL)
  pkg_check_modules (HAL REQUIRED hal>=0.5.7.1 hal-storage>=0.5.7.1)
  include (${PROJECT_SOURCE_DIR}/cmake/TestForHALPSI.cmake)

  pkg_check_modules (HAL_058 hal>=0.5.8.1 hal-storage>=0.5.8.1)
endif (ENABLE_HAL)

pkg_check_modules (TAGLIB REQUIRED taglib-gio>=1.4)

# libsidplay (v1.x)
#AC_DEFINE([HAVE_SID], [1], [have sid])

option (ENABLE_SIDPLAY ".sid/.psid taglib support" "NO")

if (ENABLE_SIDPLAY)
  include (${PROJECT_SOURCE_DIR}/cmake/FindSIDPlay.cmake)
  if (NOT SIDPLAY_FOUND)
    message (FATAL_ERROR "libsidplay 1.x not found!")
  endif (NOT SIDPLAY_FOUND)
endif (ENABLE_SIDPLAY)

# ModPlug check
#AC_DEFINE([HAVE_MOD], [1], [have mod])

option (ENABLE_MOD "modplug taglib support" "NO")

if (ENABLE_MOD)
  pkg_check_modules (MOD REQUIRED libmodplug>=0.7)
endif (ENABLE_MOD)

# SMlib/ICE
#AC_DEFINE([HAVE_SM], [1], [Define if building with SMlib support])

option (ENABLE_SM "Session management support with SMlib (default=enabled" "YES")

if (ENABLE_SM)
  include (${PROJECT_SOURCE_DIR}/cmake/FindSMlib.cmake)
  if (NOT SM_FOUND)
    message (FATAL_ERROR "Cannot find SMlib")
  endif (NOT SM_FOUND)
endif (ENABLE_SM)

# Startup Notifications
#AC_DEFINE([HAVE_STARTUP_NOTIFICATION], [], [Define when building with libstartup-notification])

option (ENABLE_SN "Enable Startup Notification (default=enabled)" "YES")

if (ENABLE_SN)
  pkg_check_modules (STARTUP_NOTIFY REQUIRED libstartup-notification-1.0>=0.8)
  set (ENABLE_SN_DESKTOP "YES")
else (ENABLE_SN)
  set (ENABLE_SN_DESKTOP "NO")
endif (ENABLE_SN)

# Zip
find_program (ZIP "zip")
if (NOT ZIP)
  set (HAVE_ZIP "NO")
endif (NOT ZIP)

pkg_check_modules (LIBOFA REQUIRED libofa>=0.9.3)

pkg_check_modules (GDL REQUIRED gdl-1.0)

pkg_check_modules (SIGX REQUIRED sigx-2.0)

# GTKGLEXT
pkg_check_modules (GTKGLEXT REQUIRED gtkglextmm-1.2)

# GAMIN
check_library_exists (fam FAMOpen "" HAVE_FAM)
check_library_exists (fam FAMNoExists "" HAVE_FAMNOEXISTS)
check_include_file (fam.h HAVE_FAM_H)

# Multimedia keys
pkg_check_modules (XPROTO xproto>=7.0.10)

set (CFLAGS "${CFLAGS} ${C_COMMON_FLAGS}")
set (CXXFLAGS "${CXXFLAGS} ${C_COMMON_FLAGS}")

configure_file (mpx.1.in mpx.1)

