dnl -*- Mode: Autoconf; -*-

AC_DEFUN([MPX_CHECK_HAL_NEWPSI],
[
	CPPFLAGS_SAVED="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $SOUP_CFLAGS"
	export CPPFLAGS

	LDFLAGS_SAVED="$LDFLAGS"
	LDFLAGS="$LDFLAGS $SOUP_LDFLAGS"
	export LDFLAGS

	AC_MSG_CHECKING(for HAL PSI iterator name)

	AC_LANG_PUSH(C++)
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	@%:@include <hal/libhal.h>
	]], [[
    LibHalPropertySetIterator iter;
    iter.idx = 0;
	]])],[
	AC_MSG_RESULT(no)
	],[
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_HAL_NEWPSI,,[define when having newstyle HAL PSI])
	])
	AC_LANG_POP([C++])

	CPPFLAGS="$CPPFLAGS_SAVED"
	LDFLAGS="$LDFLAGS_SAVED"
])
