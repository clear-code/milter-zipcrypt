if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES =		\
	test_base64.la		\
	test_utils.la		\
	test_zlib.la
endif

EXTRA_DIST =			\
	run-test.sh

INCLUDES =			\
	-I$(srcdir)		\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/src

AM_CFLAGS = 			\
	$(CUTTER_CFLAGS)

AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =				\
	-L/usr/lib/libmilter	\
	$(CUTTER_LIBS)

test_utils_la_SOURCES = test-utils.c
test_utils_la_LIBADD =					\
	$(top_builddir)/src/libmilter_zipcrypt.la

test_base64_la_SOURCES = test-base64.c
test_base64_la_LIBADD =					\
	$(top_builddir)/src/libmilter_zipcrypt.la

test_zlib_la_SOURCES = test-zlib.c
test_zlib_la_LIBADD =					\
	$(top_builddir)/src/libmilter_zipcrypt.la

echo-cutter:
	@echo $(CUTTER)
