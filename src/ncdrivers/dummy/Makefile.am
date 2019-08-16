DRIVER = dummy

pkglib_LTLIBRARIES = dummy.la
dummy_la_SOURCES = dummy.cpp
dummy_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
dummy_la_LDFLAGS = -module -avoid-version
dummy_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	dummy.vcxproj dummy.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncd/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
