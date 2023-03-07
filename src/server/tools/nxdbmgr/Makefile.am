bin_PROGRAMS = nxdbmgr
nxdbmgr_SOURCES = nxdbmgr.cpp check.cpp clear.cpp datacoll.cpp export.cpp \
                  init.cpp migrate.cpp mm.cpp modules.cpp reindex.cpp \
		  resetadmin.cpp tables.cpp tdata_convert.cpp unlock.cpp \
		  upgrade.cpp upgrade_online.cpp upgrade_v0.cpp upgrade_v21.cpp \
                  upgrade_v22.cpp upgrade_v30.cpp upgrade_v31.cpp upgrade_v32.cpp \
		  upgrade_v33.cpp upgrade_v34.cpp upgrade_v35.cpp upgrade_v36.cpp \
		  upgrade_v37.cpp upgrade_v38.cpp upgrade_v39.cpp upgrade_v40.cpp \
		  upgrade_v41.cpp upgrade_v42.cpp upgrade_v43.cpp upgrade_v44.cpp
nxdbmgr_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxdbmgr_LDFLAGS = @EXEC_LDFLAGS@
nxdbmgr_LDADD = \
                ../libnxdbmgr/libnxdbmgr.la \
                ../../libnxsrv/libnxsrv.la \
                ../../../db/libnxdb/libnxdb.la \
                ../../../snmp/libnxsnmp/libnxsnmp.la \
                ../../../libnetxms/libnetxms.la \
		@EXEC_LIBS@

if USE_INTERNAL_SQLITE
nxdbmgr_CPPFLAGS += -I@top_srcdir@/src/sqlite
nxdbmgr_LDADD += ../../../sqlite/libnxsqlite.la
else
nxdbmgr_LDADD += -lsqlite3
endif

tables.cpp: @top_srcdir@/sql/schema.in
	@top_srcdir@/tools/create_table_list.pl @top_srcdir@/sql/schema.in > tables.cpp

EXTRA_DIST = \
	nxdbmgr.vcxproj nxdbmgr.vcxproj.filters nxdbmgr.h
