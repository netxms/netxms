SOURCES = sqlite3.c

lib_LTLIBRARIES = libnxsqlite.la
libnxsqlite_la_SOURCES = $(SOURCES)
libnxsqlite_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
libnxsqlite_la_LDFLAGS = -version-info 3:22:0

EXTRA_DIST = sqlite3.h sqlite3ext.h dllmain.c nxsqlite.vcxproj nxsqlite.vcxproj.filters
