#!/bin/sh

RM="rm -v"

remove_makefile_ins()
{
    # scan for Makefile.ins and remove only if 
    # there is a corresponding Makefile.am
    
    Files=`find ./ -name Makefile.in`
    
    for File in $Files; do
	
	if test -e "`dirname $File`/Makefile.am" ; then
	    $RM $File
	else
	    echo *info* Not deleting $File...
	fi
	
    done
}


# lame test for top level directory
if ! test -f configure.ac ; then
    echo "You must execute this script from the top level directory"
fi

# remove files from aclocal
$RM -f aclocal.m4

# remove files from autoheader
$RM -f config.h.in

# remove files from libtoolize
$RM -f ltmain.sh libtool

# remove files from autopoint
$RM -f ABOUT-NLS config.rpath
$RM -f po/Makefile.in* po/*.{sed,header,sin} po/Makevars.template*
$RM -rf intl
./scripts/rm-backups.sh po -fv

# remove files from automake
$RM -f missing mkinstalldirs install-sh
$RM -f depcomp compile
remove_makefile_ins

# remove files from autoconf and configure
$RM -f configure config.status
$RM -rf autom4te.cache

# remove architecture detection scripts
$RM -f config.guess config.sub

# remove files from cvs2cl.pl
$RM -f ChangeLog
