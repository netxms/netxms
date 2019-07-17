DRIVER = anysms

pkglib_LTLIBRARIES = anysms.la
anysms_la_SOURCES = anysms.cpp
anysms_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
anysms_la_LDFLAGS = -module -avoid-version
anysms_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	anysms.vcxproj anysms.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncd/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
