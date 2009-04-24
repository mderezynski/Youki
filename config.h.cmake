#ifndef CONFIG_H
#define CONFIG_H

/* Define to the name of this package */
#define PACKAGE             "@PACKAGE@"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT   "@PACKAGE_BUGREPORT@"

/* Define to the full name of this package. */
#define PACKAGE_NAME        "@PACKAGE_NAME@"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING      "@PACKAGE_STRING@"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME     "@PACKAGE_TARNAME@"

/* Define to the version of this package. */
#define PACKAGE_VERSION     "@PACKAGE_VERSION@"

/* Dummy define to make compilation success. Code to be changed */
#define CONFIGURE_ARGS ""

/* Platform defines */
#cmakedefine HAVE_SUN
#cmakedefine HAVE_FREEBSD
#cmakedefine HAVE_NETBSD
#cmakedefine HAVE_DRAGONFLYBSD
#cmakedefine HAVE_LINUX

#define PREFIX              "@CMAKE_INSTALL_PREFIX@"

/* Data directories */
#define DATA_DIR            "@DATA_DIR@"
#define LOCALE_DIR          "@LOCALE_DIR@"

/* Plugins settings */
#define PLUGIN_DIR          "@PLUGIN_DIR@"
#define PLUGIN_VERSION      "@PLUGIN_VERSION@"

/* Directory for D-BUS service files */
#define DBUS_SERVICES_DIR   "@DBUS_SERVICES_DIR@"

/* default devices */
#define DEFAULT_DEVICE_ALSA "@DEFAULT_DEVICE_ALSA@"
#define DEFAULT_DEVICE_ESD  "@DEFAULT_DEVICE_ESD@"
#define DEFAULT_DEVICE_OSS  "@DEFAULT_DEVICE_OSS@"
#define DEFAULT_DEVICE_SUN  "@DEFAULT_DEVICE_SUN@"

/* default sink */
#define DEFAULT_SINK        "@DEFAULT_SINK@"

/* Defined when having BMP */
#cmakedefine HAVE_BMP

/* Define if ALSA is present */
#cmakedefine HAVE_ALSA

/* define if the Boost library is available */
#cmakedefine HAVE_BOOST

/* define if the Boost::Filesystem library is available */
#cmakedefine HAVE_BOOST_FILESYSTEM

/* define if the Boost::IOStreams library is available */
#cmakedefine HAVE_BOOST_IOSTREAMS

/* define if the Boost::Python library is available */
#cmakedefine HAVE_BOOST_PYTHON

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
#cmakedefine HAVE_FAM

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
