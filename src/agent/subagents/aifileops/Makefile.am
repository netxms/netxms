SUBAGENT = aifileops

pkglib_LTLIBRARIES = aifileops.la
aifileops_la_SOURCES = aifileops.cpp
aifileops_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
aifileops_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
aifileops_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

if USE_INTERNAL_JANSSON
aifileops_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
aifileops_la_LIBADD += -ljansson
endif

EXTRA_DIST = aifileops.h aifileops.vcxproj aifileops.vcxproj.filters

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
