# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

SUBDIRS = libnxdbmgr nddload nxget nxadm nxaction nxap nxdbmgr nxdownload nxminfo nxwsget nxupload scripts @SERVER_TOOLS@

bin_PROGRAMS = nddload/nddload nxaction/nxaction nxadm/nxadm nxap/nxap nxdownload/nxdownload nxget/nxget \
	       nxminfo/nxminfo nxupload/nxupload nxwsget/nxwsget

COMMON_LDADD = ../libnxsrv/libnxsrv.la ../../libnxsl/libnxsl.la ../../snmp/libnxsnmp/libnxsnmp.la ../../libnetxms/libnetxms.la @EXEC_LIBS@

nddload_nddload_SOURCES = nddload/nddload.cpp
nddload_nddload_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nddload_nddload_LDFLAGS = @EXEC_LDFLAGS@
nddload_nddload_LDADD = $(COMMON_LDADD)

nxaction_nxaction_SOURCES = nxaction/nxaction.cpp
nxaction_nxaction_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxaction_nxaction_LDFLAGS = @EXEC_LDFLAGS@
nxaction_nxaction_LDADD = $(COMMON_LDADD)

nxadm_nxadm_SOURCES = nxadm/comm.cpp nxadm/nxadm.cpp
nxadm_nxadm_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxadm_nxadm_LDFLAGS = @EXEC_LDFLAGS@
nxadm_nxadm_LDADD = $(COMMON_LDADD) @NXADM_LIBS@

nxap_nxap_SOURCES = nxap/nxap.cpp
nxap_nxap_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxap_nxap_LDFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include @EXEC_LDFLAGS@
nxap_nxap_LDADD = $(COMMON_LDADD)

nxdownload_nxdownload_SOURCES = nxdownload/nxdownload.cpp
nxdownload_nxdownload_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxdownload_nxdownload_LDFLAGS = @EXEC_LDFLAGS@
nxdownload_nxdownload_LDADD = $(COMMON_LDADD)

nxget_nxget_SOURCES = nxget/nxget.cpp
nxget_nxget_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxget_nxget_LDFLAGS = @EXEC_LDFLAGS@
nxget_nxget_LDADD = $(COMMON_LDADD)

nxminfo_nxminfo_SOURCES = nxminfo/nxminfo.cpp
nxminfo_nxminfo_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxminfo_nxminfo_LDFLAGS = @EXEC_LDFLAGS@
nxminfo_nxminfo_LDADD = $(COMMON_LDADD)

nxupload_nxupload_SOURCES = nxupload/nxupload.cpp
nxupload_nxupload_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxupload_nxupload_LDFLAGS = @EXEC_LDFLAGS@
nxupload_nxupload_LDADD = $(COMMON_LDADD)

nxwsget_nxwsget_SOURCES = nxwsget/nxwsget.cpp
nxwsget_nxwsget_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxwsget_nxwsget_LDFLAGS = @EXEC_LDFLAGS@
nxwsget_nxwsget_LDADD = $(COMMON_LDADD)
