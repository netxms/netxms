INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/src/sqlite

lib_LTLIBRARIES = libnxddr_sqlite.la
libnxddr_sqlite_la_SOURCES = sqlite.cpp
libnxddr_sqlite_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxddr_sqlite_la_LIBADD = ../../../libnetxms/libnetxms.la ../../../sqlite/libnxsqlite.la

EXTRA_DIST = \
	sqlite.vcproj \
	sqlitedrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnxddr_sqlite.a $(libdir)/libnxddr_sqlite.so.0 ; fi
