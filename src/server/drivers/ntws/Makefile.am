DRIVER = ntws

pkglib_LTLIBRARIES = ntws.la
ntws_la_SOURCES = ntws.cpp
ntws_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
ntws_la_LDFLAGS = -module -avoid-version
ntws_la_LIBADD = ../../../libnetxms/libnetxms.la \
	../../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
						 ntws.vcproj \
						 ntws.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
