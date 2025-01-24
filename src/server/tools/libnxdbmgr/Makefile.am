lib_LTLIBRARIES = libnxdbmgr.la
libnxdbmgr_la_SOURCES = check.cpp config.cpp confirm.cpp main.cpp query.cpp upgrade.cpp xml_to_json.cpp
libnxdbmgr_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -DLIBNXDBMGR_EXPORTS -I@top_srcdir@/build
libnxdbmgr_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxdbmgr_la_LIBADD = \
	../../libnxsrv/libnxsrv.la \
	../../../db/libnxdb/libnxdb.la \
	../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxdbmgr.h \
	libnxdbmgr.vcxproj libnxdbmgr.vcxproj.filters
