INCLUDES=-I@top_srcdir@/include

SUBAGENT = portcheck

pkglib_LTLIBRARIES = portcheck.la
portcheck_la_SOURCES = custom.cpp http.cpp main.cpp net.cpp pop3.cpp smtp.cpp ssh.cpp telnet.cpp
portcheck_la_LDFLAGS = -module -avoid-version
portcheck_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = main.h net.h portcheck.vcproj

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(SUBAGENT).a $(pkglibdir)/$(SUBAGENT).so ; rm -f $(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
