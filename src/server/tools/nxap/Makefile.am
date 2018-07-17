bin_PROGRAMS = nxap
nxap_SOURCES = nxap.cpp
nxap_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxap_LDFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include @EXEC_LDFLAGS@
nxap_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../../snmp/libnxsnmp/libnxsnmp.la @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxap.vcxproj nxap.vcxproj.filters
