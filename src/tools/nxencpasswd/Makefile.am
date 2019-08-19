bin_PROGRAMS = nxencpasswd
nxencpasswd_SOURCES = nxencpasswd.cpp
nxencpasswd_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxencpasswd_LDFLAGS = -I@top_srcdir@/include @EXEC_LDFLAGS@
nxencpasswd_LDADD = ../../libnetxms/libnetxms.la @EXEC_LIBS@

EXTRA_DIST = \
	nxencpasswd.vcxproj nxencpasswd.vcxproj.filters \
	Makefile.w32
