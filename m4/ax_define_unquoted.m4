AC_DEFUN([AX_DEFINE_UNQUOTED], [
  prefix_NONE=
  exec_prefix_NONE=
  test "x$prefix" = xNONE && prefix_NONE=yes && prefix=$ac_default_prefix
  test "x$exec_prefix" = xNONE && exec_prefix_NONE=yes && exec_prefix=$prefix
  ax_cv_str="$2";
  while `echo "$ax_cv_str" | grep '\$\({.*}\|[:alnum:]\+\)' -q`; do
    eval ax_cv_str="\"$ax_cv_str\"";
  done
  AC_SUBST($1, "$ax_cv_str")
  AC_DEFINE_UNQUOTED($1, "$ax_cv_str", [$3])
  test "$prefix_NONE" && prefix=NONE
  test "$exec_prefix_NONE" && exec_prefix=NONE
])
