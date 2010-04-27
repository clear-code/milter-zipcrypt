bin_PROGRAMS = milter-zipcrypt

milter_zipcrypt_CFLAGS =	\
	$(COVERAGE_CFLAGS)

milter_zipcrypt_SOURCES =	\
	mz-filter.h		\
	mz-filter.c		\
	milter-zipcrypt.c

milter_zipcrypt_LDADD =		\
	-L/usr/lib/libmilter	\
	-lmilter		\
	-lz
