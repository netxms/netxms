lib_LTLIBRARIES = libnxsms_portech.la
libnxsms_portech_la_SOURCES = main.cpp pdumode.cpp
libnxsms_portech_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsms_portech_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsms_portech_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	portech.h \
	portech.vcproj \
	README.txt
