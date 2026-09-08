# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-physical-placement
test_physical_placement_SOURCES = test-physical-placement.cpp ../../src/server/core/physical_placement.cpp
test_physical_placement_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I../include -I@top_srcdir@/build -DNXCORE_EXPORTS @MICROHTTPD_CPPFLAGS@
test_physical_placement_LDFLAGS = @EXEC_LDFLAGS@
test_physical_placement_LDADD = \
	@top_srcdir@/src/server/libnxsrv/libnxsrv.la \
	@top_srcdir@/src/snmp/libnxsnmp/libnxsnmp.la \
	@top_srcdir@/src/libnxsl/libnxsl.la \
	@top_srcdir@/src/db/libnxdb/libnxdb.la \
	@top_srcdir@/src/agent/libnxagent/libnxagent.la \
	@top_srcdir@/src/libnetxms/libnetxms.la \
	@EXEC_LIBS@

if USE_INTERNAL_JANSSON
test_physical_placement_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
test_physical_placement_LDADD += -ljansson
endif

EXTRA_DIST = Makefile.w32
