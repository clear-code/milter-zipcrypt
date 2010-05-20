bin_PROGRAMS = milter-zipcrypt

milter_zipcrypt_CFLAGS =	\
	$(COVERAGE_CFLAGS)

milter_zipcrypt_SOURCES =	\
	mz-filter.h		\
	mz-filter.c		\
	base64.h		\
	base64.c		\
	milter-zipcrypt.c

milter_zipcrypt_LDADD =		\
	-L/usr/lib/libmilter	\
	-lmilter		\
	-lz
