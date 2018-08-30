lib_LTLIBRARIES = libnxdbmgr.la
libnxdbmgr_la_SOURCES = check.cpp config.cpp confirm.cpp main.cpp query.cpp upgrade.cpp
libnxdbmgr_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -DLIBNXDBMGR_EXPORTS
libnxdbmgr_la_LDFLAGS = -release $(NETXMS_LIBRARY_VERSION)
libnxdbmgr_la_LIBADD = \
	../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxdbmgr.vcxproj libnxdbmgr.vcxproj.filters
	libnxdbmgr.h resource.h
