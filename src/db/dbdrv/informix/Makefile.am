INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = informix

pkglib_LTLIBRARIES = informix.la
informix_la_SOURCES = informix.cpp
informix_la_LDFLAGS = -module -avoid-version
informix_la_LIBADD = ../../../libnetxms/libnetxms.la @INFORMIX_LIBS@

EXTRA_DIST = informix.vcproj informixdrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(DRIVER).a $(pkglibdir)/$(DRIVER).so ; rm -f $(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(pkglibdir)/dbdrv
	mv $(pkglibdir)/$(DRIVER).so $(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(pkglibdir)/$(DRIVER).la 
	rm -f $(libdir)/libnxddr_$(DRIVER).so*
	ln -s $(pkglibdir)/dbdrv/$(DRIVER).ddr $(libdir)/libnxddr_$(DRIVER).so
