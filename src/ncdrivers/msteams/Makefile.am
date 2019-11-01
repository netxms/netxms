DRIVER = msteams

pkglib_LTLIBRARIES = msteams.la
msteams_la_SOURCES = msteams.cpp
msteams_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
msteams_la_LDFLAGS = -module -avoid-version
msteams_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	msteams.vcxproj msteams.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncdrv/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
