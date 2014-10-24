lib_LTLIBRARIES = libnxsms_kannel.la
libnxsms_kannel_la_SOURCES = main.cpp
libnxsms_kannel_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsms_kannel_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsms_kannel_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la -lcurl

EXTRA_DIST = \
	kannel.vcproj
