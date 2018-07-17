bin_PROGRAMS = nxaction
nxaction_SOURCES = nxaction.cpp
nxaction_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxaction_LDFLAGS = @EXEC_LDFLAGS@
nxaction_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../../snmp/libnxsnmp/libnxsnmp.la @EXEC_LIBS@

EXTRA_DIST = \
	nxaction.vcxproj nxaction.vcxproj.filters \
	Makefile.w32
