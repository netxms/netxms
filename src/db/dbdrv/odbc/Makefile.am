INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxddr_odbc.la
libnxddr_odbc_la_SOURCES = odbc.cpp
libnxddr_odbc_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) @ODBC_LIBS@
libnxddr_odbc_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = odbc.vcproj odbcdrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnxddr_odbc.a $(libdir)/libnxddr_odbc.so.0 ; fi
