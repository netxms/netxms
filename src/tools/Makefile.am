# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

SUBDIRS = nxcrashsrv nxcsum nxdevcfg nxencpasswd nxgenguid nxping @TOOLS@ scripts

bin_PROGRAMS = nxcsum/nxcsum nxdevcfg/nxdevcfg nxencpasswd/nxencpasswd nxgenguid/nxgenguid nxping/nxping

nxcsum_nxcsum_SOURCES = nxcsum/nxcsum.cpp
nxcsum_nxcsum_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxcsum_nxcsum_LDFLAGS = @EXEC_LDFLAGS@
nxcsum_nxcsum_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxcsum_nxcsum_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxcsum_nxcsum_LDADD += -ljansson
endif

nxdevcfg_nxdevcfg_SOURCES = nxdevcfg/flags.cpp nxdevcfg/nxdevcfg.cpp
nxdevcfg_nxdevcfg_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxdevcfg_nxdevcfg_LDFLAGS = -I@top_srcdir@/include @EXEC_LDFLAGS@
nxdevcfg_nxdevcfg_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxdevcfg_nxdevcfg_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxdevcfg_nxdevcfg_LDADD += -ljansson
endif

nxencpasswd_nxencpasswd_SOURCES = nxencpasswd/nxencpasswd.cpp
nxencpasswd_nxencpasswd_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxencpasswd_nxencpasswd_LDFLAGS = -I@top_srcdir@/include @EXEC_LDFLAGS@
nxencpasswd_nxencpasswd_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxencpasswd_nxencpasswd_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxencpasswd_nxencpasswd_LDADD += -ljansson
endif

nxgenguid_nxgenguid_SOURCES = nxgenguid/nxgenguid.cpp
nxgenguid_nxgenguid_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxgenguid_nxgenguid_LDFLAGS = @EXEC_LDFLAGS@
nxgenguid_nxgenguid_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxgenguid_nxgenguid_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxgenguid_nxgenguid_LDADD += -ljansson
endif

nxping_nxping_SOURCES = nxping/nxping.cpp
nxping_nxping_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxping_nxping_LDFLAGS = @EXEC_LDFLAGS@
nxping_nxping_LDADD = ../libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxping_nxping_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxping_nxping_LDADD += -ljansson
endif
