DRIVER = pgsql

pkglib_LTLIBRARIES = pgsql.la
pgsql_la_SOURCES = pgsql.cpp
pgsql_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include @PGSQL_CPPFLAGS@
pgsql_la_LDFLAGS = -module -avoid-version @PGSQL_LDFLAGS@
pgsql_la_LIBADD = ../../../libnetxms/libnetxms.la @PGSQL_LIBS@

EXTRA_DIST = \
	pgsql.vcproj \
	pgsqldrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/dbdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la	
	rm -f $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so*
	ln -s netxms/dbdrv/$(DRIVER).ddr $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so
