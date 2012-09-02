INCLUDES=-I@top_srcdir@/include

SUBAGENT = freebsd

pkglib_LTLIBRARIES = freebsd.la
freebsd_la_SOURCES = disk.cpp freebsd.cpp net.cpp proc.cpp system.cpp 
freebsd_la_LDFLAGS = -module -avoid-version -lkvm
freebsd_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = disk.h net.h system.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
endif
