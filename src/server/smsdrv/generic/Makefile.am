lib_LTLIBRARIES = libnxsms_generic.la
libnxsms_generic_la_SOURCES = main.cpp pdumode.cpp
libnxsms_generic_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsms_generic_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsms_generic_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	generic.vcproj \
	main.h
