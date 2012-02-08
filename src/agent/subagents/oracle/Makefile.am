INCLUDES=-I@top_srcdir@/include

SUBAGENT = oracle

pkglib_LTLIBRARIES = oracle.la
oracle_la_SOURCES = main.cpp
oracle_la_LDFLAGS = -module -avoid-version
oracle_la_LIBADD = ../../../libnetxms/libnetxms.la ../../../db/libnxdb/libnxdb.la

EXTRA_DIST = oracle.vcproj oracle_subagent.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
