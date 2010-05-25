if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES =		\
	test_utils.la
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
test_utils_la_LIBADD =						\
	$(top_builddir)/src/milter_zipcrypt-mz-attachment.o	\
	$(top_builddir)/src/milter_zipcrypt-mz-utils.o		\
	$(top_builddir)/src/milter_zipcrypt-base64.o

echo-cutter:
	@echo $(CUTTER)
