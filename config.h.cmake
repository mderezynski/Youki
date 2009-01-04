#ifndef CONFIG_H
#define CONFIG_H

/* Name of package */
#cmakedefine PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION

/* Building with debug or profiling information */
#cmakedefine WITH_DEBUG
#cmakedefine WITH_PROFILING

/* Platform defines */
#cmakedefine HAVE_LINUX
#cmakedefine HAVE_DRAGONFLYBSD
#cmakedefine HAVE_FREEBSD
#cmakedefine HAVE_SUN

/* Data directories */
#cmakedefine DATA_DIR
#cmakedefine LOCALE_DIR

/* Plugins settings */
#cmakedefine PLUGIN_DIR
#cmakedefine PLUGIN_VERSION

/* Directory for D-BUS service files */
#cmakedefine DBUS_SERVICES_DIR

/* default devices */
#cmakedefine DEFAULT_DEVICE_ALSA
#cmakedefine DEFAULT_DEVICE_ESD
#cmakedefine DEFAULT_DEVICE_OSS
#cmakedefine DEFAULT_DEVICE_SUN

/* default sink */
#cmakedefine DEFAULT_SINK

/* Defined when having BMP */
#cmakedefine HAVE_BMP

/* Define if ALSA is present */
#cmakedefine HAVE_ALSA

/* define if the Boost library is available */
#cmakedefine HAVE_BOOST

/* define if the Boost::Filesystem library is available */
#cmakedefine HAVE_BOOST_FILESYSTEM

/* define if the Boost::IOStreams library is available */
#cmakedefine  HAVE_BOOST_IOSTREAMS

/* define if the Boost::Python library is available */
#cmakedefine  HAVE_BOOST_PYTHON

/* define if the Boost::Regex library is available */
#cmakedefine HAVE_BOOST_REGEX

/* have cdio */
#cmakedefine HAVE_CDIO

/* have cdparaonia */
#cmakedefine HAVE_CDPARANOIA

/* Define if you have gamin instead of fam */
#cmakedefine HAVE_FAMNOEXISTS

/* Define if the GNU gettext() function is already present or preinstalled. */
#cmakedefine HAVE_GETTEXT

/* Defined when having GTK2 */
#cmakedefine HAVE_GTK

/* Define if building with HAL support */
#cmakedefine HAVE_HAL

/* Defined when building with HAL >= 0.5.8.1 */
#cmakedefine HAVE_HAL_058

/* define when having newstyle HAL PSI */
#cmakedefine HAVE_HAL_NEWPSI

/* define when having OFA */
#cmakedefine HAVE_OFA

/* Define to 1 if you have the `fam' library (-lfam). */
#cmakedefine HAVE_LIBFAM

/* Define to 1 if you have the <libintl.h> header file. */
#cmakedefine HAVE_LIBINTL_H

/* have sid */
#cmakedefine HAVE_SID

/* Define if building with SMlib support */
#cmakedefine HAVE_SM

/* Define to 1 if you have the `socket' function. */
#cmakedefine HAVE_SOCKET

/* Define when building with libstartup-notification */
#cmakedefine HAVE_STARTUP_NOTIFICATION

/* Define when having C++ tr1 extensions */
#cmakedefine HAVE_TR1

/* Define when having libxml2 */
#cmakedefine HAVE_XML

/* Define when having NetworkManager */
#cmakedefine HAVE_NM

/* Define if having ZIP */
#cmakedefine HAVE_ZIP

#endif
