INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxget
nxget_SOURCES = nxget.cpp
nxget_LDADD = @PTHREAD_LIBS@ ../../../libnetxms/libnetxms.la ../../../libnxcscp/libnxcscp.la ../../libnxsrv/libnxsrv.la
