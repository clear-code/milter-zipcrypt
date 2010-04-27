if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES =			\
	test_filter.la
endif

EXTRA_DIST =		\
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

LIBS =						\
	-L/usr/lib/libmilter	\
	-lmilter		\
	$(CUTTER_LIBS)

test_filter_la_SOURCES = test-filter.c
test_filter_la_LIBADD = 				\
	$(top_builddir)/src/milter_ziprypt-mz-filter.o

echo-cutter:
	@echo $(CUTTER)
