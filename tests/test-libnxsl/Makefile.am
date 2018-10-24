# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

nxsltestdir=$(pkgdatadir)/nxsltest

nxsltest_DATA = \
	arrays.nxsl \
	base64.nxsl \
	control.nxsl \
	globals.nxsl \
	math.nxsl \
	strings.nxsl \
	try-catch.nxsl \
	types.nxsl \
	with.nxsl

bin_PROGRAMS = test-libnxsl
test_libnxsl_SOURCES = test-libnxsl.cpp
test_libnxsl_CPPFLAGS = -I@top_srcdir@/include -I../include
test_libnxsl_LDFLAGS = @EXEC_LDFLAGS@
test_libnxsl_LDADD = @top_srcdir@/src/libnxsl/libnxsl.la @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@

if USE_INTERNAL_LIBTRE
test_libnxsl_LDADD += @top_srcdir@/src/libtre/libnxtre.la
endif

EXTRA_DIST = test-libnxsl.vcxproj test-libnxsl.vcxproj.filters
