DRIVER = odbc

pkglib_LTLIBRARIES = odbc.la
odbc_la_SOURCES = odbc.cpp
odbc_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include @ODBC_CPPFLAGS@
odbc_la_LDFLAGS = -module -avoid-version @ODBC_LDFLAGS@
odbc_la_LIBADD = ../../../libnetxms/libnetxms.la @ODBC_LIBS@

EXTRA_DIST = Makefile.w32 odbc.vcxproj odbc.vcxproj.filters odbcdrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/dbdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la	
	rm -f $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so*
	ln -s netxms/dbdrv/$(DRIVER).ddr $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so
