# Copyright (C) 2026 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-nxshell
test_nxshell_SOURCES = test-nxshell.cpp \
	../../src/client/nxshell/parser.cpp
test_nxshell_CPPFLAGS = -I@top_srcdir@/include -I../include -I@top_srcdir@/build \
	-I@top_srcdir@/src/client/nxshell
test_nxshell_LDFLAGS = @EXEC_LDFLAGS@
test_nxshell_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
