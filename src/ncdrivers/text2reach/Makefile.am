DRIVER = text2reach

pkglib_LTLIBRARIES = text2reach.la
text2reach_la_SOURCES = text2reach.cpp
text2reach_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
text2reach_la_LDFLAGS = -module -avoid-version
text2reach_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	text2reach.vcxproj text2reach.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncdrv/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
