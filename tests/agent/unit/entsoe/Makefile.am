# Copyright (C) 2026 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-unit-entsoe
test_unit_entsoe_SOURCES = main.cpp \
	@top_srcdir@/src/agent/subagents/entsoe/adapter.cpp \
	@top_srcdir@/src/agent/subagents/entsoe/factors.cpp
test_unit_entsoe_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/tests/include -I@top_srcdir@/build -I@top_srcdir@/src/agent/subagents/entsoe
test_unit_entsoe_LDFLAGS = @EXEC_LDFLAGS@
test_unit_entsoe_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
