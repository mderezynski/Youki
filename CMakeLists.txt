cmake_minimum_required (VERSION 2.6)

project (audiosource)

include (CheckIncludeFile)
include (CheckIncludeFiles)
include (CheckLibraryExists)

#subdirs (
#  scripts
#  desktop
#  glade
#  icons
#  images
#  po
#  include
#  xsd
#  widgets
#  musicbrainz
#  com
#  xmlcpp
#  taglib
#)

check_include_files ("argz.h arpa/inet.h fcntl.h float.h inttypes.h langinfo.h libintl.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdio_ext.h stdlib.h string.h strings.h sys/param.h sys/socket.h sys/time.h unistd.h wchar.h" HAVE_MISC_HEADERS)

check_library_exists (socket connect "" HAVE_SOCKET_LIB)
check_library_exists (nsl getrpcbyname "" HAVE_NSL)

find_package (Gettext)

## Profilling
#AC_ARG_ENABLE([profile],
#    [AS_HELP_STRING([--enable-profile], [Enable Profiling support ***THIS IS A DEVELOPER OPTION*** Don't enable this if you don't need it (and if you don't know whether you do or not, then you dont!) (default=disabled)])],
#    [enable_profile="$enableval"],
#    [enable_profile="no"]
#)

#if test "$enable_profile" = "yes"; then
#    C_COMMON_FLAGS="$C_COMMON_FLAGS -pg -fno-inline"
#fi

## Debugging
#AC_ARG_ENABLE([debug],
#    [AS_HELP_STRING([--enable-debug], [Enable Debugging support (default=disabled)])],
#    [enable_debug="$enableval"],
#    [enable_debug="no"]
#)

#if test "$enable_debug" = "yes"; then
#    C_COMMON_FLAGS="$C_COMMON_FLAGS -ggdb3"
#    CFLAGS="$CFLAGS -ggdb3"
#    AC_DEFINE([WITH_DEBUG], [], [Building with debug information])
#fi
#AM_CONDITIONAL([WITH_DEBUG], [test "$enable_debug" = "yes"])

# CFLAGS/CXXFLAGS
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "gcc")
  # NOTE: We cannot use -pedantic with g++ because GST headers ()@#!@#
  # adds extra commas after the last item in enums
  set (C_COMMON_FLAGS "${C_COMMON_FLAGS} -fno-strict-aliasing -fmessage-length=0 -Wall -D_FORTIFY_SOURCE=2")
  set (CFLAGS "${CFLAGS} -std=c99 -pedantic")
else (${CMAKE_CXX_COMPILER_ID} STREQUAL "gcc")
  set (C_COMMON_FLAGS "${C_COMMON_FLAGS} -Wall -D_FORTIFY_SOURCE=2")
endif (${CMAKE_CXX_COMPILER_ID} STREQUAL "gcc")

find_package (X11)

find_package (Boost)
#AX_BOOST_BASE([1.33.1])
#AX_BOOST_FILESYSTEM
#AX_BOOST_REGEX
#AX_BOOST_IOSTREAMS
#AX_BOOST_PYTHON

set (USE_TR1 "YES")

if (USE_TR1)
  #BMP_CHECK_TR1
  if (HAVE_TR1)
    #AC_MSG_RESULT([available])
    #AC_DEFINE([HAVE_TR1], [], [Define when having C++ tr1 extensions])
  else (HAVE_TR1)
    #AC_MSG_RESULT([not available])
  endif (HAVE_TR1)
endif (USE_TR1)

find_package (PkgConfig)

#PYTHON_CFLAGS=`python-config --cflags`
#PYTHON_LIBS=`python-config --libs`
#AC_SUBST(PYTHON_CFLAGS)
#AC_SUBST(PYTHON_LIBS)

pkg_check_modules (PYGTK REQUIRED pygtk-2.0>=2.12.0)

#AC_MSG_CHECKING([for conflicting pygtk versions])
#pygtk_version=`$PKG_CONFIG --modversion pygtk-2.0`
#case $pygtk_version in
#     2.9.0|2.9.1)
#        AC_MSG_RESULT([found $pygtk_version])
#        AC_MSG_ERROR([invalid pygtk version found; please upgrade])
#        ;;
#     *)
#        AC_MSG_RESULT([none])
#        ;;
#esac

#AC_MSG_CHECKING(for pygtk defs)
#PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
#AC_SUBST(PYGTK_DEFSDIR)
#AC_MSG_RESULT($PYGTK_DEFSDIR)

pkg_check_modules (XML REQUIRED libxml-2.0>=2.6.1)

pkg_check_modules (SOUP REQUIRED libsoup-2.4>=2.2.10)

pkg_check_modules (SQLITE REQUIRED sqlite3>=3.4)

pkg_check_modules (GLIB   REQUIRED glib-2.0>=2.10.0 gobject-2.0 gthread-2.0 gmodule-2.0)
pkg_check_modules (GLIBMM REQUIRED glibmm-2.4>=2.8.3)
pkg_check_modules (SIGC   REQUIRED sigc++-2.0)

# AM_GLIB_GNU_GETTEXT

#pkg_check_modules (CLUTTER REQUIRED clutter-0.6 clutter-cairo-0.6 clutter-gtk-0.6)

pkg_check_modules (GIOMM      REQUIRED giomm-2.4)
pkg_check_modules (GTK        REQUIRED gtk+-2.0>=2.10.0)
pkg_check_modules (GTKMM      REQUIRED gtkmm-2.4>=2.6.0)
pkg_check_modules (LIBGLADEMM REQUIRED libglademm-2.4>=2.6.0)
pkg_check_modules (CAIROMM    REQUIRED cairomm-1.0>=0.6.0)

#pkg_check_modules (SEXYMM REQUIRED libsexymm>=0.1.9)
set (SEXYMM_INCLUDES "")
set (SEXYMM_LIBS "-lsexymm")

## Network Manager
#AC_ARG_WITH([nm],
#    [AS_HELP_STRING([--with-nm], [With GNOME NetworkManager])],
#    [use_nm="$withval"],
#    [use_nm="no"]
#)
#
set (ENABLE_NM "NO")

if (ENABLE_NM)
  pkg_check_modules (NM, NetworkManager>=0.6.0)

  if (NM_FOUND)
    #AC_DEFINE([HAVE_NM], [], [Network Manager support])
  endif (NM_FOUND)
endif (ENABLE_NM)

## xerces
#AC_ARG_WITH([xerces-prefix],
#    [AC_HELP_STRING([--with-xerces-prefix],[xerces prefix [default=/usr]])],
#    [xerces_prefix="$withval"],
#    [xerces_prefix="/usr"]
#)

check_library_exists (xerces_c init "" HAVE_XERCES)
if(HAVE_XERCES)
    set (XML_INCLUDES "-I$xml_prefix/xercesc")
    set (XML_LIBS "-L$xml_prefix/lib -lxerces-c")
endif(HAVE_XERCES)

pkg_check_modules (GST              REQUIRED gstreamer-0.10>=0.10.14 gstreamer-base-0.10>=0.10.14)
pkg_check_modules (GST_PLUGINS_BASE REQUIRED gstreamer-plugins-base-0.10>=0.10.9)

pkg_check_modules (DBUS REQUIRED dbus-1>=0.93 dbus-glib-1>=0.61)
#DEFAULT_DBUS_PREFIX=`pkg-config --variable=prefix dbus-1`

find_program (DBUSBINDINGTOOL "dbus-binding-tool")

#GLIB_GENMARSHAL=`$PKG_CONFIG glib-2.0 --variable=glib_genmarshal`
#AC_SUBST(GLIB_GENMARSHAL)

#AC_ARG_WITH([dbus-services-dir],
#    [AS_HELP_STRING([--with-dbus-services-dir@<:@=PATH@:>@], [D-BUS services file destination dir])],
#    [DBUS_SERVICES_DIR="$withval"],
#    [DBUS_SERVICES_DIR="$DEFAULT_DBUS_PREFIX/share/dbus-1/services"]
#)

#if test "$with_dbus_services_dir" = ""; then 
#    if test "$DEFAULT_DBUS_PREFIX" = ""; then
#        AC_MSG_ERROR([Unable to determine D-BUS installation prefix for automatic discovery of the serivices dir. Please use --with-dbus-services-dir!])
#    fi
#fi
#AC_SUBST([DBUS_SERVICES_DIR])
#AC_DEFINE_UNQUOTED([DBUS_SERVICES_DIR], ["$DBUS_SERVICES_DIR"], [Directory for D-BUS service files])

set (ENABLE_HAL, "NO")

if (ENABLE_HAL)
  pkg_check_modules (HAL hal>=0.5.7.1 hal-storage>=0.5.7.1)
  if (HAL_FOUND)
    set (ENABLE_HAL "YES")
  else (HAL_FOUND)
    set (ENABLE_HAL "NO")
  endif (HAL_FOUND)
endif (ENABLE_HAL)

if (ENABLE_HAL)
  #MPX_CHECK_HAL_NEWPSI
  set (HAVE_HAL_058, 0)

  pkg_check_modules (HAL REQUIRED hal>=0.5.7.1 hal-storage>=0.5.7.1)
  #AC_DEFINE([HAVE_HAL], [], [Define if building with HAL support])

  pkg_check_modules (HAL_058 hal>=0.5.8.1 hal-storage>=0.5.8.1)
  if(HAL_058_FOUND)
    #AC_DEFINE([HAVE_HAL_058], [], [Defined when building with HAL >= 0.5.8.1])
  endif(HAL_058_FOUND)

endif (ENABLE_HAL)

pkg_check_modules (TAGLIB REQUIRED taglib-gio>=1.4)

# libsidplay (v1.x)

#AC_ARG_ENABLE([sid],
#    [AC_HELP_STRING([--enable-sid],[build MPX with .sid/.psid taglib support])],
#    [have_sid="$enableval"],
#    [have_sid="no"]
#)

#AC_ARG_WITH([sid-prefix],
#    [AC_HELP_STRING([--with-sid-prefix],[sidplay prefix [default=/usr]])],
#    [sid_prefix="$withval"],
#    [sid_prefix="/usr"]
#)
set (ENABLE_SID "NO")
set (SID_PREFIX "/usr")

if (ENABLE_SID)
  check_library_exists (sidplay C64_clockSpeed "" HAVE_SID)

  if (HAVE_SID)
    #AC_DEFINE([HAVE_SID], [1], [have sid])
    set (SID_INCLUDES "-I ${SID_PREFIX}/sidplay")
    set (SID_LIBS "-I ${SID_PREFIX}/lib -lsidplay")
  else (HAVE_SID)
    #AC_MSG_ERROR([libsidplay 1.x not found!])
  endif (HAVE_SID)

endif (ENABLE_SID)

# ModPlug check
#AC_ARG_ENABLE([modplug],
#    [AC_HELP_STRING([--enable-modplug],[build MPX with modplug taglib support])],
#    [have_mod="$enableval"],
#    [have_mod="no"]
#)
set (ENABLE_MOD "YES")

if (ENABLE_MOD)
  pkg_check_modules (MOD, libmodplug>=0.7)
  #AC_DEFINE([HAVE_MOD], [1], [have mod])
endif (ENABLE_MOD)

# SMlib/ICE

#AC_ARG_ENABLE([sm],
#    [AS_HELP_STRING([--enable-sm], [Enable session management using SMlib (default=enabled)])],
#    [enable_sm="$enableval"],
#    [enable_sm="yes"]
#)
set (ENABLE_SM, "YES")

if (ENABLE_SM)
  #BMP_CHECK_SMLIB([],
  # [AC_MSG_ERROR([Cannot find SMlib])]
  #)

  #
  #AC_DEFINE([HAVE_SM], [1], [Define if building with SMlib support])
endif(ENABLE_SM)

# Startup Notification
#AC_ARG_ENABLE([sn],
#    [AS_HELP_STRING([--enable-sn], [Enable Startup Notification (default=enabled)])],
#    [enable_sn="$enableval"],
#    [enable_sn="yes"]
#)
set (ENABLE_SN "YES")

if (ENABLE_SN)
  pkg_check_modules (STARTUP_NOTIFY libstartup-notification-1.0>=0.8)
  #AC_DEFINE([HAVE_STARTUP_NOTIFICATION], [], [Define when building with libstartup-notification])
  set (ENABLE_SN_DESKTOP "FALSE")
else (ENABLE_SN)
  set (ENABLE_SN_DESKTOP "FALSE")
endif (ENABLE_SN)

# Zip
find_program (ZIP "zip")
#if test "x$ZIP" != "x"; then
#  AC_DEFINE([HAVE_ZIP], [], [Define if having ZIP])
#fi

pkg_check_modules(LIBOFA REQUIRED libofa>=0.9.3)

pkg_check_modules(GDL REQUIRED gdl-1.0)

pkg_check_modules(SIGX REQUIRED sigx-2.0)

# GTKGLEXT
pkg_check_modules(GTKGLEXT REQUIRED gtkglextmm-1.2)

# GAMIN
check_library_exists (fam FAMOpen "" HAVE_FAM)
check_library_exists (fam FAMNoExists "" HAVE_FAMNOEXISTS)
check_include_file (fam.h HAVE_FAM_H)

# Multimedia keys
pkg_check_modules(XPROTO xproto>=7.0.10)

#CFLAGS="$CFLAGS $C_COMMON_FLAGS"
#CXXFLAGS="$CXXFLAGS $C_COMMON_FLAGS"
