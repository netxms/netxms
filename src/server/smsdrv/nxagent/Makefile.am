lib_LTLIBRARIES = libnxsms_nxagent.la
libnxsms_nxagent_la_SOURCES = main.cpp
libnxsms_nxagent_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsms_nxagent_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsms_nxagent_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	nxagent.vcproj
