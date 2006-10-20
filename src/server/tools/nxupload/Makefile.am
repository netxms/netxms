INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxupload
nxupload_SOURCES = nxupload.cpp
nxupload_LDADD = @PTHREAD_LIBS@ ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	nxupload.dsp nxupload.dsw
