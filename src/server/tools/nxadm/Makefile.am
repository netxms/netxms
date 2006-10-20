INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxadm
nxadm_SOURCES = comm.cpp nxadm.cpp
nxadm_LDADD = @PTHREAD_LIBS@ ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	nxadm.dsp nxadm.dsw \
	nxadm.h
