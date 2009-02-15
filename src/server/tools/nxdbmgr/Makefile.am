INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = nxdbmgr
nxdbmgr_SOURCES = nxdbmgr.cpp check.cpp clear.cpp export.cpp import.cpp \
                  init.cpp reindex.cpp unlock.cpp upgrade.cpp
nxdbmgr_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la \
                ../../../libnxsnmp/libnxsnmp.la

EXTRA_DIST = \
	nxdbmgr.vcproj nxdbmgr.h
