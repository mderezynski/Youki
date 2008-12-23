#libxml (C) 2003-2007 MPX developers, see AUTHORS

AC_PREREQ([2.60])
AC_INIT([audiosource], [0.01], [http://mpx.backtrace.info/report])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([1.8])
AM_GNU_GETTEXT_VERSION(0.17)
AM_GNU_GETTEXT([external])

AC_CONFIG_HEADERS([config.h])

# stash the configure arguments in config.h
AC_DEFINE_UNQUOTED(CONFIGURE_ARGS, "$ac_configure_args", [configure arguments])

# Check for compilers 
AC_LANG([C])
AC_LANG([C++])
AC_ISC_POSIX

# Various checks
AC_PROG_CXX
AC_PROG_CXX_C_O
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([argz.h arpa/inet.h fcntl.h float.h inttypes.h langinfo.h libintl.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdio_ext.h stdlib.h string.h strings.h sys/param.h sys/socket.h sys/time.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE
AC_CHECK_TYPES([ptrdiff_t])

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([__argz_count __argz_next __argz_stringify dup2 fdatasync ftime ftruncate getcwd gethostbyname gettimeofday localtime_r mempcpy memset mkdir munmap nl_langinfo rmdir select setlocale socket stpcpy strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strstr strtol strtoul access])

AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 8)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(long long, 8)

AC_SUBST(SIZEOF_INT, "$ac_cv_sizeof_int")
AC_SUBST(SIZEOF_LONG, "$ac_cv_sizeof_long")
AC_SUBST(SIZEOF_SHORT, "$ac_cv_sizeof_short")
AC_SUBST(SIZEOF_LONG_LONG, "$ac_cv_sizeof_long_long")

AC_SEARCH_LIBS([connect], socket)
AC_SEARCH_LIBS([getrpcbyname], nsl)

# i18n
#AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$PACKAGE", [Package])
#GETTEXT_PACKAGE="$PACKAGE"
#AC_SUBST(GETTEXT_PACKAGE)

#IT_PROG_INTLTOOL([0.35.0])
#AM_GLIB_GNU_GETTEXT

# Next 2 lines is a hack to prevent libtool checking for F77
m4_undefine([AC_PROG_F77])
m4_defun([AC_PROG_F77],[])

# Libtool
AC_DISABLE_STATIC
AC_ENABLE_SHARED
AC_PROG_LIBTOOL

####################################
########## BUILDING FLAGS ##########
####################################

dnl Avoid unnecessary linking.

AC_MSG_CHECKING([whether $LD accepts --as-needed])
case `$LD --as-needed -v 2>&1 </dev/null` in
*GNU* | *'with BFD'*)
  LDFLAGS="$LDFLAGS -Wl,--as-needed"
  AC_MSG_RESULT([yes])
  ;;
*)
  AC_MSG_RESULT([no])
  ;;
esac

dnl Optimize linking.

AC_MSG_CHECKING([whether $LD accepts -O1])
case `$LD -O1 -v 2>&1 </dev/null` in
*GNU* | *'with BFD'*)
  LDFLAGS="$LDFLAGS -Wl,-O1"
  AC_MSG_RESULT([yes])
  ;;
*)
  AC_MSG_RESULT([no])
  ;;
esac

AC_ARG_ENABLE([ld-workaround],
    [AS_HELP_STRING([--enable-ld-workaround], [Enable Linker workaround for GCC 4.0.X to 4.2.1 series , possible other. (default=disabled)])],
    [ld_workaround="$enableval"],
    [ld_workaround="no"]
)

if test "$ld_workaround" = "yes"; then
    LDFLAGS="$LDFLAGS -Wl,--noinhibit-exec"
fi

# Clear C[XX]FLAGS option
AC_ARG_ENABLE([default-cflags],
    [AS_HELP_STRING([--enable-default-cflags], [Don't reset C{XX}FLAGS for the build (default=enabled)])],
    [default_cflags="$enableval"],
    [default_cflags="yes"]
)

if test "$default_cflags" != "yes"; then 
    CFLAGS=""
    CXXFLAGS=""
fi

# Debian C[XX]FLAGS option
AC_ARG_ENABLE([debian-cflags],
    [AS_HELP_STRING([--enable-debian-cflags], [Enable debian build flags])],
    [debian_cflags="$enableval"],
    [debian_cflags="no"]
)

if test "$debian_cflags" = "yes"; then 
    CFLAGS="-Wall -g -std=c99 -pedantic -fno-strict-aliasing -fmessage-length=0 -Wall -D_FORTIFY_SOURCE=2 -fPIC"
    CXXFLAGS="-g -fno-strict-aliasing -fmessage-length=0 -Wall -D_FORTIFY_SOURCE=2 -fPIC"
fi

# Profilling
AC_ARG_ENABLE([profile],
    [AS_HELP_STRING([--enable-profile], [Enable Profiling support ***THIS IS A DEVELOPER OPTION*** Don't enable this if you don't need it (and if you don't know whether you do or not, then you dont!) (default=disabled)])],
    [enable_profile="$enableval"],
    [enable_profile="no"]
)

if test "$enable_profile" = "yes"; then
    C_COMMON_FLAGS="$C_COMMON_FLAGS -pg -fno-inline"
fi


# Debugging
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug], [Enable Debugging support (default=disabled)])],
    [enable_debug="$enableval"],
    [enable_debug="no"]
)

if test "$enable_debug" = "yes"; then
    C_COMMON_FLAGS="$C_COMMON_FLAGS -ggdb3"
    CFLAGS="$CFLAGS -ggdb3"
    AC_DEFINE([WITH_DEBUG], [], [Building with debug information])
fi
AM_CONDITIONAL([WITH_DEBUG], [test "$enable_debug" = "yes"])

# CFLAGS/CXXFLAGS
if test "$GCC" = "yes"; then
    dnl NOTE: We cannot use -pedantic with g++ because GST headers ()@#!@#
    dnl adds extra commas after the last item in enums

    C_COMMON_FLAGS="$C_COMMON_FLAGS -fno-strict-aliasing -fmessage-length=0 -Wall -D_FORTIFY_SOURCE=2"
    CFLAGS="$CFLAGS -std=c99 -pedantic"
else
    C_COMMON_FLAGS="$C_COMMON_FLAGS -Wall -D_FORTIFY_SOURCE=2"
fi

###########################################
########## REQUIRED DEPENDENCIES ##########
###########################################

# X11
AC_PATH_XTRA

if test "$no_x" = "yes" ; then
    AC_MSG_ERROR([X11 headers/libraries were not found])
else
    XLIB_LIBS='$(X_LIBS) $(X_PRE_LIBS) -lX11 $(X_EXTRA_LIBS)'
    AC_SUBST([XLIB_LIBS])
fi

# Boost
AX_BOOST_BASE([1.33.1])
AX_BOOST_FILESYSTEM
AX_BOOST_REGEX
AX_BOOST_IOSTREAMS
AX_BOOST_PYTHON

# tr1
AC_ARG_WITH([tr1],
    [AS_HELP_STRING([--with-tr1], [Use C++ TR1 extensions])],
    [use_tr1="$withval"],
    [use_tr1="no"]
)

have_tr1=no
if test "$use_tr1" = "yes"; then
  BMP_CHECK_TR1
  if test "$have_tr1" = "yes"; then
    AC_MSG_RESULT([available])
    AC_DEFINE([HAVE_TR1], [], [Define when having C++ tr1 extensions])
  else
    AC_MSG_RESULT([not available])
  fi
fi
AM_CONDITIONAL([HAVE_TR1], [test "$have_tr1" = "yes"])

# pkg-config
PKG_PROG_PKG_CONFIG
AC_ARG_WITH([pkgconfig-dir],
    [AS_HELP_STRING([--with-pkgconfig-dir@<:@=PATH@:>@], [pkgconfig file destination dir])],
    [pkgconfigdir="$withval"],
    [pkgconfigdir='$(libdir)/pkgconfig']
)
AC_SUBST([pkgconfigdir])

PYTHON_CFLAGS=`python-config --cflags`
PYTHON_LIBS=`python-config --libs`
AC_SUBST(PYTHON_CFLAGS)
AC_SUBST(PYTHON_LIBS)

# check for pygtk
PKG_CHECK_MODULES([PYGTK], [pygtk-2.0 >= 2.12.0])
AC_SUBST(PYGTK_CFLAGS)
AC_PATH_PROG(PYGTK_CODEGEN, pygtk-codegen-2.0, no)
if test "x$PYGTK_CODEGEN" = xno; then
  AC_MSG_ERROR(could not find pygtk-codegen-2.0 script)
fi

AC_MSG_CHECKING([for conflicting pygtk versions])
pygtk_version=`$PKG_CONFIG --modversion pygtk-2.0`
case $pygtk_version in
     2.9.0|2.9.1)
        AC_MSG_RESULT([found $pygtk_version])
        AC_MSG_ERROR([invalid pygtk version found; please upgrade])
        ;;
     *)
        AC_MSG_RESULT([none])
        ;;
esac

AC_MSG_CHECKING(for pygtk defs)
PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
AC_SUBST(PYGTK_DEFSDIR)
AC_MSG_RESULT($PYGTK_DEFSDIR)

# Libxml
PKG_CHECK_MODULES([XML], [libxml-2.0 >= 2.6.1])

# libsoup
PKG_CHECK_MODULES([SOUP], [libsoup-2.4 >= 2.2.101])

# sqlite
PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.4])

# glib/glibmm/sigc++
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.10.0 gobject-2.0 gthread-2.0 gmodule-2.0])
PKG_CHECK_MODULES([GLIBMM], [glibmm-2.4 >= 2.8.3])
PKG_CHECK_MODULES([SIGC], [sigc++-2.0 >= 2.0.0])

# clutter
# PKG_CHECK_MODULES([CLUTTER], [clutter-0.6 >= 0.6.0 clutter-cairo-0.6 >= 0.6.0 clutter-gtk-0.6 >= 0.6.0])

# GIOMM
PKG_CHECK_MODULES([GIOMM], [giomm-2.4])

# Gtk+
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.10.0])

# GTKmm
PKG_CHECK_MODULES([GTKMM], [gtkmm-2.4 >= 2.10.0])

# libglademm 
PKG_CHECK_MODULES([GLADEMM], [libglademm-2.4 >= 2.6.0])

# cairomm
PKG_CHECK_MODULES([CAIROMM], [cairomm-1.0 >= 0.6.0])

#sexymm
#PKG_CHECK_MODULES([SEXYMM], [libsexymm >= 0.1.9])
SEXYMM_INCLUDES=
SEXYMM_LIBS=-lsexymm

# Network Manager
AC_ARG_WITH([nm],
    [AS_HELP_STRING([--with-nm], [With GNOME NetworkManager])],
    [use_nm="$withval"],
    [use_nm="no"]
)

have_nm=no
if test "x$use_nm" = "xyes"; then 
    PKG_CHECK_MODULES([NM], [NetworkManager >= 0.6.0], [have_nm="yes"], [have_nm="no"])
fi
if test "x$have_nm" = "xyes"; then 
    AC_DEFINE([HAVE_NM], [], [Network Manager support])
fi
AM_CONDITIONAL([HAVE_NM], [test "x$have_nm" = "xyes"])

# xerces
AC_ARG_WITH([xerces-prefix],
    [AC_HELP_STRING([--with-xerces-prefix],[xerces prefix [default=/usr]])],
    [xerces_prefix="$withval"],
    [xerces_prefix="/usr"]
)

AC_CHECK_LIB([xerces-c], [_init], [have_xerces=yes], [have_xerces=no])
if test "$have_xerces" = "yes"; then
    XML_INCLUDES="-I$xml_prefix/xercesc"
    XML_LIBS="-L$xml_prefix/lib -lxerces-c"
else
    AC_MSG_ERROR([Xerces-C not found])
fi
AC_SUBST([XML_INCLUDES])
AC_SUBST([XML_LIBS])

# GStreamer
PKG_CHECK_MODULES([GST], [gstreamer-0.10 >= 0.10.14 gstreamer-base-0.10 >= 0.10.14])
PKG_CHECK_MODULES([GST_PLUGINS_BASE], [gstreamer-plugins-base-0.10 >= 0.10.9])

# D-BUS
PKG_CHECK_MODULES([DBUS], [dbus-1 >= 0.93 dbus-glib-1 >= 0.61])
DEFAULT_DBUS_PREFIX=`pkg-config --variable=prefix dbus-1` 

AC_PATH_PROG(DBUSBINDINGTOOL, dbus-binding-tool)
AC_SUBST(DBUSBINDINGTOOL)

GLIB_GENMARSHAL=`$PKG_CONFIG glib-2.0 --variable=glib_genmarshal`
AC_SUBST(GLIB_GENMARSHAL)

AC_ARG_WITH([dbus-services-dir],
    [AS_HELP_STRING([--with-dbus-services-dir@<:@=PATH@:>@], [D-BUS services file destination dir])],
    [DBUS_SERVICES_DIR="$withval"],
    [DBUS_SERVICES_DIR="$DEFAULT_DBUS_PREFIX/share/dbus-1/services"]
)

if test "$with_dbus_services_dir" = ""; then 
    if test "$DEFAULT_DBUS_PREFIX" = ""; then
        AC_MSG_ERROR([Unable to determine D-BUS installation prefix for automatic discovery of the serivices dir. Please use --with-dbus-services-dir!])
    fi
fi
AC_SUBST([DBUS_SERVICES_DIR])
AC_DEFINE_UNQUOTED([DBUS_SERVICES_DIR], ["$DBUS_SERVICES_DIR"], [Directory for D-BUS service files])

# HAL
# AC_ARG_ENABLE([hal],
#    [AS_HELP_STRING([--enable-hal], [Enable usage of HAL for managing storage (default=enabled)])],
#    [enable_hal="$enableval"],
#    [enable_hal="yes"]
#)
enable_hal="yes"

if test "$enable_hal" = "yes"; then
PKG_CHECK_EXISTS([hal >= 0.5.7.1 hal-storage >= 0.5.7.1],
                 [enable_hal="yes"], [enable_hal="no"])
fi

if test "$enable_hal" = "yes"; then
  MPX_CHECK_HAL_NEWPSI
  have_hal_058=0
  PKG_CHECK_MODULES([HAL], [hal >= 0.5.7.1 hal-storage >= 0.5.7.1])
  AC_DEFINE([HAVE_HAL], [], [Define if building with HAL support])

  PKG_CHECK_EXISTS([hal >= 0.5.8.1 hal-storage >= 0.5.8.1],
                   [have_hal_058=1], [have_hal_058=0])
  if test "$have_hal_058" = "1"; then
    AC_DEFINE([HAVE_HAL_058], [], [Defined when building with HAL >= 0.5.8.1])
  fi

fi
AM_CONDITIONAL([HAVE_HAL], [test "$enable_hal" = "yes"])
AM_CONDITIONAL([HAVE_HAL_058], [test "$have_hal_058" = "1"])

# Taglib
PKG_CHECK_MODULES([TAGLIB], [taglib-gio >= 1.4])

# libsidplay (v1.x)
AC_ARG_ENABLE([sid],
    [AC_HELP_STRING([--enable-sid],[build MPX with .sid/.psid taglib support])],
    [have_sid="$enableval"],
    [have_sid="no"]
)

AC_ARG_WITH([sid-prefix],
    [AC_HELP_STRING([--with-sid-prefix],[sidplay prefix [default=/usr]])],
    [sid_prefix="$withval"],
    [sid_prefix="/usr"]
)

echo $have_sid

if test "$have_sid" != "no"; then

    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS

    AC_CHECK_LIB([sidplay], [C64_clockSpeed], [have_sid=yes], [have_sid=no])

    if test "$have_sid" = "yes"; then
        AC_DEFINE([HAVE_SID], [1], [have sid])
        SID_INCLUDES="-I$sid_prefix/sidplay"
        SID_LIBS="-L$sid_prefix/lib -lsidplay"
    else
        AC_MSG_ERROR([libsidplay 1.x not found!])
    fi

    AC_LANG_RESTORE
fi
AC_SUBST([SID_INCLUDES])
AC_SUBST([SID_LIBS])
AM_CONDITIONAL([HAVE_SID], [test "$have_sid" != "no"])

# ModPlug check
AC_ARG_ENABLE([modplug],
    [AC_HELP_STRING([--enable-modplug],[build MPX with modplug taglib support])],
    [have_mod="$enableval"],
    [have_mod="no"]
)

if test "$have_mod" != "no"; then
  PKG_CHECK_MODULES([MOD], [libmodplug >= 0.7])
  AC_DEFINE([HAVE_MOD], [1], [have mod])
fi
AM_CONDITIONAL([HAVE_MOD], [test "$have_mod" != "no"])

# SMlib/ICE
AC_ARG_ENABLE([sm],
    [AS_HELP_STRING([--enable-sm], [Enable session management using SMlib (default=enabled)])],
    [enable_sm="$enableval"],
    [enable_sm="yes"]
)

if test "$enable_sm" = "yes" ; then
    BMP_CHECK_SMLIB([],
        [AC_MSG_ERROR([Cannot find SMlib])]
    )

    AC_DEFINE([HAVE_SM], [1], [Define if building with SMlib support])
fi
AM_CONDITIONAL([HAVE_SM], [test "$enable_sm" = "yes"])

# Startup Notification
AC_ARG_ENABLE([sn],
    [AS_HELP_STRING([--enable-sn], [Enable Startup Notification (default=enabled)])],
    [enable_sn="$enableval"],
    [enable_sn="yes"]
)

if test "$enable_sn" = "yes"; then
    PKG_CHECK_MODULES([STARTUP_NOTIFY], [libstartup-notification-1.0 >= 0.8])
    AC_DEFINE([HAVE_STARTUP_NOTIFICATION], [], [Define when building with libstartup-notification])
    ENABLE_SN_DESKTOP="true"
else
    ENABLE_SN_DESKTOP="false"
fi
AC_SUBST([ENABLE_SN_DESKTOP])
AM_CONDITIONAL([HAVE_STARTUP_NOTIFICATION], [test "$enable_sn" = "yes"])

# Zip
AC_CHECK_PROG(ZIP, zip, zip)
if test "x$ZIP" != "x"; then
  AC_DEFINE([HAVE_ZIP], [], [Define if having ZIP])
fi
AM_CONDITIONAL([HAVE_ZIP], [test "x$ZIP" != "x"])

# libofa
PKG_CHECK_MODULES([LIBOFA], [libofa >= 0.9.3])

# GDL
PKG_CHECK_MODULES([GDL], [gdl-1.0])

# SIGX
PKG_CHECK_MODULES([SIGX], [sigx-2.0])

# GTKGLEXT
PKG_CHECK_MODULES([GTKGLEXT], [gtkglextmm-1.2])

# GAMIN
AC_CHECK_LIB(fam,FAMOpen,,[echo "To build this software, you must install the FAM development package"; exit 1])
AC_CHECK_LIB(fam,FAMNoExists,[AC_DEFINE(HAVE_FAMNOEXISTS, 1, [Define if you have gamin instead of fam])],)
AC_CHECK_HEADER(fam.h,,[echo "To build this software, you must install the FAM development package"; exit 1])

#########################################
######### PLATFORM SPECIFITIES ##########
#########################################

# Architecture
arch=unknown
arch_ia32=false
arch_powerpc=false
arch_64bit=false
big_endian=false

case $build_cpu in
    i386 | i486 | i586 | i686)
        arch=ia32
        arch_ia32=true
        AC_DEFINE([ARCH_IA32], [], [Defined if using x86 ASM])
    ;;
    powerpc)
        arch=powerpc
        arch_powerpc=true
        big_endian=true
        AC_DEFINE([ARCH_POWERPC], [], [Defined if using PPC ASM])
        AC_DEFINE([ARCH_IS_BIG_ENDIAN], [], [Defined if arch is big endian])
    ;;
    x86_64 | ia64)
        C_COMMON_FLAGS="$C_COMMON_FLAGS -fPIC"
        AC_DEFINE([ARCH_64BIT], [], [Define if having a 64bit based architecture])
        arch_64bit=true
    ;;
    *)
    ;;
esac
AM_CONDITIONAL([ARCH_POWERPC], [test "$arch" = "powerpc"])
AM_CONDITIONAL([ARCH_IA32], [test "$arch" = "ia32"])
AM_CONDITIONAL([ARCH_64BIT], [test "$arch_64bit" = "true"])
AM_CONDITIONAL([ARCH_IS_BIG_ENDIAN], [test "$big_endian" = "true"])

#########################
### Operating System ####
#########################

system=unknown
have_alsa=false
default_sink="autoaudiosink"
default_device="/dev/dsp"

case $host in
    *linux*)
      system=linux
      default_sink="alsasink"
      default_device="default"
      PKG_CHECK_MODULES([ALSA], [alsa >= 1.0.9])
      AC_DEFINE([HAVE_ALSA], [1], [Define if ALSA is present])
      AC_DEFINE([HAVE_LINUX], [1], [Define if building on FreeBSD])
      have_alsa=true
      ;;
    *freebsd*)
      system=freebsd
      default_sink="osssink"
      default_device="/dev/dsp"
      AC_DEFINE([HAVE_FREEBSD], [], [Define if building on FreeBSD])
      ;;
    *netbsd*)
      system=netbsd
      default_sink="osssink"
      default_device="/dev/audio"
      AC_DEFINE([HAVE_NETBSD], [], [Define if building on NetBSD])
      ;;
    *dragonfly*)
      system=dragonflybsd
      default_sink="osssink"
      default_device="/dev/dsp"
      AC_DEFINE([HAVE_DRAGONFLYBSD], [], [Define if building on DragonFlyBSD])
      ;;
    *-*-solaris* | *-*-sunos*)
      system=solaris
      default_sink="sunaudiosink"
      default_device="/dev/audio"
      AC_DEFINE([HAVE_SUN], [], [Define if building on Solaris/SunOS])
      ;;
    *)
      ;;
esac

have_cdparanoia=no
have_cdio=no

if test "$system" = "solaris"; then
  PKG_CHECK_MODULES([CDIO], [libcdio_cdda >= 0.70])
  have_cdio=yes
  AC_DEFINE([HAVE_CDIO], [1], [have cdio])
else
  AC_ARG_WITH([cdparanoia-prefix],
      [AC_HELP_STRING([--with-cdparanoia-prefix],[cdparanoia prefix [default=/usr]])],
      [cdparanoia_prefix="$withval"],
      [cdparanoia_prefix="/usr"]
  )

  cdparanoia_cdda_h=no
  CFLAGS_SAVE="$CFLAGS"
  CFLAGS="$CFLAGS -I$cdparanoia_prefix/include"

  AC_CHECK_HEADER([cdda_interface.h], [], [AC_CHECK_HEADER([cdda/cdda_interface.h], [cdparanoia_cdda_h=yes], [AC_MSG_ERROR([cdparanoia headers not found])])])
  AC_CHECK_LIB([cdda_interface], [cdda_open], [have_cdparanoia=yes], [AC_MSG_ERROR([cdparanoia libraries not found])])

  if test "$have_cdparanoia" = "yes"; then
          AC_DEFINE([HAVE_CDPARANOIA], [1], [have cdparaonia])
          CDPARANOIA_INCLUDES=""
          if test "$cdparanoia_cdda_h" = "yes"; then
            CDPARANOIA_INCLUDES="-I$cdparanoia_prefix/include/cdda"
          else
            CDPARANOIA_INCLUDES="-I$cdparanoia_prefix/include"
          fi
          CDPARANOIA_LIBS="-L$cdparanoia_prefix/lib -lcdda_interface"
  else
          AC_MSG_ERROR([cdparanoia not found!])
  fi
  CFLAGS="$CFLAGS_SAVE"
  AC_SUBST([CDPARANOIA_INCLUDES])
  AC_SUBST([CDPARANOIA_LIBS])
fi

AM_CONDITIONAL([HAVE_NM], [test x$HAVE_NM = xyes])
AM_CONDITIONAL([HAVE_CDPARANOIA], [test "$have_cdparanoia" != "no"])
AM_CONDITIONAL([HAVE_CDIO], [test "$have_cdio" != "no"])

AM_CONDITIONAL([HAVE_SUN], [test "$system" = "solaris"])
AM_CONDITIONAL([HAVE_FREEBSD], [test "$system" = "freebsd"])
AM_CONDITIONAL([HAVE_NETBSD], [test "$system" = "netbsd"])
AM_CONDITIONAL([HAVE_DRAGONFLYBSD], [test "$system" = "dragonflybsd"])
AM_CONDITIONAL([HAVE_LINUX], [test "$system" = "linux"])

AC_DEFINE_UNQUOTED(DEFAULT_SINK, "$default_sink", [default sink])
AC_DEFINE_UNQUOTED(DEFAULT_DEVICE_OSS, "/dev/dsp0", [default oss device])
AC_DEFINE_UNQUOTED(DEFAULT_DEVICE_ALSA, "default", [default alsa device])
AC_DEFINE_UNQUOTED(DEFAULT_DEVICE_SUN, "/dev/audio", [default sun device])
AC_DEFINE_UNQUOTED(DEFAULT_DEVICE_ESD, "localhost", [default esd device])

DEFAULT_SINK=$default_sink
DEFAULT_DEVICE=$default_device

AC_SUBST(DEFAULT_SINK)
AC_SUBST(DEFAULT_DEVICE)

# Multimedia keys
PKG_CHECK_MODULES([XPROTO], [xproto >= 7.0.10])

# MPX 
AC_SUBST([abs_top_srcdir])
AC_SUBST([abs_top_builddir])

PREFIX=${prefix}
PLUGIN_DIR=${libdir}/${PACKAGE}/plugins
LOCALE_DIR=${localedir}
DATA_DIR=${pkgdatadir}
AC_SUBST([PREFIX])
AC_SUBST([PLUGIN_DIR])
AC_SUBST([LOCALE_DIR])
AC_SUBST([DATA_DIR])

AX_DEFINE_UNQUOTED([PREFIX], [${prefix}], [The installation prefix])
AX_DEFINE_UNQUOTED([PLUGIN_DIR], [${libdir}/${PACKAGE}/plugins], [Plugins path])
AX_DEFINE_UNQUOTED([LOCALE_DIR], [${localedir}], [The locale directory])
AX_DEFINE_UNQUOTED([DATA_DIR], [${prefix}/share/${PACKAGE}], [Data directory])

MPX_DBUS_SERVICE_="\"org.mpris.mpx\""

MPX_DBUS_INTERFACE__MPRIS_="\"org.freedesktop.MediaPlayer\""
MPX_DBUS_PATH__MPRIS_ROOT_="\"/\""
MPX_DBUS_PATH__MPRIS_PLAYER_="\"/Player\""
MPX_DBUS_PATH__MPRIS_TRACKLIST_="\"/TrackList\""

MPX_DBUS_INTERFACE__MPX_="\"info.backtrace.mpx\""
MPX_DBUS_PATH__MPX_="\"/MPX\""

AC_SUBST(MPX_DBUS_PATH__MPRIS_ROOT_)
AC_SUBST(MPX_DBUS_PATH__MPRIS_PLAYER_)
AC_SUBST(MPX_DBUS_PATH__MPRIS_TRACKLIST_)
AC_SUBST(MPX_DBUS_PATH__MPX_)
AC_SUBST(MPX_DBUS_INTERFACE__MPRIS_)
AC_SUBST(MPX_DBUS_INTERFACE__MPX_)
AC_SUBST(MPX_DBUS_SERVICE_)

AC_DEFINE([PLUGIN_VERSION], [10], [Plugin Version])

# MCS stubs
AC_DEFINE([HAVE_BMP], [1], [Defined when having BMP])
AC_DEFINE([HAVE_XML], [1], [Defined when having libxml2])
AC_DEFINE([HAVE_GTK], [1], [Defined when having GTK2])
AM_CONDITIONAL([HAVE_BMP], [test "$PACKAGE" = "mpx"])
AM_CONDITIONAL([HAVE_XML], [test "1" = "1"]) 
AM_CONDITIONAL([HAVE_GTK], [test "1" = "1"])

CFLAGS="$CFLAGS $C_COMMON_FLAGS"
CXXFLAGS="$CXXFLAGS $C_COMMON_FLAGS"

AC_OUTPUT([

    icons/Makefile
    icons/hicolor/Makefile
    icons/hicolor/64x64/Makefile
    icons/hicolor/64x64/places/Makefile
    icons/hicolor/24x24/Makefile
    icons/hicolor/24x24/stock/Makefile
    icons/hicolor/16x16/Makefile
    icons/hicolor/16x16/stock/Makefile

	desktop/Makefile
	desktop/mpx.desktop

    glade/Makefile
    images/Makefile
    jnetlib/Makefile
    libhal++/Makefile
    mcs/Makefile

	boost/Makefile
	boost/python/Makefile
	boost/python/suite/Makefile
	boost/python/suite/indexing/Makefile
	boost/python/suite/indexing/detail/Makefile

    python/Makefile
	python/plugins/Makefile
	python/plugins/trackinfo-webkit/Makefile
	python/plugins/trackinfo-webkit/__init__.py
	python/plugins/tracktags/Makefile
	python/plugins/trackcover/Makefile
	python/plugins/osd/Makefile
	python/plugins/osd/__init__.py
	python/plugins/mpxdj/Makefile
	python/plugins/mpxdj/__init__.py
    python/plugins/albumactions/Makefile
    python/plugins/albumactions/__init__.py
    python/plugins/mtpplayer/Makefile
    python/plugins/mtpplayer/__init__.py
	python/plugins/audioscrobbler/Makefile
	python/plugins/audioscrobbler/__init__.py
	python/plugins/puid-scan/Makefile
	python/plugins/puid-scan/__init__.py
	python/plugins/lastfmstuff/Makefile
	python/plugins/lastfmstuff/__init__.py

    python/pyapi/Makefile
    python/pyapi/lastfm/Makefile
    python/pyapi/lastfm/model/Makefile


	remote/Makefile
    scripts/Makefile
    widgets/Makefile
    musicbrainz/Makefile
    com/Makefile
    xmlcpp/Makefile
    api/Makefile

    include/Makefile
    include/mpx/Makefile
    include/mpx/algorithm/Makefile
    include/mpx/aux/Makefile
    include/mpx/parser/Makefile
    include/mpx/widgets/Makefile
    include/mpx/xml/Makefile
    include/mpx/com/Makefile

    sources/Makefile
    sources/musiclib/Makefile
    sources/radio/Makefile
    sources/lastfm/Makefile

    taglib/Makefile
    taglib/id3v2/Makefile
    taglib/xiph/Makefile
    taglib/common/Makefile
    taglib/mp4/Makefile
    taglib/asf/Makefile
    taglib/sid/Makefile
    taglib/mod/Makefile
    taglib/ogg/Makefile
    taglib/flac/Makefile
    taglib/mp3/Makefile
    taglib/mpc/Makefile

    xsd/Makefile
    xsd/cxx/Makefile
    xsd/cxx/xml/Makefile
    xsd/cxx/xml/bits/Makefile
    xsd/cxx/xml/sax/Makefile
    xsd/cxx/xml/sax/bits/Makefile
    xsd/cxx/xml/dom/Makefile
    xsd/cxx/xml/dom/bits/Makefile
    xsd/cxx/tree/Makefile
    xsd/cxx/tree/bits/Makefile
    xsd/cxx/parser/Makefile
    xsd/cxx/parser/xerces/Makefile
    xsd/cxx/parser/validating/Makefile
    xsd/cxx/parser/non-validating/Makefile
    xsd/cxx/parser/expat/Makefile
    
    mpx/Makefile

    po/Makefile.in

    mpx.pc
    mpx.1

    Makefile
])


echo
echo "MPX build configuration"
echo "========================"
echo
echo "    C compiler flags.............: $CFLAGS"
echo "    C++ compiler flags...........: $CXXFLAGS"
echo "    Linker flags.................: $LDFLAGS"
if test "$ld_workaround" = "no"; then
echo "      ** Linker workarounds are disabled. Do *NOT* report *ANY* linking errors"
echo "      ** when building with gcc 4.0.X - 4.2.1 and binutils >=2.17.X."
echo
fi
echo "    Documentation generation.....: $generate_documentation"
echo
echo "    Audio: Default Sink..........: $default_sink (device $default_device)"
echo
echo "    X Session Management.........: $enable_sm"
echo "    Startup-notification Support.: $enable_sn"
echo "    HAL Storage Management.......: $enable_hal" 
echo -n "    Optional taglib plugins......:" 
if test "$have_sid" = "yes"; then
echo -n " SID/PSID "
taglib_warning=yes
fi
if test "$have_mod" = "yes"; then
echo -n " MOD/S3M/XM/IT "
taglib_warning=yes
fi
if test "$taglib_warning" = "yes"; then
echo
echo "      ** You will need to install the corresponding GStreamer plugins to be" 
echo "      ** able to play these formats; the support indicated is for metadata"
echo "      ** reading ONLY (yes that's how it works)!" 
fi
echo
