INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = netscreen

pkglib_LTLIBRARIES = netscreen.la
netscreen_la_SOURCES = netscreen.cpp
netscreen_la_LDFLAGS = -module -avoid-version
netscreen_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	netscreen.vcproj \
	netscreen.h

install-exec-hook:
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
