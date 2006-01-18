INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxdbmgr
nxdbmgr_SOURCES = nxdbmgr.cpp check.cpp init.cpp unlock.cpp upgrade.cpp
nxdbmgr_LDADD = @PTHREAD_LIBS@ ../../../libnetxms/libnetxms.la ../../../libnxcscp/libnxcscp.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	nxdbmgr.dsp nxdbmgr.dsw nxdbmgr.h
