bin_PROGRAMS = nxminfo
nxminfo_SOURCES = nxminfo.cpp
nxminfo_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxminfo_LDFLAGS = @EXEC_LDFLAGS@
nxminfo_LDADD = \
	../../libnxsrv/libnxsrv.la \
	../../../libnxsl/libnxsl.la \
	../../../snmp/libnxsnmp/libnxsnmp.la \
	../../../libnetxms/libnetxms.la \
	@SERVER_LIBS@ \
	@EXEC_LIBS@

EXTRA_DIST = \
	nxminfo.vcxproj nxminfo.vcxproj.filters
