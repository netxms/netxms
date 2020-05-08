lib_LTLIBRARIES = libnxdb.la
libnxdb_la_SOURCES = cache.cpp dbcp.cpp drivers.cpp main.cpp session.cpp util.cpp
libnxdb_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXDB_EXPORTS -I@top_srcdir@/build
libnxdb_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxdb_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxdb.vcxproj libnxdb.vcxproj.filters libnxdb.h
