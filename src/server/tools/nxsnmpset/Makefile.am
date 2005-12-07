INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxsnmpset
nxsnmpset_SOURCES = nxsnmpset.cpp
nxsnmpset_LDADD = @PTHREAD_LIBS@ ../../../libnetxms/libnetxms.la ../../../libnxsnmp/libnxsnmp.la

EXTRA_DIST = \
	nxsnmpset.dsp nxsnmpset.dsw
