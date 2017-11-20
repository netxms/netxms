lib_LTLIBRARIES = libnxdb.la
libnxdb_la_SOURCES = dbcp.cpp drivers.cpp main.cpp session.cpp util.cpp
libnxdb_la_CPPFLAGS=-I@top_srcdir@/include
libnxdb_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxdb_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	Makefile.w32 \
	libnxdb.vcxproj libnxdb.vcxproj.filters libnxdb.h
