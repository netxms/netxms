MODULE = ntcb

pkglib_LTLIBRARIES = ntcb.la
ntcb_la_SOURCES = main.cpp session.cpp telemetry.cpp
ntcb_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
ntcb_la_LDFLAGS = -module -avoid-version
ntcb_la_LIBADD = ../../libnetxms/libnetxms.la ../libnxsrv/libnxsrv.la ../core/libnxcore.la

EXTRA_DIST = \
	ntcb.h \
	ntcb.vcxproj ntcb.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(MODULE).a $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(MODULE).nxm
	rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).la
