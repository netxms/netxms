INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxddr_db2.la
libnxddr_db2_la_SOURCES = db2.cpp
libnxddr_db2_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) @DB2_LIBS@
libnxddr_db2_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = db2.vcproj db2drv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnxddr_db2.a $(libdir)/libnxddr_db2.so.0 ; fi
