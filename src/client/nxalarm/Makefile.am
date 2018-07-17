bin_PROGRAMS = nxalarm
nxalarm_SOURCES = nxalarm.cpp
nxalarm_CPPFLAGS = -I@top_srcdir@/include
nxalarm_LDFLAGS = @EXEC_LDFLAGS@
nxalarm_LDADD = ../../libnetxms/libnetxms.la ../libnxclient/libnxclient.la @EXEC_LIBS@

EXTRA_DIST = Makefile.w32 nxalarm.vcxproj nxalarm.vcxproj.filters nxalarm.h
