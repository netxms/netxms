MODULE = leef

pkglib_LTLIBRARIES = leef.la
leef_la_SOURCES = main.cpp sender.cpp
leef_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
leef_la_LDFLAGS = -module -avoid-version
leef_la_LIBADD = ../../libnetxms/libnetxms.la ../libnxsrv/libnxsrv.la ../core/libnxcore.la

EXTRA_DIST = \
	leef.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(MODULE).a $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(MODULE).nxm
	rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).la
