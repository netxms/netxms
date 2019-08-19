# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-libnxdb
test_libnxdb_SOURCES = oracle.cpp test-libnxdb.cpp
test_libnxdb_CPPFLAGS = -I@top_srcdir@/include -I../include -I@top_srcdir@/build
test_libnxdb_LDFLAGS = @EXEC_LDFLAGS@
test_libnxdb_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/db/libnxdb/libnxdb.la @EXEC_LIBS@

EXTRA_DIST = test-libnxdb.vcxproj test-libnxdb.vcxproj.filters
