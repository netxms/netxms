DRIVER = informix

pkglib_LTLIBRARIES = informix.la
informix_la_SOURCES = informix.cpp
informix_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
informix_la_LDFLAGS = -module -avoid-version
informix_la_LIBADD = ../../../libnetxms/libnetxms.la @INFORMIX_LIBS@

EXTRA_DIST = informix.vcproj informixdrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/dbdrv
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la	
	rm -f $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so*
	ln -s $(pkglibdir)/dbdrv/$(DRIVER).ddr $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so
