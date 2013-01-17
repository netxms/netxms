SOURCES = element.cpp link.cpp main.cpp objlist.cpp

lib_LTLIBRARIES = libnxmap.la

libnxmap_la_SOURCES = $(SOURCES)
libnxmap_la_CPPFLAGS=-I@top_srcdir@/include
libnxmap_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxmap_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxmap.vcproj \
	libnxmap.h
