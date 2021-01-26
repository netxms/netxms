SUBAGENT = ssh

pkglib_LTLIBRARIES = ssh.la
ssh_la_SOURCES = handlers.cpp key_list.cpp main.cpp session.cpp sp.cpp
ssh_la_CPPFLAGS = @LIBSSH_CPPFLAGS@ -I@top_srcdir@/include -I@top_srcdir@/build
ssh_la_LDFLAGS = @LIBSSH_LDFLAGS@ -module -avoid-version -export-symbols ../subagent.sym
ssh_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la -lssh

EXTRA_DIST = ssh_subagent.h ssh.vcxproj ssh.vcxproj.filters

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
