bin_PROGRAMS = netxmsd
netxmsd_SOURCES = netxmsd.cpp
netxmsd_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
netxmsd_LDADD = \
	../core/libnxcore.la \
	../libnxsrv/libnxsrv.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnxsl/libnxsl.la \
	../../libnxmap/libnxmap.la \
	../../libnxlp/libnxlp.la \
	../../db/libnxdb/libnxdb.la \
	../../agent/libnxagent/libnxagent.la \
	../../libnetxms/libnetxms.la \
   @SERVER_LIBS@

EXTRA_DIST = \
	netxmsd.vcproj \
	netxmsd.h \
	winsrv.cpp
