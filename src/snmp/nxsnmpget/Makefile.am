bin_PROGRAMS = nxsnmpget
nxsnmpget_SOURCES = nxsnmpget.cpp
nxsnmpget_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxsnmpget_LDFLAGS = @EXEC_LDFLAGS@
nxsnmpget_LDADD = ../../libnetxms/libnetxms.la ../libnxsnmp/libnxsnmp.la @EXEC_LIBS@

EXTRA_DIST = \
	nxsnmpget.vcxproj nxsnmpget.vcxproj.filters
