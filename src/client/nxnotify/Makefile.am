bin_PROGRAMS = nxnotify
nxnotify_SOURCES = nxnotify.cpp
nxnotify_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxnotify_LDFLAGS = @EXEC_LDFLAGS@
nxnotify_LDADD = ../../libnetxms/libnetxms.la ../libnxclient/libnxclient.la @EXEC_LIBS@

EXTRA_DIST = nxnotify.vcxproj nxnotify.vcxproj.filters nxnotify.h
