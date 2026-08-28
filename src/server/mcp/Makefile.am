MODULE = mcp

pkglib_LTLIBRARIES = mcp.la
mcp_la_SOURCES = endpoint.cpp main.cpp
mcp_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build @MICROHTTPD_CPPFLAGS@
mcp_la_LDFLAGS = -module -avoid-version @MICROHTTPD_LDFLAGS@
mcp_la_LIBADD = ../../libnetxms/libnetxms.la ../libnxsrv/libnxsrv.la ../core/libnxcore.la @MICROHTTPD_LIBS@
if USE_INTERNAL_JANSSON
mcp_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
mcp_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	mcp.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(MODULE).a $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(MODULE).nxm
	rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).la
