DRIVER = hpe

pkglib_LTLIBRARIES = hpe.la
hpe_la_SOURCES = h3c.cpp hpe.cpp hpsw.cpp procurve.cpp
hpe_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
hpe_la_LDFLAGS = -module -avoid-version
hpe_la_LIBADD = ../../../libnetxms/libnetxms.la \
		      ../../../snmp/libnxsnmp/libnxsnmp.la \
		      ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	hpe.vcxproj hpe.vcxproj.filters \
	hpe.h \
	Makefile.w32

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
