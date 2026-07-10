# Copyright (C) 2026 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-unit-openmeteo
test_unit_openmeteo_SOURCES = main.cpp \
	@top_srcdir@/src/agent/subagents/openmeteo/adapter.cpp
test_unit_openmeteo_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/tests/include -I@top_srcdir@/build -I@top_srcdir@/src/agent/subagents/openmeteo
test_unit_openmeteo_LDFLAGS = @EXEC_LDFLAGS@
test_unit_openmeteo_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@

if USE_INTERNAL_JANSSON
test_unit_openmeteo_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
test_unit_openmeteo_LDADD += -ljansson
endif
