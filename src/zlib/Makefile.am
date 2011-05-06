## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libnxzlib.la
libnxzlib_la_LDFLAGS = -no-undefined -version-info 1:0:0 $(LDFLAGS)

noinst_HEADERS = \
	crc32.h \
	deflate.h \
	inffast.h \
	inffixed.h \
	inflate.h \
	inftrees.h \
	trees.h \
	zconf.h \
	zlib.h \
	zutil.h

libnxzlib_la_SOURCES = \
	adler32.c \
	compress.c \
	crc32.c \
	deflate.c \
	gzio.c \
	infback.c \
	inffast.c \
	inflate.c \
	inftrees.c \
	trees.c \
	uncompr.c \
	zutil.c

EXTRA_DIST = \
	ChangeLog \
	README \
	nxzlib.def \
	nxzlib.vcproj
