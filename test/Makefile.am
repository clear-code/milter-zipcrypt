if WITH_GCUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES =		\
	libmz_test_utils.la	\
	test_base64.la		\
	test_convert.la		\
	test_utils.la		\
	test_zip_stream.la
endif

EXTRA_DIST =			\
	run-test.sh

INCLUDES =			\
	-I$(srcdir)		\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/src

AM_CFLAGS = 			\
	$(GCUTTER_CFLAGS)

AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =							\
	-L/usr/lib/libmilter				\
	$(GCUTTER_LIBS)

libmz_test_utils_la_SOURCES = mz-test-utils.c
libmz_test_utils_la_LIBADD =				\
	$(top_builddir)/src/libmilter_zipcrypt.la

test_utils_la_SOURCES = test-utils.c
test_utils_la_LIBADD =					\
	$(top_builddir)/test/libmz_test_utils.la

test_base64_la_SOURCES = test-base64.c
test_base64_la_LIBADD =					\
	$(top_builddir)/test/libmz_test_utils.la

test_convert_la_SOURCES = test-convert.c
test_convert_la_LIBADD =					\
	$(top_builddir)/test/libmz_test_utils.la

test_zip_stream_la_SOURCES = test-zip-stream.c
test_zip_stream_la_LIBADD =				\
	$(top_builddir)/test/libmz_test_utils.la

echo-cutter:
	@echo $(CUTTER)
