lib_LTLIBRARIES = libnxsms_dummy.la
libnxsms_dummy_la_SOURCES = main.cpp
libnxsms_dummy_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsms_dummy_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsms_dummy_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	dummy.vcproj
