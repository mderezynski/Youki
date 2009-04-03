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
        printf "\033[1mplease install /bin/ksh or /usr/xpg4/bin/sh\033[0m"
        exit
    fi
    export USE_SHELL
    printf "\033[1mrestarting under $USE_SHELL...\033[0m"
    $USE_SHELL $0 "$@"
    exit
}
   
TOP_DIR=$(cd $(dirname $0); echo $PWD)
LAST_DIR=$PWD

if test ! -f $TOP_DIR/configure.ac ; then
    printf "\033[1mYou must execute this script from the top level directory.\033[0m"
    exit 1
fi

# Functions
dump_help_screen ()
{
    printf "\033[1mUsage: autogen.sh [options]"
    echo 
    echo "options:"
    echo "  -h,--help             Show this help screen"
    echo "  -n,--no-log           Don't create ChangeLog from SVN commit logs" 
    echo "  -l,--last-version     Specify the revision down to which create the log from"
    printf "\033[0m"
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
            echo -e "\033[1mAssuming configure arguments!\033[0m\n"
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
        echo "\033[1;33m*warning* no command specified!\033[0m\n"
        return 1
    fi

    shift;

    OPTIONS="$@"

    # print a message
    printf "\033[1m*info* running $COMMAND\033[0m"
    if test -n "$OPTIONS" ; then
        echo " ($OPTIONS)"
    else
        echo
    fi

    # run or die
    $COMMAND $OPTIONS ; RESULT=$?
    if test $RESULT -ne 0 ; then
        echo "\033[1;31m*error* $COMMAND failed. (exit code = $RESULT)\033[0m"
        exit 1
    fi

    return 0
}

compare_version() {
	LHS=$1;
	RHS=$2;

	LHS_1=$(echo $LHS | cut -f 1 -d . | sed 's:^\([0-9]\+\).*$:\1:');
	if [ "x$LHS_1" = "x" ]; then LHS_1=0; fi
	LHS_2=$(echo $LHS | cut -f 2 -d . | sed 's:^\([0-9]\+\).*$:\1:');
	if [ "x$LHS_2" = "x" ]; then LHS_2=0; fi
	LHS_3=$(echo $LHS | cut -f 3 -d . | sed 's:^\([0-9]\+\).*$:\1:');
	if [ "x$LHS_3" = "x" ]; then LHS_3=0; fi

	RHS_1=$(echo $RHS | cut -f 1 -d . | sed 's:^\([0-9]\+\).*$:\1:');
	if [ "x$RHS_1" = "x" ]; then RHS_1=0; fi
	RHS_2=$(echo $RHS | cut -f 2 -d . | sed 's:^\([0-9]\+\).*$:\1:');
	if [ "x$RHS_2" = "x" ]; then RHS_2=0; fi
	RHS_3=$(echo $RHS | cut -f 3 -d . | sed 's:^\([0-9]\+\).*$:\1:');
	if [ "x$RHS_3" = "x" ]; then RHS_3=0; fi

	if [ $LHS_1 -ge $RHS_1 ]; then
		if [ $LHS_1 -gt $RHS_1 ]; then
			return 0;
		fi
		if [ $LHS_2 -ge $RHS_2 ]; then
			if [ $LHS_2 -gt $RHS_2 ]; then
				return 0;
			fi
			if [ $LHS_3 -ge $RHS_3 ]; then
				if [ $LHS_3 -gt $RHS_3 ]; then
					return 0;
				fi
				return 0;
			fi
			return 1;
		fi
		return 1;
	fi
	return 1;
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

# Check for proper automake version
automake_req=1.9

printf "\033[1mChecking Automake version... "

automake_version=`$AUTOMAKE --version | head -n1 | cut -f 4 -d \ `

printf "$automake_version: \033[0m"
if compare_version $automake_version $automake_req; then
	echo -e "\033[1;32mok\033[0m"
else
  echo "\033[1;31m!!"
	echo "*error*: mpx requires automake $automake_req\033[0m"
	exit 1
fi

# Check for proper autoconf version
autoconf_req=2.60

printf "\033[1mChecking Autoconf version... "

autoconf_version=`$AUTOCONF --version | head -n1 | cut -f 4 -d \ `

printf "$autoconf_version: \033[0m"
if compare_version $autoconf_version $autoconf_req; then
	echo -e "\033[1;32mok\033[0m"
else
  echo "\033[1;31m!!"
	echo "*error* mpx requires autoconf $autoconf_req\033[0m"
	exit 1
fi


# Check for proper libtool version
libtool_req=1.5.21

printf "\033[1mChecking Libtool version... "

libtool_version=`$LIBTOOLIZE --version | head -n1 | cut -f 4 -d \ `

printf "$libtool_version: \033[0m"
if compare_version $libtool_version $libtool_req; then
	echo -e "\033[1;32mok\033[0m"
else
  echo "\033[1;31m!!"
	echo "*error* mpx requires libtool $libtool_req\033[0m"
	exit 1
fi

# Check for proper gettext version
gettext_req=0.16

printf "\033[1mChecking Gettext version... "

gettext_version=`$AUTOPOINT --version | head -n1 | cut -f 4 -d \ `

printf "$gettext_version: \033[0m"
if compare_version $gettext_version $gettext_req; then
	echo -e "\033[1;32mok\033[0m"
else
  echo "\033[1;31m!!"
	echo "*error* mpx requires gettext $gettext_req\033[0m"
	exit 1
fi

# Changelog from SVN (svn2cl)
if [ -d .svn ]; then
    if [ "x$no_log" != "x1" ]; then
      # only recreate if we in svn repo
      printf "\033[1m*info* creating ChangeLog from SVN history\033[0m"
      if [ "x$lastversion" != "x" ]; then
        REVISION=$(svnversion)
        $USE_SHELL ./scripts/svn2cl.sh -i --authors=./authors.xml --break-before-msg --separate-daylogs --reparagraph -r $REVISION:$lastversion
      else
        $USE_SHELL ./scripts/svn2cl.sh -i --authors=./authors.xml --break-before-msg --separate-daylogs --reparagraph
      fi
    fi
fi

if [ ! Changelog ]; then
  touch ChangeLog
fi

run_or_die $AUTOPOINT --force
run_or_die $ACLOCAL --force -I m4
run_or_die $AUTOHEADER --force
run_or_die $LIBTOOLIZE --force --copy --automake
run_or_die $AUTOCONF --force
run_or_die $AUTOMAKE --add-missing --copy --include-deps

cd $LAST_DIR

echo
echo "\033[1m*info* running 'configure' with options: $GLOBOPTIONS\033[0m"
echo
./configure "$@"
