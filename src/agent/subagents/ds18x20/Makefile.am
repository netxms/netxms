SUBAGENT = ds18x20

pkglib_LTLIBRARIES = ds18x20.la
ds18x20_la_SOURCES = ds18x20.cpp
ds18x20_la_CPPFLAGS=-I@top_srcdir@/include
ds18x20_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
ds18x20_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

EXTRA_DIST = ds18x20.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
