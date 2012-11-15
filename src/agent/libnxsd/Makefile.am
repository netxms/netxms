INCLUDES=-I@top_srcdir@/include
SOURCES = main.cpp attr.cpp

lib_LTLIBRARIES = libnxsd.la

libnxsd_la_SOURCES = $(SOURCES)
libnxsd_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)

EXTRA_DIST = libnxsd.h libnxsd.vcproj
