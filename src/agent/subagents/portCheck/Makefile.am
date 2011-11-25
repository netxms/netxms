INCLUDES=-I@top_srcdir@/include

SUBAGENT = portcheck

pkglib_LTLIBRARIES = portcheck.la
portcheck_la_SOURCES = custom.cpp http.cpp main.cpp net.cpp pop3.cpp smtp.cpp ssh.cpp telnet.cpp
portcheck_la_LDFLAGS = -module -avoid-version
portcheck_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = main.h net.h portcheck.vcproj

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
