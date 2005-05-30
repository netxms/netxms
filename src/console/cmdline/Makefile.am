INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxcmd
nxcmd_SOURCES = main.cpp cmdline.cpp
nxcmd_LDADD = @PTHREAD_LIBS@ -ldl ../../libnetxms/libnetxms.la ../../libnxcscp/libnxcscp.la ../../libnxcl/libnxcl.la

EXTRA_DIST = nxcmd.dsp nxcmd.dsw nxcmd.h
