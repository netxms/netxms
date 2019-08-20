DRIVER = smseagle

pkglib_LTLIBRARIES = smseagle.la
smseagle_la_SOURCES = smseagle.cpp
smseagle_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
smseagle_la_LDFLAGS = -module -avoid-version
smseagle_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	smseagle.vcxproj smseagle.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncdrv/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
