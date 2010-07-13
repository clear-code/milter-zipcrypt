INCLUDES = 			\
	-I$(top_builddir)	\
	-I$(top_builddir)/test	\
	-I$(top_srcdir)

AM_CFLAGS = 			\
	$(GCUTTER_CFLAGS)	\
	$(DBUS_GLIB_CFLAGS)

if WITH_GCUTTER
noinst_PROGRAMS = sendmail-test
endif

BUILT_SOURCES = 			\
	mz-test-sendmail-reporter.h

mz-test-sendmail-reporter.h: $(top_srcdir)/test/mz-test-sendmail.xml
	dbus-binding-tool --prefix=mz_test_sendmail --mode=glib-client $< > $@

sendmail_test_LDADD = 		\
	$(GCUTTER_LIBS)		\
	$(DBUS_GLIB_LIBS)
sendmail_test_SOURCES = 					\
	mz-test-sendmail-reporter.h	\
	sendmail-test.c
