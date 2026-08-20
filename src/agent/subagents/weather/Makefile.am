SUBAGENT = weather

pkglib_LTLIBRARIES = weather.la
weather_la_SOURCES = main.cpp util.cpp httpclient.cpp openmeteo.cpp metno.cpp location.cpp
weather_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
weather_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
weather_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

if USE_INTERNAL_JANSSON
weather_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
weather_la_LIBADD += -ljansson
endif

EXTRA_DIST = weather.h README.md

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
