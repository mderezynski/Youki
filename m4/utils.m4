#-*- Mode: Autoconf; -*-

dnl BMP_WRAPPED_CHECK([BODY])
AC_DEFUN([BMP_WRAPPED_CHECK],
    [AC_LANG_ASSERT([C++])
     CXXFLAGS_SAVE="$CXXFLAGS"
     LIBS_SAVE="$LIBS"
     $1
     CXXFLAGS="$CXXFLAGS_SAVE"
     LIBS="$LIBS_SAVE"
    ])
