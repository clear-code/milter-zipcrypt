AC_INIT(milter-zipcrypt, 1.0.0,
        [?])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AM_INIT_AUTOMAKE([1.9 tar-pax])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(config.h)

AM_PROG_LIBTOOL

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_LIB(z, inflateInit2_)
AC_CHECK_LIB(milter, mlfi_connect)
AC_CHECK_LIB(crypt, crypt)
AC_CHECK_HEADERS([libmilter/mfapi.h],,[AC_MSG_ERROR([libmilter/mfapi.h not found.])])
AC_CHECK_HEADERS([getopt.h],,[AC_MSG_ERROR([getopt.h not found.])])
AC_CHECK_FUNC([daemon],,[AC_MSG_ERROR([daemon() function not found])])

AC_PATH_PROG([PKG_CONFIG], [pkg-config], [no])

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
if test "x$ac_cv_use_gcutter" != "xno"; then
  MILTER_TEST_SERVER=`$PKG_CONFIG --variable=milter_test_server 'milter-server >= 1.5.3'`
  AC_DEFINE_UNQUOTED(MILTER_TEST_SERVER, ["$MILTER_TEST_SERVER"], [milter-test-server name])
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
test/Makefile
test/fixtures/Makefile
test/fixtures/sendmail-test/Makefile
])

AC_OUTPUT
