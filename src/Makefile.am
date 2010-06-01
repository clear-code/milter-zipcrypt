bin_PROGRAMS =			\
	milter-zipcrypt

noinst_LTLIBRARIES =		\
	libmilter_zipcrypt.la

AM_CFLAGS =			\
	$(COVERAGE_CFLAGS)

milter_zipcrypt_SOURCES =	\
	milter-zipcrypt.c

milter_zipcrypt_LDADD =		\
	libmilter_zipcrypt.la	\
	-L/usr/lib/libmilter	\
	-lmilter		\
	-lz

libmilter_zipcrypt_la_SOURCES =	\
	mz-attachment.h		\
	mz-attachment.c		\
	mz-utils.h		\
	mz-utils.c		\
	base64.h		\
	base64.c

