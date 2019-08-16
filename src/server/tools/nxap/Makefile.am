bin_PROGRAMS = nxap
nxap_SOURCES = nxap.cpp
nxap_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxap_LDFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include @EXEC_LDFLAGS@
nxap_LDADD = \
	     ../../libnxsrv/libnxsrv.la \
	     ../../../libnxsl/libnxsl.la \
	     ../../../snmp/libnxsnmp/libnxsnmp.la \
	     ../../../libnetxms/libnetxms.la \
	     @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxap.vcxproj nxap.vcxproj.filters
