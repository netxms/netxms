DRIVER = ers8000

pkglib_LTLIBRARIES = ers8000.la
ers8000_la_SOURCES = ers8000.cpp
ers8000_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
ers8000_la_LDFLAGS = -module -avoid-version
ers8000_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../lib/avaya-ers/libavaya-ers.la

EXTRA_DIST = \
	ers8000.vcproj \
	ers8000.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
