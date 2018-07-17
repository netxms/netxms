bin_PROGRAMS = nxsms
nxsms_SOURCES = nxsms.cpp
nxsms_CPPFLAGS = -I@top_srcdir@/include
nxsms_LDFLAGS = @EXEC_LDFLAGS@
nxsms_LDADD = ../../libnetxms/libnetxms.la ../libnxclient/libnxclient.la @EXEC_LIBS@

EXTRA_DIST = Makefile.w32 nxsms.vcxproj nxsms.vcxproj.filters nxsms.h
