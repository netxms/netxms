# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-ncd-webhook
test_ncd_webhook_SOURCES = test-ncd-webhook.cpp \
	../../src/server/ncdrivers/webhook/webhook_helpers.cpp
test_ncd_webhook_CPPFLAGS = -I@top_srcdir@/include -I../include -I@top_srcdir@/build \
	-I@top_srcdir@/src/server/ncdrivers/webhook
test_ncd_webhook_LDFLAGS = @EXEC_LDFLAGS@
test_ncd_webhook_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@

if USE_INTERNAL_JANSSON
test_ncd_webhook_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
test_ncd_webhook_LDADD += -ljansson
endif

EXTRA_DIST = test-ncd-webhook.vcxproj test-ncd-webhook.vcxproj.filters
