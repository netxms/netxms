SUBAGENT = openmeteo

pkglib_LTLIBRARIES = openmeteo.la
openmeteo_la_SOURCES = main.cpp adapter.cpp location.cpp
openmeteo_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
openmeteo_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
openmeteo_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

if USE_INTERNAL_JANSSON
openmeteo_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
openmeteo_la_LIBADD += -ljansson
endif

EXTRA_DIST = openmeteo.h README.md

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
