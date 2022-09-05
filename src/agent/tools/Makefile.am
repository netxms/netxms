# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

SUBDIRS = nxaevent nxappget nxapush nxhwid nxreload nxtftp scripts

bin_PROGRAMS = nxaevent/nxaevent nxappget/nxappget nxapush/nxapush nxhwid/nxhwid nxtftp/nxtftp

nxaevent_nxaevent_SOURCES = nxaevent/nxaevent.cpp
nxaevent_nxaevent_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxaevent_nxaevent_LDFLAGS = @EXEC_LDFLAGS@
nxaevent_nxaevent_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxaevent_nxaevent_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxaevent_nxaevent_LDADD += -ljansson
endif
if STATIC_BUILD
if ALL_STATIC
nxaevent_nxaevent_LDFLAGS += -all-static
else
nxaevent_nxaevent_LDFLAGS += -static
endif
endif

nxappget_nxappget_SOURCES = nxappget/nxappget.cpp
nxappget_nxappget_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxappget_nxappget_LDFLAGS = @EXEC_LDFLAGS@
nxappget_nxappget_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/appagent/libappagent.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxappget_nxappget_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxappget_nxappget_LDADD += -ljansson
endif
if STATIC_BUILD
if ALL_STATIC
nxappget_nxappget_LDFLAGS += -all-static
else
nxappget_nxappget_LDFLAGS += -static
endif
endif

nxapush_nxapush_SOURCES = nxapush/nxapush.cpp
nxapush_nxapush_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxapush_nxapush_LDFLAGS = @EXEC_LDFLAGS@
nxapush_nxapush_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxapush_nxapush_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxapush_nxapush_LDADD += -ljansson
endif
if STATIC_BUILD
if ALL_STATIC
nxapush_nxapush_LDFLAGS += -all-static
else
nxapush_nxapush_LDFLAGS += -static
endif
endif

nxhwid_nxhwid_SOURCES = nxhwid/nxhwid.cpp
nxhwid_nxhwid_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxhwid_nxhwid_LDFLAGS = @EXEC_LDFLAGS@
nxhwid_nxhwid_LDADD = @top_srcdir@/src/agent/libnxagent/libnxagent.la @top_srcdir@/src/db/libnxdb/libnxdb.la @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxhwid_nxhwid_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxhwid_nxhwid_LDADD += -ljansson
endif
if STATIC_BUILD
if ALL_STATIC
nxhwid_nxhwid_LDFLAGS += -all-static
else
nxhwid_nxhwid_LDFLAGS += -static
endif
endif

nxtftp_nxtftp_SOURCES = nxtftp/nxtftp.cpp
nxtftp_nxtftp_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxtftp_nxtftp_LDFLAGS = @EXEC_LDFLAGS@
nxtftp_nxtftp_LDADD = @top_srcdir@/src/agent/libnxagent/libnxagent.la @top_srcdir@/src/db/libnxdb/libnxdb.la @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
if USE_INTERNAL_JANSSON
nxtftp_nxtftp_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxtftp_nxtftp_LDADD += -ljansson
endif
if STATIC_BUILD
if ALL_STATIC
nxtftp_nxtftp_LDFLAGS += -all-static
else
nxtftp_nxtftp_LDFLAGS += -static
endif
endif
