# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-libnetxms
test_libnetxms_SOURCES = cc.cpp crypto.cpp gauge64.cpp geolocation.cpp mempool.cpp nxcp.cpp test-libnetxms.cpp proc.cpp queue.cpp threads.cpp tp.cpp
test_libnetxms_CPPFLAGS = -I@top_srcdir@/include -I../include -I@top_srcdir@/build
test_libnetxms_LDFLAGS = @EXEC_LDFLAGS@
test_libnetxms_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@

EXTRA_DIST = test-libnetxms.vcxproj test-libnetxms.vcxproj.filters
