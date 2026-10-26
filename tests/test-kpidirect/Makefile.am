# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-kpidirect
test_kpidirect_SOURCES = test-kpidirect.cpp \
	../../src/server/kpidirect/integrate.cpp \
	../../src/server/kpidirect/compose.cpp
test_kpidirect_CPPFLAGS = -I@top_srcdir@/include -I../include -I@top_srcdir@/build \
	-I@top_srcdir@/src/server/kpidirect
test_kpidirect_LDFLAGS = @EXEC_LDFLAGS@
test_kpidirect_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@

if USE_INTERNAL_JANSSON
test_kpidirect_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
test_kpidirect_LDADD += -ljansson
endif

EXTRA_DIST = \
	kpidirect_vectors.json \
	kpidirect_ref.py \
	make_vectors.py
