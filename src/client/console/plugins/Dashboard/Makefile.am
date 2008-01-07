INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/client/console/include

SUBDIRS = rc

lib_LTLIBRARIES = libnxmcp_Dashboard.la
libnxmcp_Dashboard_la_SOURCES = alarm_overview.cpp dashboard.cpp main.cpp
libnxmcp_Dashboard_la_LDFLAGS = ../../../../libnetxms/libnetxms.la ../../../../libnxcl/libnxcl.la ../../libnxmc/libnxmc.la

EXTRA_DIST = dashboard.h mcpDashboard.dsp Dashboard.rc resource.h

install-exec-hook:
	if test ! -e $(libdir)/nxmc ; then \
		mkdir $(libdir)/nxmc ; \
	fi ; \
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/nxmc/libnxmcp_Dashboard.so.0 ; \
		$(AR) x $(libdir)/libnxmcp_Dashboard.a $(libdir)/nxmc/libnxmcp_Dashboard.so.0 ; \
		mv $(libdir)/nxmc/libnxmcp_Dashboard.so.0 $(libdir)/nxmc/libnxmcp_Dashboard.so ; \
	else \
		mv $(libdir)/libnxmcp_Dashboard.* $(libdir)/nxmc/ ; \
	fi
