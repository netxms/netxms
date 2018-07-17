bin_PROGRAMS = nxget
nxget_SOURCES = nxget.cpp
nxget_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxget_LDFLAGS = @EXEC_LDFLAGS@
nxget_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../../snmp/libnxsnmp/libnxsnmp.la @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxget.vcxproj nxget.vcxproj.filters
