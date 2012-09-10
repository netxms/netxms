INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = netxmsd
netxmsd_SOURCES = netxmsd.cpp
netxmsd_LDADD = ../../libnetxms/libnetxms.la \
                ../../snmp/libnxsnmp/libnxsnmp.la \
                ../../libnxsl/libnxsl.la \
                ../../libnxmap/libnxmap.la \
                ../../libnxlp/libnxlp.la \
                ../../db/libnxdb/libnxdb.la \
                ../libnxsrv/libnxsrv.la \
                ../core/libnxcore.la \
                @SERVER_LIBS@

EXTRA_DIST = \
	netxmsd.vcproj \
	netxmsd.h \
	winsrv.cpp
