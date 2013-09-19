bin_PROGRAMS = nxdbmgr
nxdbmgr_SOURCES = nxdbmgr.cpp check.cpp clear.cpp export.cpp import.cpp \
                  init.cpp mm.cpp reindex.cpp tables.cpp unlock.cpp upgrade.cpp \
						resetadmin.cpp
nxdbmgr_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/src/sqlite
nxdbmgr_LDADD = ../../../libnetxms/libnetxms.la \
                ../../../db/libnxdb/libnxdb.la \
                ../../../libnxmap/libnxmap.la \
                ../../libnxsrv/libnxsrv.la \
                ../../../snmp/libnxsnmp/libnxsnmp.la \
                ../../../sqlite/libnxsqlite.la

tables.cpp: @top_srcdir@/sql/schema.in
	@top_srcdir@/tools/create_table_list.pl @top_srcdir@/sql/schema.in > tables.cpp

EXTRA_DIST = \
	nxdbmgr.vcproj nxdbmgr.h
