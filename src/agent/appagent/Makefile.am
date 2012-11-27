INCLUDES=-I@top_srcdir@/include
SOURCES = appagent.cpp client.cpp message.cpp

lib_LTLIBRARIES = libappagent.la

libappagent_la_SOURCES = $(SOURCES)
libappagent_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)

EXTRA_DIST = appagent-internal.h appagent.vcproj
