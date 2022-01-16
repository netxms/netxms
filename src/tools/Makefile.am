# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

SUBDIRS = nxcrashsrv nxcsum nxdevcfg nxencpasswd nxgenguid @TOOLS@ scripts

bin_PROGRAMS = nxcsum/nxcsum nxdevcfg/nxdevcfg nxencpasswd/nxencpasswd nxgenguid/nxgenguid

nxcsum_nxcsum_SOURCES = nxcsum/nxcsum.cpp
nxcsum_nxcsum_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxcsum_nxcsum_LDFLAGS = @EXEC_LDFLAGS@
nxcsum_nxcsum_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@

nxdevcfg_nxdevcfg_SOURCES = nxdevcfg/flags.cpp nxdevcfg/nxdevcfg.cpp
nxdevcfg_nxdevcfg_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxdevcfg_nxdevcfg_LDFLAGS = -I@top_srcdir@/include @EXEC_LDFLAGS@
nxdevcfg_nxdevcfg_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@

nxencpasswd_nxencpasswd_SOURCES = nxencpasswd/nxencpasswd.cpp
nxencpasswd_nxencpasswd_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxencpasswd_nxencpasswd_LDFLAGS = -I@top_srcdir@/include @EXEC_LDFLAGS@
nxencpasswd_nxencpasswd_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@

nxgenguid_nxgenguid_SOURCES = nxgenguid/nxgenguid.cpp
nxgenguid_nxgenguid_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxgenguid_nxgenguid_LDFLAGS = @EXEC_LDFLAGS@
nxgenguid_nxgenguid_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@
