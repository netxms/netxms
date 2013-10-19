DRIVER = symbol_ws

pkglib_LTLIBRARIES = symbol_ws.la
symbol_ws_la_SOURCES = symbol-ws.cpp
symbol_ws_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
symbol_ws_la_LDFLAGS = -module -avoid-version
symbol_ws_la_LIBADD = ../../../libnetxms/libnetxms.la \
	../../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
						 symbol-ws.vcproj \
						 symbol-ws.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
