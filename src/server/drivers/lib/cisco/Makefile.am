lib_LTLIBRARIES = libcisco.la
libcisco_la_SOURCES = cisco.cpp
libcisco_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libcisco_la_LDFLAGS = -avoid-version
libcisco_la_LIBADD = ../../../../libnetxms/libnetxms.la \
		     ../../../../snmp/libnxsnmp/libnxsnmp.la \
		     ../../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	cisco.vcxproj cisco.vcxproj.filters \
	cisco.h \
	Makefile.w32

