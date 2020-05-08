# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = nxpush
nxpush_SOURCES = nxpush.cpp
nxpush_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxpush_LDFLAGS = @EXEC_LDFLAGS@
nxpush_LDADD = ../../libnetxms/libnetxms.la ../libnxclient/libnxclient.la @EXEC_LIBS@

EXTRA_DIST = nxpush.vcxproj nxpush.vcxproj.filters
