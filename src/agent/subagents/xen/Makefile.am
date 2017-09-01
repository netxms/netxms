SUBAGENT = xen

pkglib_LTLIBRARIES = xen.la
xen_la_SOURCES = cpu.cpp domain.cpp host.cpp net.cpp xen.cpp
xen_la_CPPFLAGS=-I@top_srcdir@/include @XEN_CPPFLAGS@
xen_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym @XEN_LDFLAGS@
xen_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la @XEN_LIBS@

EXTRA_DIST = xen.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
