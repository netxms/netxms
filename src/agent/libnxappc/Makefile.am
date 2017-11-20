SOURCES = nxappc.c

lib_LTLIBRARIES = libnxappc.la

libnxappc_la_SOURCES = $(SOURCES)
libnxappc_la_CPPFLAGS=-I@top_srcdir@/include
libnxappc_la_LDFLAGS = -static

EXTRA_DIST = nxappc.h libnxappc.vcxproj libnxappc.vcxproj.filters

install-exec-hook:
	if test "x@BUILD_SDK@" = "xyes"; then mkdir -p $(DESTDIR)$(includedir); cp nxappc.h $(DESTDIR)$(includedir); fi
