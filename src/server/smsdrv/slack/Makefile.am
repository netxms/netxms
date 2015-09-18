lib_LTLIBRARIES = libnxsms_slack.la
libnxsms_slack_la_SOURCES = main.cpp
libnxsms_slack_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsms_slack_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsms_slack_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la -lcurl
if USE_INTERNAL_JANSSON
libnxsms_slack_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
libnxsms_slack_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	slack.vcproj
