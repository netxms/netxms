INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxddr_pgsql.la
libnxddr_pgsql_la_SOURCES = pgsql.cpp
libnxddr_pgsql_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) ../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	pgsql.vcproj \
	pgsqldrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnxddr_pgsql.a $(libdir)/libnxddr_pgsql.so.0 ; fi
