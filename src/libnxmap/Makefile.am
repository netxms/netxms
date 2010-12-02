INCLUDES=-I@top_srcdir@/include

SOURCES = element.cpp graph.cpp link.cpp main.cpp map.cpp objlist.cpp \
          radial.cpp rt.cpp submap.cpp vertex.cpp

if BUILD_UNICODE_LIBS
lib_LTLIBRARIES = libnxmap.la libnxmapw.la
else
lib_LTLIBRARIES = libnxmap.la
endif

libnxmap_la_SOURCES = $(SOURCES)
libnxmap_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxmap_la_LIBADD = ../libnetxms/libnetxms.la

libnxmapw_la_SOURCES = $(SOURCES)
libnxmapw_la_CPPFLAGS = -DUNICODE
libnxmapw_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxmapw_la_LIBADD = ../libnetxms/libnetxmsw.la

EXTRA_DIST = \
	libnxmap.vcproj libnxmapw.vcproj \
	libnxmap.h
