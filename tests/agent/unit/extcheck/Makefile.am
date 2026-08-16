# Copyright (C) 2026 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-unit-extcheck
test_unit_extcheck_SOURCES = main.cpp \
	@top_srcdir@/src/agent/core/extcheck.cpp
test_unit_extcheck_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/tests/include -I@top_srcdir@/build -I@top_srcdir@/src/agent/core
test_unit_extcheck_LDFLAGS = @EXEC_LDFLAGS@
test_unit_extcheck_LDADD = @top_srcdir@/src/agent/libnxagent/libnxagent.la @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@

EXTRA_DIST = test-unit-extcheck.vcxproj test-unit-extcheck.vcxproj.filters
