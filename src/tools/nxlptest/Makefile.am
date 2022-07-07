# Copyright (C) 2014 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = nxlptest
nxlptest_SOURCES = nxlptest.cpp
nxlptest_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxlptest_LDFLAGS = @EXEC_LDFLAGS@
nxlptest_LDADD = ../../libnxlp/libnxlp.la ../../libnetxms/libnetxms.la @EXEC_LIBS@

if USE_INTERNAL_JANSSON
nxlptest_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxlptest_LDADD += -ljansson
endif

EXTRA_DIST = nxlptest.h nxlptest.vcxproj nxlptest.vcxproj.filters
