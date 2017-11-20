bin_PROGRAMS = nxencpasswd
nxencpasswd_SOURCES = nxencpasswd.cpp
nxencpasswd_CPPFLAGS=-I@top_srcdir@/include
nxencpasswd_LDADD = ../../libnetxms/libnetxms.la

if USE_INTERNAL_LIBTRE
nxencpasswd_LDADD += ../../libtre/libnxtre.la
endif

EXTRA_DIST = \
	nxencpasswd.vcxproj nxencpasswd.vcxproj.filters \
	Makefile.w32
