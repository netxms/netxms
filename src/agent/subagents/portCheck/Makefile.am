SUBAGENT = portcheck

pkglib_LTLIBRARIES = portcheck.la
portcheck_la_SOURCES = custom.cpp http.cpp main.cpp net.cpp pop3.cpp smtp.cpp ssh.cpp telnet.cpp
portcheck_la_CPPFLAGS=-I@top_srcdir@/include
portcheck_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
portcheck_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la
if USE_INTERNAL_LIBTRE
portcheck_la_LIBADD += @top_srcdir@/src/libtre/libnxtre.la
endif

EXTRA_DIST = main.h net.h portcheck.vcxproj portcheck.vcxproj.filters

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
