SOURCES = ceh.cpp comm.cpp main.cpp

lib_LTLIBRARIES = libnxcc.la

libnxcc_la_SOURCES = $(SOURCES)
libnxcc_la_CPPFLAGS=-I@top_srcdir@/include
libnxcc_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxcc_la_LIBADD = @top_srcdir@/src/libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxcc.vcproj \
	libnxcc.h
