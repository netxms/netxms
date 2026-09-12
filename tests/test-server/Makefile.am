# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-server
test_server_SOURCES = authtokens.cpp nxsl_http.cpp test-server.cpp
test_server_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I../include -I@top_srcdir@/build @MICROHTTPD_CPPFLAGS@
test_server_LDFLAGS = @EXEC_LDFLAGS@ @MICROHTTPD_LDFLAGS@
test_server_LDADD = \
	@top_srcdir@/src/server/core/libnxcore.la \
	@top_srcdir@/src/server/libnxsrv/libnxsrv.la \
	@top_srcdir@/src/snmp/libnxsnmp/libnxsnmp.la \
	@top_srcdir@/src/ethernetip/libethernetip/libethernetip.la \
	@top_srcdir@/src/libnxsl/libnxsl.la \
	@top_srcdir@/src/libnxlp/libnxlp.la \
	@top_srcdir@/src/db/libnxdb/libnxdb.la \
	@top_srcdir@/src/agent/libnxagent/libnxagent.la \
	@top_srcdir@/src/libnetxms/libnetxms.la \
	@MICROHTTPD_LIBS@ @SERVER_LIBS@ @EXEC_LIBS@

if USE_INTERNAL_JANSSON
test_server_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
test_server_LDADD += -ljansson
endif

EXTRA_DIST = Makefile.w32
