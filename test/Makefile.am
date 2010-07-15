SUBDIRS = fixtures

if WITH_GCUTTER
TESTS_ENVIRONMENT = 		\
	NO_MAKE=yes 		\
	CUTTER="$(CUTTER)"	\
	$(CUTTER) 		\
	--source-directory=$(top_srcdir) .
TESTS = test

check_LTLIBRARIES =		\
	libmz_test_utils.la	\
	test_base64.la		\
	test_config.la		\
	test_convert.la		\
	test_milter_zipcrypt.la	\
	test_password.la	\
	test_sendmail.la		\
	test_utils.la		\
	test_zip_stream.la
endif

EXTRA_DIST =			\
	mz-test-sendmail.xml	\
	run-test.sh

check_HEADERS = 			\
	mz-test-sendmail-receiver.h

INCLUDES =			\
	-I$(srcdir)		\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/src	\
	-I$(top_srcdir)/test

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

mz-test-sendmail-receiver.h: mz-test-sendmail.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=mz_test_sendmail --mode=glib-server $< > $@

libmz_test_utils_la_SOURCES = 				\
	mz-test-utils.c					\
	mz-test-utils.h

libmz_test_utils_la_LIBADD =				\
	$(top_builddir)/src/libmilter_zipcrypt.la

test_utils_la_SOURCES = test-utils.c
test_utils_la_LIBADD =					\
	$(top_builddir)/test/libmz_test_utils.la

test_base64_la_SOURCES = test-base64.c
test_base64_la_LIBADD =					\
	$(top_builddir)/test/libmz_test_utils.la

test_config_la_SOURCES = test-config.c
test_config_la_LIBADD =				\
	$(top_builddir)/test/libmz_test_utils.la

test_convert_la_SOURCES = test-convert.c
test_convert_la_LIBADD =				\
	$(top_builddir)/test/libmz_test_utils.la

test_milter_zipcrypt_la_SOURCES = test-milter-zipcrypt.c
test_milter_zipcrypt_la_LIBADD =			\
	$(top_builddir)/test/libmz_test_utils.la

test_password_la_SOURCES = test-password.c
test_password_la_LIBADD =				\
	$(top_builddir)/test/libmz_test_utils.la

test_sendmail_la_CFLAGS = 				\
	$(AM_CFLAGS)					\
	$(DBUS_GLIB_CFLAGS)
test_sendmail_la_SOURCES = 				\
	test-sendmail.c					\
	mz-test-sendmail-receiver.h
test_sendmail_la_LIBADD =				\
	$(DBUS_GLIB_LIBS)				\
	$(top_builddir)/test/libmz_test_utils.la

test_zip_stream_la_SOURCES = test-zip-stream.c
test_zip_stream_la_LIBADD =				\
	$(top_builddir)/test/libmz_test_utils.la

test:
	@

echo-cutter:
	@echo $(CUTTER)
