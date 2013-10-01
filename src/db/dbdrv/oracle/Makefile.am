DRIVER = oracle

pkglib_LTLIBRARIES = oracle.la
oracle_la_SOURCES = oracle.cpp
oracle_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
oracle_la_LDFLAGS = -module -avoid-version
oracle_la_LIBADD = ../../../libnetxms/libnetxms.la @ORACLE_LIBS@

EXTRA_DIST = \
	oracle.vcproj \
	oracledrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/dbdrv
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la	
	rm -f $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so*
	ln -s netxms/dbdrv/$(DRIVER).ddr $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so
