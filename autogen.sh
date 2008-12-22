#! /bin/sh

if [ -z "$USE_SHELL" ]; then
        USE_SHELL=/bin/sh; export USE_SHELL
fi

(eval 'echo $((1+1))') >/dev/null 2>&1 || {
    if [ -x /bin/ksh ]; then
        USE_SHELL=/bin/ksh
    elif [ -x /usr/xpg4/bin/sh ]; then
        USE_SHELL=/usr/xpg4/bin/sh
    else
        echo "please install /bin/ksh or /usr/xpg4/bin/sh"
        exit
    fi
    export USE_SHELL
    echo "restarting under $USE_SHELL..."
    $USE_SHELL $0 "$@"
    exit
}
   
TOP_DIR=$(cd $(dirname $0); echo $PWD)
LAST_DIR=$PWD

if test ! -f $TOP_DIR/configure.ac ; then
    echo "You must execute this script from the top level directory."
    exit 1
fi

# Functions
dump_help_screen ()
{
    echo "Usage: autogen.sh [options]"
    echo 
    echo "options:"
    echo "  -h,--help             Show this help screen"
    echo "  -n,--no-log           Don't create ChangeLog from SVN commit logs" 
    echo "  -l,--last-version     Specify the revision down to which create the log from"
    echo
    exit 0
}

GLOBOPTIONS=""

parse_options ()
{
    case $1 in
        -h|--help)
            dump_help_screen
            ;;
        -n|--no-log)
            no_log=1 
            shift 1
            ;;
        -l|--last-version)
            lastversion=$2 
            shift 2
            ;;
        *)
            echo Assuming configure arguments!
            ;;
    esac

    GLOBOPTIONS=$@
}

run_or_die_shell ()
{
    run_or_die $USE_SHELL "$@"
}

run_or_die ()
{
    COMMAND=$1

    # check for empty commands
    if test -z "$COMMAND" ; then
        echo "*warning* no command specified"
        return 1
    fi

    shift;

    OPTIONS="$@"

    # print a message
    printf "*info* running $COMMAND"
    if test -n "$OPTIONS" ; then
        echo " ($OPTIONS)"
    else
        echo
    fi

    # run or die
    $COMMAND $OPTIONS ; RESULT=$?
    if test $RESULT -ne 0 ; then
        echo "*error* $COMMAND failed. (exit code = $RESULT)"
        exit 1
    fi

    return 0
}

# Main run
parse_options "$@"
cd $TOP_DIR

AUTOCONF=${AUTOCONF:-autoconf}
AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOPOINT=${AUTOPOINT:-autopoint}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
#INTLTOOLIZE=${INTLTOOLIZE:-intltoolize}
#GLIB_GETTEXTIZE=${GLIB_GETTEXTIZE:-glib-gettextize}

# Check for proper automake version
automake_maj_req=1
automake_min_req=9

printf "Checking Automake version..."

automake_version=`$AUTOMAKE --version | head -n1 | cut -f 4 -d \ `
automake_major=$(echo $automake_version | cut -f 1 -d . | sed -e 's:^\([0-9]\+\).*$:\1:')
automake_minor=$(echo $automake_version | cut -f 2 -d . | sed -e 's:^\([0-9]\+\).*$:\1:')
automake_micro=$(echo $automake_version | cut -f 3 -d . | sed -e 's:^\([0-9]\+\).*$:\1:')

printf "$automake_major.$automake_minor.$automake_micro.:  "

if [ $automake_major -ge $automake_maj_req ]; then
    if [ $automake_minor -lt $automake_min_req ]; then 
      echo "error: bmpx requires automake $automake_maj_req.$automake_min_req"
      exit 1
    else
      echo "ok"
    fi
fi

# Check for proper autoconf version
autoconf_maj_req=2
autoconf_min_req=60

printf "Checking Autoconf version... "

autoconf_version=`$AUTOCONF --version | head -n1 | cut -f 4 -d \ `
autoconf_major=$(echo $autoconf_version | cut -f 1 -d . | sed -e 's:^\([0-9]\+\).*$:\1:')
autoconf_minor=$(echo $autoconf_version | cut -f 2 -d . | sed -e 's:^\([0-9]\+\).*$:\1:')

printf "$autoconf_major.$autoconf_minor..:  "

if [ $autoconf_major -ge $autoconf_maj_req ]; then
    if [ $autoconf_minor -lt $autoconf_min_req ]; then 
      echo "error: bmpx requires autoconf $autoconf_maj_req.$autoconf_min_req"
      exit 1
    else
      echo "ok"
    fi
fi

# Changelog from SVN (svn2cl)
if [ -d .svn ]; then
    if [ "x$no_log" != "x1" ]; then
      # only recreate if we in svn repo
      echo "*info* creating ChangeLog from SVN history"
      if [ "x$lastversion" != "x" ]; then
        REVISION=$(svnversion)
        $USE_SHELL ./scripts/svn2cl.sh -i --authors=./authors.xml --break-before-msg --separate-daylogs --reparagraph -r $REVISION:$lastversion
      else
        $USE_SHELL ./scripts/svn2cl.sh -i --authors=./authors.xml --break-before-msg --separate-daylogs --reparagraph
      fi
    fi
fi

if [ ! -e Changelog ]; then
  touch ChangeLog
fi

run_or_die $ACLOCAL -I m4
run_or_die $AUTOPOINT --force
#run_or_die $GLIB_GETTEXTIZE --force --copy
#run_or_die $INTLTOOLIZE --force --copy --automake
run_or_die $LIBTOOLIZE --force --copy --automake
run_or_die $AUTOCONF
run_or_die $AUTOHEADER
run_or_die $AUTOMAKE -a -c

cd $LAST_DIR

echo "Running 'configure' with options $GLOBOPTIONS"
./configure $GLOBOPTIONS
