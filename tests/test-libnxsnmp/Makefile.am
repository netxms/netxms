# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-libnxsnmp
test_libnxsnmp_SOURCES = test-libnxsnmp.cpp
test_libnxsnmp_CPPFLAGS = -I@top_srcdir@/include -I../include
test_libnxsnmp_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/snmp/libnxsnmp/libnxsnmp.la

if USE_INTERNAL_LIBTRE
test_libnxsnmp_LDADD += @top_srcdir@/src/libtre/libnxtre.la
endif

EXTRA_DIST = test-libnxsnmp.vcxproj test-libnxsnmp.vcxproj.filters
