INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/client/console/include

lib_LTLIBRARIES = libnxmcp_ObjectBrowser.la
libnxmcp_ObjectBrowser_la_SOURCES = browser.cpp main.cpp objview.cpp
libnxmcp_ObjectBrowser_la_LDFLAGS = ../../../../libnetxms/libnetxms.la ../../../../libnxcl/libnxcl.la ../../libnxmc/libnxmc.la

EXTRA_DIST = object_browser.h mcpObjectBrowser.dsp

install-exec-hook:
	if test ! -e $(libdir)/nxmc ; then \
		mkdir $(libdir)/nxmc ; \
	fi ; \
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/nxmc/libnxmcp_ObjectBrowser.so.0 ; \
		$(AR) x $(libdir)/libnxmcp_ObjectBrowser.a $(libdir)/nxmc/libnxmcp_ObjectBrowser.so.0 ; \
		mv $(libdir)/nxmc/libnxmcp_ObjectBrowser.so.0 $(libdir)/nxmc/libnxmcp_ObjectBrowser.so ; \
	else \
		mv $(libdir)/libnxmcp_ObjectBrowser.* $(libdir)/nxmc/ ; \
	fi
