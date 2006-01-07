INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = netxmsd
netxmsd_SOURCES = netxmsd.cpp
netxmsd_LDADD = @PTHREAD_LIBS@ ../../libnetxms/libnetxms.la ../../libnxcscp/libnxcscp.la ../../libnxsnmp/libnxsnmp.la ../../libnxsl/libnxsl.la ../libnxsrv/libnxsrv.la ../core/libnxcore.la

EXTRA_DIST = \
	netxmsd.dsp netxmsd.dsw \
   netxmsd.h
