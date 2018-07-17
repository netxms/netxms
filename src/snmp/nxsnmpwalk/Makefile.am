bin_PROGRAMS = nxsnmpwalk
nxsnmpwalk_SOURCES = nxsnmpwalk.cpp
nxsnmpwalk_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxsnmpwalk_LDFLAGS = @EXEC_LDFLAGS@
nxsnmpwalk_LDADD = ../../libnetxms/libnetxms.la ../libnxsnmp/libnxsnmp.la @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxsnmpwalk.vcxproj nxsnmpwalk.vcxproj.filters
