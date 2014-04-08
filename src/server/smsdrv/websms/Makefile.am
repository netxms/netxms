lib_LTLIBRARIES = libnxsms_websms.la
libnxsms_websms_la_SOURCES = main.cpp
libnxsms_websms_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsms_websms_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsms_websms_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la -lcurl

EXTRA_DIST = \
	websms.vcproj
