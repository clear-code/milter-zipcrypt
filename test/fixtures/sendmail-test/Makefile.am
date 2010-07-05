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

sendmail_test_LDADD = 		\
	$(GCUTTER_LIBS)		\
	$(DBUS_GLIB_LIBS)
sendmail_test_SOURCES = sendmail-test.c
