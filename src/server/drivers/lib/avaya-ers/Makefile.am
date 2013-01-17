lib_LTLIBRARIES = libavaya-ers.la
libavaya_ers_la_SOURCES = avaya-ers.cpp vlan-if.cpp
libavaya_ers_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libavaya_ers_la_LDFLAGS = -avoid-version
libavaya_ers_la_LIBADD = ../../../../libnetxms/libnetxms.la \
			 ../../../../snmp/libnxsnmp/libnxsnmp.la \
			 ../../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	avaya-ers.vcproj \
	avaya-ers.h
