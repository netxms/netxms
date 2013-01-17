DRIVER = db2

pkglib_LTLIBRARIES = db2.la
db2_la_SOURCES = db2.cpp
db2_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
db2_la_LDFLAGS = -module -avoid-version
db2_la_LIBADD = ../../../libnetxms/libnetxms.la @DB2_LIBS@

EXTRA_DIST = db2.vcproj db2drv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/dbdrv
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la	
	rm -f $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so*
	ln -s $(pkglibdir)/dbdrv/$(DRIVER).ddr $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so
