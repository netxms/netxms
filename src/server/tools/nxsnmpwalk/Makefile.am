INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxsnmpwalk
nxsnmpwalk_SOURCES = nxsnmpwalk.cpp
nxsnmpwalk_LDADD = @PTHREAD_LIBS@ ../../../libnetxms/libnetxms.la ../../../libnxsnmp/libnxsnmp.la

EXTRA_DIST = \
	nxsnmpwalk.dsp nxsnmpwalk.dsw
