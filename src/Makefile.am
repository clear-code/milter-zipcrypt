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
	mz-base64.h		\
	mz-base64.c		\
	mz-quoted-printable.h	\
	mz-quoted-printable.c	\
	mz-mime-private.h	\
	mz-utils.h		\
	mz-utils.c		\
	mz-zip.h

EXTRA_DIST = gen-table.c
