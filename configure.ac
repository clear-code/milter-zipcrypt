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

AM_ICONV_LINK(iconv)

if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[[\ \	]]-Wall[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi

m4_ifdef([AC_CHECK_CUTTER], [
AC_CHECK_CUTTER(>= 1.0.6)
],
[ac_cv_use_cutter="no"])
AM_CONDITIONAL([WITH_CUTTER], [test "$ac_cv_use_cutter" != "no"])

AC_CONFIG_FILES([
Makefile
src/Makefile
zip30/Makefile
test/Makefile
])

AC_OUTPUT
