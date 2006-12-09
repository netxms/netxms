INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxmap.la
libnxmap_la_SOURCES = graph.cpp main.cpp map.cpp objlist.cpp radial.cpp rt.cpp \
		      submap.cpp vertex.cpp
libnxmap_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxmap_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxmap.dsp libnxmap.dsw \
	libnxmap.h
