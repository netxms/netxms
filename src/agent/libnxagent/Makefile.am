SOURCES = bridge.cpp main.cpp registry.cpp tools.cpp

lib_LTLIBRARIES = libnxagent.la

libnxagent_la_SOURCES = $(SOURCES)
libnxagent_la_CPPFLAGS=-I@top_srcdir@/include
libnxagent_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxagent_la_LIBADD = ../../db/libnxdb/libnxdb.la ../../libnetxms/libnetxms.la

EXTRA_DIST = libnxagent.h libnxagent.vcproj
