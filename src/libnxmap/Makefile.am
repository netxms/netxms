INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxmap.la
libnxmap_la_SOURCES = main.cpp map.cpp submap.cpp
libnxmap_la_LDFLAGS = -version-info $(LIBNXMAP_LIBRARY_VERSION) $(PTHREAD_LIBS)
libnxmap_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxmap.dsp libnxmap.dsw \
	libnxmap.h
