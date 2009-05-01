INCLUDES=-I@top_srcdir@/include
SOURCES = sqlite3.c

lib_LTLIBRARIES = libnxsqlite.la
libnxsqlite_la_SOURCES = $(SOURCES)
libnxsqlite_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)

EXTRA_DIST = sqlite3.h sqlite3ext.h dllmain.c nxsqlite.vcproj
