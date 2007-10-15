dnl @synopsis BMP_CHECK_SQLITE3([MINIMUM-VERSION])
dnl
dnl Test for the SQLite 3 library of a particular version (or newer)
dnl
dnl @license AllPermissive

AC_DEFUN([BMP_CHECK_SQLITE3],
[
    SQLITE3_VERSION=""

    ac_sqlite3_header="sqlite3.h"

    sqlite3_version_req=ifelse([$1], [], [3.0.0], [$1])
    sqlite3_version_req_shorten=`expr $sqlite3_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
    sqlite3_version_req_major=`expr $sqlite3_version_req : '\([[0-9]]*\)'`
    sqlite3_version_req_minor=`expr $sqlite3_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
    sqlite3_version_req_micro=`expr $sqlite3_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
    if test "x$sqlite3_version_req_micro" = "x" ; then
        sqlite3_version_req_micro="0"
    fi

    sqlite3_version_req_number=`expr $sqlite3_version_req_major \* 1000000 \
                               \+ $sqlite3_version_req_minor \* 1000 \
                               \+ $sqlite3_version_req_micro`

    AC_MSG_CHECKING([for SQLite3 library >= $sqlite3_version_req])

    for ac_sqlite3_path_tmp in $2 /usr ; do
        if test -f "$ac_sqlite3_path_tmp/include/$ac_sqlite3_header" \
            && test -r "$ac_sqlite3_path_tmp/include/$ac_sqlite3_header"; then
            ac_sqlite3_path=$ac_sqlite3_path_tmp
            ac_sqlite3_ldflags="-I$ac_sqlite3_path_tmp/include"
            ac_sqlite3_cppflags="-L$ac_sqlite3_path_tmp/lib"
            break;
        fi
    done

    ac_sqlite3_header_path="$ac_sqlite3_path/include/$ac_sqlite3_header"

    dnl Retrieve SQLite release version
    if test "x$ac_sqlite3_header_path" != "x"; then
        ac_sqlite3_version=`cat $ac_sqlite3_header_path \
            | grep '#define.*SQLITE_VERSION.*\"' | sed -e 's/.* "//' \
                | sed -e 's/"//'`
        if test $ac_sqlite3_version != ""; then
            SQLITE3_VERSION=$ac_sqlite3_version
            AC_MSG_RESULT([yes])
        else
            AC_MSG_RESULT([Can not find SQLITE_VERSION macro in sqlite3.h header to retrieve SQLite version!])
        fi
    fi

    AC_SUBST(SQLITE3_VERSION)
])
