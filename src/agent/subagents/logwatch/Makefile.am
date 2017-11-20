SUBAGENT = logwatch

pkglib_LTLIBRARIES = logwatch.la
logwatch_la_SOURCES = logwatch.cpp
logwatch_la_CPPFLAGS=-I@top_srcdir@/include
logwatch_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
logwatch_la_LIBADD = ../../libnxagent/libnxagent.la @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/libnxlp/libnxlp.la

EXTRA_DIST = logwatch.h logwatch.vcxproj logwatch.vcxproj.filters

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
