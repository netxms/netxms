SOURCES = dispatcher.cpp filter.cpp main.cpp message.cpp subscriber.cpp

lib_LTLIBRARIES = libnxmb.la

libnxmb_la_SOURCES = $(SOURCES)
libnxmb_la_CPPFLAGS=-I@top_srcdir@/include
libnxmb_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxmb_la_LIBADD = @top_srcdir@/src/libnetxms/libnetxms.la


EXTRA_DIST = \
	libnxmb.vcproj \
	libnxmb.h
