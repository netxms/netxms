SUBAGENT = entsoe

pkglib_LTLIBRARIES = entsoe.la
entsoe_la_SOURCES = main.cpp adapter.cpp catalog.cpp factors.cpp zone.cpp
entsoe_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
entsoe_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
entsoe_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

EXTRA_DIST = entsoe.h catalog_data.h README.md tools/generate_catalog.py

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
