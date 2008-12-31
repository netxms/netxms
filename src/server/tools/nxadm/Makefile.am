INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxadm
nxadm_SOURCES = comm.cpp nxadm.cpp
nxadm_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../../libnxsnmp/libnxsnmp.la

EXTRA_DIST = \
	nxadm.vcproj \
	nxadm.h
