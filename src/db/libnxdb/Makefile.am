INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxdb.la
libnxdb_la_SOURCES = dbcp.cpp drivers.cpp main.cpp session.cpp
libnxdb_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxdb_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxdb.vcproj libnxdbw.vcproj libnxdb.h
