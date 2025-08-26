SUBAGENT = redis

pkglib_LTLIBRARIES = redis.la
redis_la_SOURCES = db.cpp main.cpp
redis_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @REDIS_CPPFLAGS@
redis_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym @REDIS_LDFLAGS@
redis_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la @REDIS_LIBS@

EXTRA_DIST = redis_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif