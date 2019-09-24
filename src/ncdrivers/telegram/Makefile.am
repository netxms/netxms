DRIVER = telegram

pkglib_LTLIBRARIES = telegram.la
telegram_la_SOURCES = telegram.cpp
telegram_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
telegram_la_LDFLAGS = -module -avoid-version
telegram_la_LIBADD = ../../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
telegram_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
telegram_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	telegram.vcxproj telegram.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncdrv/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
