SUBAGENT = netconf

pkglib_LTLIBRARIES = netconf.la
netconf_la_SOURCES = handlers.cpp keys.cpp main.cpp session.cpp sp.cpp
netconf_la_CPPFLAGS = @LIBSSH_CPPFLAGS@ -I@top_srcdir@/include -I@top_srcdir@/build
netconf_la_LDFLAGS = @LIBSSH_LDFLAGS@ -module -avoid-version -export-symbols ../subagent.sym
netconf_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnxnetconf/libnxnetconf.la ../../../libnetxms/libnetxms.la -lssh

EXTRA_DIST = \
	Makefile.w32 \
	netconf_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
