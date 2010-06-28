AC_INIT(milter-zipcrypt, 1.0.0,
        [?])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AM_INIT_AUTOMAKE([1.9 tar-pax])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([m4])

AM_PROG_LIBTOOL

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_LIB(z, inflateInit2_)
AC_CHECK_LIB(milter, mlfi_connect)
AC_CHECK_LIB(crypt, crypt)
AC_CHECK_HEADERS([libmilter/mfapi.h],,[AC_MSG_ERROR([libmilter/mfapi.h does not found.])])
AC_CHECK_HEADERS([getopt.h],,[AC_MSG_ERROR([getopt.h does not found.])])

AM_ICONV_LINK(iconv)

if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[[\ \	]]-Wall[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi

m4_ifdef([AC_CHECK_GCUTTER], [
AC_CHECK_GCUTTER(>= 1.1.5)
],
[ac_cv_use_gcutter="no"])
AM_CONDITIONAL([WITH_GCUTTER], [test "$ac_cv_use_gcutter" != "no"])

AC_CONFIG_FILES([
Makefile
src/Makefile
test/Makefile
])

AC_OUTPUT
