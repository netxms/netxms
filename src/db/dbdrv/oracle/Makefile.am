INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxddr_oracle.la
libnxddr_oracle_la_SOURCES = oracle.cpp
libnxddr_oracle_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxddr_oracle_la_LIBADD = ../../../libnetxms/libnetxms.la @ORACLE_LIBS@

EXTRA_DIST = \
	oracle.vcproj \
	oracledrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnxddr_oracle.a $(libdir)/libnxddr_oracle.so.1 ; fi
