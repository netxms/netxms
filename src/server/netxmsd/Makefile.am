bin_PROGRAMS = netxmsd
netxmsd_SOURCES = netxmsd.cpp
netxmsd_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include @PYTHON_CPPFLAGS@ -I@top_srcdir@/build
netxmsd_LDFLAGS = @PYTHON_LDFLAGS@ @EXEC_LDFLAGS@ @LIBISOTREE_LDFLAGS@
netxmsd_LDADD = \
	../core/libnxcore.la \
	../libnxsrv/libnxsrv.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../../ethernetip/libethernetip/libethernetip.la \
	../../libnxsl/libnxsl.la \
	../../libnxlp/libnxlp.la \
	../../db/libnxdb/libnxdb.la \
	../../agent/libnxagent/libnxagent.la \
	../../libnetxms/libnetxms.la \
   @SERVER_LIBS@ @EXEC_LIBS@

EXTRA_DIST = \
	netxmsd.vcxproj netxmsd.vcxproj.filters \
	netxmsd.h \
	winsrv.cpp
