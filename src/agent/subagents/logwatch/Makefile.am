INCLUDES=-I@top_srcdir@/include

SUBAGENT = logwatch

pkglib_LTLIBRARIES = logwatch.la
logwatch_la_SOURCES = main.cpp
logwatch_la_LDFLAGS = -module -avoid-version
logwatch_la_LIBADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/libnxlp/libnxlp.la

EXTRA_DIST = logwatch.h logwatch.vcproj \
             eventlog.cpp wevt.cpp

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
