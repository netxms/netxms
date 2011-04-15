INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libcisco.la
libavaya_ers_la_SOURCES = cisco.cpp
libavaya_ers_la_LDFLAGS = -avoid-version
libavaya_ers_la_LIBADD = ../../../../libnetxms/libnetxms.la ../../../../snmp/libnxsnmp/libnxsnmp.la

EXTRA_DIST = \
	cisco.vcproj \
	cisco.h
