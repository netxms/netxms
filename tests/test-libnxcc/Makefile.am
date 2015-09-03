# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-libnxcc
test_libnxcc_SOURCES = test-libnxcc.cpp
test_libnxcc_CPPFLAGS = -I@top_srcdir@/include -I../include
test_libnxcc_LDADD = @top_srcdir@/src/libnxcc/libnxcc.la @top_srcdir@/src/libnetxms/libnetxms.la

EXTRA_DIST = test-libnxcc.vcproj
