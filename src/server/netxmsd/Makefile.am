INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = netxmsd
netxmsd_SOURCES = netxmsd.cpp
netxmsd_LDADD = @PTHREAD_LIBS@ ../../libnetxms/libnetxms.la ../../libnxsnmp/libnxsnmp.la ../../libnxsl/libnxsl.la ../../libnxmap/libnxmap.la ../libnxsrv/libnxsrv.la ../core/libnxcore.la

EXTRA_DIST = \
	netxmsd.dsp netxmsd.dsw \
   netxmsd.h
