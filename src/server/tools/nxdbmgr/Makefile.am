bin_PROGRAMS = nxdbmgr
nxdbmgr_SOURCES = nxdbmgr.cpp check.cpp clear.cpp export.cpp import.cpp \
                  init.cpp migrate.cpp mm.cpp reindex.cpp resetadmin.cpp \
                  tables.cpp tdata_convert.cpp unlock.cpp upgrade.cpp \
                  upgrade_v0.cpp upgrade_v21.cpp upgrade_v22.cpp
nxdbmgr_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxdbmgr_LDADD = ../../../libnetxms/libnetxms.la \
                ../../../db/libnxdb/libnxdb.la \
                ../../libnxsrv/libnxsrv.la \
                ../../../snmp/libnxsnmp/libnxsnmp.la

if USE_INTERNAL_SQLITE
nxdbmgr_CPPFLAGS += -I@top_srcdir@/src/sqlite
nxdbmgr_LDADD += ../../../sqlite/libnxsqlite.la
else
nxdbmgr_LDADD += -lsqlite3
endif

tables.cpp: @top_srcdir@/sql/schema.in
	@top_srcdir@/tools/create_table_list.pl @top_srcdir@/sql/schema.in > tables.cpp

EXTRA_DIST = \
	nxdbmgr.vcxproj nxdbmgr.vcxproj.filters nxdbmgr.h \
	Makefile.w32
