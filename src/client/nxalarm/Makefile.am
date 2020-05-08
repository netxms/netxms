bin_PROGRAMS = nxalarm
nxalarm_SOURCES = nxalarm.cpp
nxalarm_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxalarm_LDFLAGS = @EXEC_LDFLAGS@
nxalarm_LDADD = ../../libnetxms/libnetxms.la ../libnxclient/libnxclient.la @EXEC_LIBS@

EXTRA_DIST = nxalarm.vcxproj nxalarm.vcxproj.filters nxalarm.h
