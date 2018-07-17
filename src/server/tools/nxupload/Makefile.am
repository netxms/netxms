bin_PROGRAMS = nxupload
nxupload_SOURCES = nxupload.cpp
nxupload_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxupload_LDFLAGS = @EXEC_LDFLAGS@
nxupload_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../../snmp/libnxsnmp/libnxsnmp.la @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxupload.vcxproj nxupload.vcxproj.filters
