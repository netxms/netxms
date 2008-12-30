INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxddr_sqlite.la
libnxddr_sqlite_la_SOURCES = sqlite.cpp sqlite3.c
libnxddr_sqlite_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) ../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	sqlite.vcproj \
	sqlitedrv.h sqlite3ext.h sqlite3.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnxddr_sqlite.a $(libdir)/libnxddr_sqlite.so.0 ; fi
