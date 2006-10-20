INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxaction
nxaction_SOURCES = nxaction.cpp
nxaction_LDADD = @PTHREAD_LIBS@ ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	nxaction.dsp nxaction.dsw
