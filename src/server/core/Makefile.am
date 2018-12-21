# vim:ts=3:sw=3

radiusdir=$(pkgdatadir)
radius_DATA = radius.dict

lib_LTLIBRARIES = libnxcore.la
libnxcore_la_SOURCES = abind_target.cpp accesspoint.cpp acl.cpp actions.cpp addrlist.cpp \
			admin.cpp agent.cpp agent_policy.cpp alarm.cpp alarm_category.cpp audit.cpp \
			ap_jobs.cpp beacon.cpp bizservice.cpp \
			bizsvcroot.cpp bridge.cpp cas_validator.cpp ccy.cpp cdp.cpp \
			cert.cpp chassis.cpp client.cpp cluster.cpp columnfilter.cpp \
			components.cpp condition.cpp config.cpp console.cpp \
			container.cpp correlate.cpp dashboard.cpp datacoll.cpp dbwrite.cpp \
			dc_nxsl.cpp dci_recalc.cpp dcitem.cpp dcithreshold.cpp dcivalue.cpp \
			dcobject.cpp dcowner.cpp dcst.cpp dctable.cpp dctarget.cpp \
			dctcolumn.cpp dctthreshold.cpp debug.cpp devdb.cpp dfile_info.cpp \
			download_job.cpp ef.cpp email.cpp entirenet.cpp \
			epp.cpp events.cpp evproc.cpp fdb.cpp filemonitoring.cpp \
			graph.cpp hdlink.cpp hk.cpp icmpscan.cpp id.cpp \
			import.cpp inaddr_index.cpp index.cpp interface.cpp \
			isc.cpp job.cpp jobmgr.cpp jobqueue.cpp layer2.cpp \
			ldap.cpp lln.cpp lldp.cpp locks.cpp logfilter.cpp \
			loghandle.cpp logs.cpp macdb.cpp main.cpp maint.cpp \
			market.cpp mdconn.cpp mdsession.cpp mobile.cpp \
			modules.cpp mt.cpp ndd.cpp ndp.cpp \
			netinfo.cpp netmap.cpp netmap_element.cpp netmap_link.cpp \
			netmap_objlist.cpp netobj.cpp netsrv.cpp \
			node.cpp nodelink.cpp np.cpp npe.cpp nxsl_classes.cpp \
			nxslext.cpp objects.cpp objtools.cpp package.cpp \
			pds.cpp poll.cpp ps.cpp rack.cpp \
			radius.cpp reporting.cpp rootobj.cpp schedule.cpp script.cpp \
			sensor.cpp session.cpp slmcheck.cpp smclp.cpp \
			sms.cpp snmp.cpp snmptrap.cpp stp.cpp subnet.cpp summary_email.cpp \
			svccontainer.cpp swpkg.cpp syncer.cpp syslogd.cpp \
			template.cpp tools.cpp tracert.cpp tunnel.cpp \
			uniroot.cpp upload_job.cpp uptimecalc.cpp userdb.cpp \
			userdb_objects.cpp vobject.cpp vpnconn.cpp vrrp.cpp watchdog.cpp \
			winperf.cpp xmpp.cpp zeromq.cpp zone.cpp
libnxcore_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -DNXCORE_EXPORTS \
	@MQTT_CPPFLAGS@ @ZEROMQ_CPPFLAGS@ @PYTHON_CPPFLAGS@
libnxcore_la_CXXFLAGS = -I@top_srcdir@/@CUSTOM_INCLUDE@
libnxcore_la_LDFLAGS = -release $(NETXMS_LIBRARY_VERSION) @MQTT_LDFLAGS@ @ZEROMQ_LDFLAGS@ @PYTHON_LDFLAGS@
libnxcore_la_LIBADD = \
	../libnxsrv/libnxsrv.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnxlp/libnxlp.la \
	../../db/libnxdb/libnxdb.la \
	../../libnxsl/libnxsl.la \
	../../agent/libnxagent/libnxagent.la \
	../../libnxjava/libnxjava.la \
	../../libnetxms/libnetxms.la \
	@SERVER_LIBS@
if PYTHON_SUPPORT
libnxcore_la_LIBADD += @top_srcdir@/src/libnxpython/libnxpython.la @PYTHON_LIBS@
endif
if XMPP_SUPPORT
libnxcore_la_LIBADD += @top_srcdir@/src/libstrophe/libstrophe.la
endif
if USE_INTERNAL_EXPAT
libnxcore_la_LIBADD += @top_srcdir@/src/libexpat/libexpat/libnxexpat.la
endif
if USE_INTERNAL_LIBTRE
libnxcore_la_LIBADD += @top_srcdir@/src/libtre/libnxtre.la
endif
if USE_INTERNAL_JANSSON
libnxcore_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
libnxcore_la_LIBADD += -ljansson
endif
if WITH_PRIVATE_EXTENSIONS
libnxcore_la_CXXFLAGS += -I@top_srcdir@/private/netxms/include -I@top_srcdir@/private/netxms/src/server/init -I@top_srcdir@/private/common/src/licensing
endif

EXTRA_DIST = \
	nxcore.vcxproj nxcore.vcxproj.filters \
	nxcore.h radius.h \
	radius.dict
