DRIVER = mariadb

pkglib_LTLIBRARIES = mariadb.la
mariadb_la_SOURCES = mariadb.cpp
mariadb_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include @MARIADB_CPPFLAGS@
mariadb_la_LDFLAGS = -module -avoid-version @MARIADB_LDFLAGS@
mariadb_la_LIBADD = ../../../libnetxms/libnetxms.la @MARIADB_LIBS@

EXTRA_DIST = \
	mariadb.vcxproj mariadb.vcxproj.filters \
	mariadbdrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/dbdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la	
	rm -f $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so*
	ln -s netxms/dbdrv/$(DRIVER).ddr $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so
