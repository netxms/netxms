INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxaction
nxaction_SOURCES = nxaction.cpp
nxaction_LDADD = @PTHREAD_LIBS@ ../../../libnetxms/libnetxms.la ../../../libnxcscp/libnxcscp.la ../../libnxsrv/libnxsrv.la -lsnmp
