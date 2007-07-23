INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/client/console/include

SUBDIRS = rc

lib_LTLIBRARIES = libnxmcp_AlarmBrowser.la
libnxmcp_AlarmBrowser_la_SOURCES = browser.cpp main.cpp view.cpp
libnxmcp_AlarmBrowser_la_LDFLAGS = ../../../../libnetxms/libnetxms.la ../../../../libnxcl/libnxcl.la ../../libnxmc/libnxmc.la

EXTRA_DIST = alarm_browser.h mcpAlarmBrowser.dsp AlarmBrowser.rc resource.h

install-exec-hook:
	if test ! -e $(libdir)/nxmc ; then \
		mkdir $(libdir)/nxmc ; \
	fi ; \
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/nxmc/libnxmcp_AlarmBrowser.so.0 ; \
		$(AR) x $(libdir)/libnxmcp_AlarmBrowser.a $(libdir)/nxmc/libnxmcp_AlarmBrowser.so.0 ; \
		mv $(libdir)/nxmc/libnxmcp_AlarmBrowser.so.0 $(libdir)/nxmc/libnxmcp_AlarmBrowser.so ; \
	else \
		mv $(libdir)/libnxmcp_AlarmBrowser.* $(libdir)/nxmc/ ; \
	fi
