dnl -*- Mode: Autoconf; -*-

dnl BMP_CHECK_SMLIB([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([BMP_CHECK_SMLIB],
    [AC_REQUIRE([AC_PATH_XTRA])

     AC_ARG_VAR([SM_CFLAGS], [C compiler flags for SMlib])
     AC_ARG_VAR([SM_LIBS], [linker flags for SMlib])

     if test -z "$SM_CFLAGS"; then
         SM_CFLAGS="$X_CFLAGS"
     fi

     if test -z "$SM_LIBS"; then
         SM_LIBS="-lSM -lICE"
     fi

     BMP_WRAPPED_CHECK(
         [AC_CHECK_LIB([SM], [SmcOpenConnection], 
              [$1],
              [$2],
              [$X_LIBS -lICE])
         ])

     AC_SUBST([SM_CFLAGS])
     AC_SUBST([SM_LIBS])
    ]
)
