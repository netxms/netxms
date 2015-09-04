lib_LTLIBRARIES = libnxsms_text2reach.la
libnxsms_text2reach_la_SOURCES = main.cpp
libnxsms_text2reach_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsms_text2reach_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsms_text2reach_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la -lcurl

EXTRA_DIST = \
	text2reach.vcproj
