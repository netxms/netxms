MODULE = webapi

pkglib_LTLIBRARIES = webapi.la
webapi_la_SOURCES = 2fa.cpp actions.cpp ai.cpp alarm_categories.cpp alarms.cpp conn_history.cpp datacoll.cpp dcst.cpp epp.cpp events.cpp find.cpp geoareas.cpp grafana.cpp images.cpp info.cpp main.cpp notifications.cpp object_categories.cpp object_collections.cpp object_helpers.cpp object_properties.cpp objects.cpp objtools.cpp schedules.cpp scripts.cpp snmp_mib.cpp sshkeys.cpp tcpproxy.cpp topology.cpp users.cpp websvc.cpp
webapi_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build @MICROHTTPD_CPPFLAGS@
webapi_la_LDFLAGS = -module -avoid-version @MICROHTTPD_LDFLAGS@
webapi_la_LIBADD = ../../libnetxms/libnetxms.la ../libnxsrv/libnxsrv.la ../core/libnxcore.la @MICROHTTPD_LIBS@

EXTRA_DIST = \
	object_helpers.h \
	webapi.h \
	webapi.vcxproj webapi.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(MODULE).a $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(MODULE).nxm
	rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).la
