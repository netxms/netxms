INCLUDES=-I@top_srcdir@/include

SOURCES = element.cpp graph.cpp link.cpp main.cpp map.cpp objlist.cpp \
          radial.cpp rt.cpp submap.cpp vertex.cpp

lib_LTLIBRARIES = libnxmap.la

libnxmap_la_SOURCES = $(SOURCES)
libnxmap_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxmap_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxmap.vcproj \
	libnxmap.h
