SUBAGENT = mqtt

pkglib_LTLIBRARIES = mqtt.la
mqtt_la_SOURCES = broker.cpp main.cpp topic.cpp
mqtt_la_CPPFLAGS=-I@top_srcdir@/include @MQTT_CPPFLAGS@
mqtt_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym @MQTT_LDFLAGS@
mqtt_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la @MQTT_LIBS@

EXTRA_DIST = Makefile.w32 mqtt_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
