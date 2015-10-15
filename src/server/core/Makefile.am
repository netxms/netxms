# vim:ts=3:sw=3

lib_LTLIBRARIES = libnxcore.la
libnxcore_la_SOURCES =  accesspoint.cpp acl.cpp actions.cpp admin.cpp \
			agent.cpp agent_policy.cpp alarm.cpp audit.cpp \
			ap_config.cpp ap_jobs.cpp beacon.cpp bizservice.cpp \
			bizsvcroot.cpp bridge.cpp cas_validator.cpp cdp.cpp \
			cert.cpp client.cpp cluster.cpp columnfilter.cpp \
			components.cpp condition.cpp config.cpp container.cpp \
			correlate.cpp dashboard.cpp datacoll.cpp dbwrite.cpp \
			dc_nxsl.cpp dcitem.cpp dcithreshold.cpp dcivalue.cpp \
			dcobject.cpp dcst.cpp dctable.cpp dctarget.cpp \
			dctcolumn.cpp dctthreshold.cpp debug.cpp \
			download_job.cpp ef.cpp email.cpp entirenet.cpp \
			epp.cpp events.cpp evproc.cpp fdb.cpp \
			filemonitoring.cpp graph.cpp hdlink.cpp hk.cpp id.cpp \
			import.cpp inaddr_index.cpp index.cpp interface.cpp \
			isc.cpp job.cpp jobmgr.cpp jobqueue.cpp layer2.cpp \
			ldap.cpp lln.cpp lldp.cpp locks.cpp logfilter.cpp \
			loghandle.cpp logs.cpp macdb.cpp main.cpp maint.cpp \
			mdconn.cpp mdsession.cpp mobile.cpp modules.cpp \
			mt.cpp ndd.cpp ndp.cpp \
			netinfo.cpp netmap.cpp netobj.cpp netsrv.cpp \
			node.cpp nodelink.cpp np.cpp nxsl_classes.cpp \
			nxslext.cpp objects.cpp objtools.cpp package.cpp \
			pds.cpp poll.cpp rack.cpp radius.cpp \
			reporting.cpp rootobj.cpp schedule.cpp script.cpp \
			session.cpp situation.cpp slmcheck.cpp smclp.cpp \
			sms.cpp snmp.cpp snmptrap.cpp stp.cpp subnet.cpp \
			svccontainer.cpp swpkg.cpp syncer.cpp syslogd.cpp \
			template.cpp timer.cpp tools.cpp tracert.cpp \
			uniroot.cpp upload_job.cpp uptimecalc.cpp userdb.cpp \
			userdb_objects.cpp vpnconn.cpp vrrp.cpp watchdog.cpp \
			winperf.cpp xmpp.cpp zone.cpp
libnxcore_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxcore_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) 
libnxcore_la_LIBADD = \
	../libnxsrv/libnxsrv.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnxmap/libnxmap.la \
	../../libnxlp/libnxlp.la \
	../../db/libnxdb/libnxdb.la \
	../../libnxsl/libnxsl.la \
	../../agent/libnxagent/libnxagent.la \
	../../libnetxms/libnetxms.la \
	@SERVER_LIBS@
if XMPP_SUPPORT
libnxcore_la_LIBADD += @top_srcdir@/src/libstrophe/libstrophe.la
endif
if USE_INTERNAL_EXPAT
libnxcore_la_LIBADD += @top_srcdir@/src/libexpat/libexpat/libnxexpat.la
endif
if USE_INTERNAL_LIBTRE
libnxcore_la_LIBADD += @top_srcdir@/src/libtre/libnxtre.la
endif

EXTRA_DIST = \
	nxcore.vcproj \
	nxcore.h radius.h
