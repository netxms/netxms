INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxddr_sqlite.la
libnxddr_sqlite_la_SOURCES = sqlite.cpp
libnxddr_sqlite_la_LDFLAGS = -version-info $(DBDRV_LIBRARY_VERSION) ../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	sqlite.dsp sqlite.dsw \
	sqlitedrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnxddr_sqlite.a $(libdir)/libnxddr_sqlite.so.0 ; fi
