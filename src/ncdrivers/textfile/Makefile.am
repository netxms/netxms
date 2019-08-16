DRIVER = textfile

pkglib_LTLIBRARIES = textfile.la
textfile_la_SOURCES = text_file.cpp 
textfile_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
textfile_la_LDFLAGS = -module -avoid-version
textfile_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	textfile.vcxproj textfile.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncd/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
