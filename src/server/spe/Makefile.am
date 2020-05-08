MODULE = spe

pkglib_LTLIBRARIES = spe.la
spe_la_SOURCES = main.cpp nn.cpp tsre.cpp
spe_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
spe_la_LDFLAGS = -module -avoid-version
spe_la_LIBADD = ../../libnetxms/libnetxms.la ../libnxsrv/libnxsrv.la ../core/libnxcore.la

EXTRA_DIST = \
	spe.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(MODULE).a $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(MODULE).nxm
	rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).la
