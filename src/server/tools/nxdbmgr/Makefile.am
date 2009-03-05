INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/src/sqlite

bin_PROGRAMS = nxdbmgr
nxdbmgr_SOURCES = nxdbmgr.cpp check.cpp clear.cpp export.cpp import.cpp \
                  init.cpp reindex.cpp unlock.cpp upgrade.cpp
nxdbmgr_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la \
                ../../../snmp/libnxsnmp/libnxsnmp.la ../../../sqlite/libnxsqlite.la

EXTRA_DIST = \
	nxdbmgr.vcproj nxdbmgr.h
