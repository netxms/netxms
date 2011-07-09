INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = odbc

pkglib_LTLIBRARIES = odbc.la
odbc_la_SOURCES = odbc.cpp
odbc_la_LDFLAGS = -module -avoid-version
odbc_la_LIBADD = ../../../libnetxms/libnetxms.la @ODBC_LIBS@

EXTRA_DIST = odbc.vcproj odbcdrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(DRIVER).a $(pkglibdir)/$(DRIVER).so ; rm -f $(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(pkglibdir)/dbdrv
	mv $(pkglibdir)/$(DRIVER).so $(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(pkglibdir)/$(DRIVER).la 
	rm -f $(libdir)/libnxddr_$(DRIVER).so*
	ln -s $(pkglibdir)/dbdrv/$(DRIVER).ddr $(libdir)/libnxddr_$(DRIVER).so
