dnl -*- Mode: Autoconf; -*-

AC_DEFUN([BMP_CHECK_SOUP_EXTERN_C],
[
	CPPFLAGS_SAVED="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $SOUP_CFLAGS"
	export CPPFLAGS

	LDFLAGS_SAVED="$LDFLAGS"
	LDFLAGS="$LDFLAGS $SOUP_LDFLAGS"
	export LDFLAGS

	AC_MSG_CHECKING(for extern in libsoup headers)

	AC_LANG_PUSH(C++)
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	@%:@include <libsoup/soup.h>
	]], [[
	SoupSession * session = soup_session_sync_new ();
	]])],[
	AC_MSG_RESULT(no)
	],[
	AC_MSG_RESULT(yes)
	AC_DEFINE(LIBSOUP_HAS_EXTERN_C,,[define if libsoup has extern c])
	])
	AC_LANG_POP([C++])

	CPPFLAGS="$CPPFLAGS_SAVED"
	LDFLAGS="$LDFLAGS_SAVED"
])
