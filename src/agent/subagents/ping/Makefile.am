SUBAGENT = ping

pkglib_LTLIBRARIES = ping.la
ping_la_SOURCES = ping.cpp scan_unix.cpp
ping_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
ping_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
ping_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

EXTRA_DIST = ping.h ping.vcxproj ping.vcxproj.filters scan_win32.cpp

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
