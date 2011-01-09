# vim:ts=3:sw=3

INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxcore.la
libnxcore_la_SOURCES = acl.cpp agent.cpp agent_policy.cpp actions.cpp \
			admin.cpp alarm.cpp audit.cpp ap_config.cpp \
			ap_jobs.cpp beacon.cpp bridge.cpp cert.cpp client.cpp \
			cluster.cpp columnfilter.cpp condition.cpp config.cpp \
			container.cpp correlate.cpp datacoll.cpp \
			dbwrite.cpp dcitem.cpp dcithreshold.cpp dcivalue.cpp \
			debug.cpp ef.cpp email.cpp entirenet.cpp epp.cpp \
			events.cpp evproc.cpp fdb.cpp hk.cpp id.cpp import.cpp \
			interface.cpp isc.cpp job.cpp jobmgr.cpp jobqueue.cpp \
			layer2.cpp lln.cpp lldp.cpp locks.cpp logfilter.cpp \
			loghandle.cpp logs.cpp main.cpp map.cpp modules.cpp \
			ndp.cpp netinfo.cpp netobj.cpp netsrv.cpp node.cpp \
			nortel.cpp np.cpp nxsl_classes.cpp nxslext.cpp \
			objects.cpp objtools.cpp package.cpp poll.cpp \
			radius.cpp rootobj.cpp script.cpp session.cpp \
			situation.cpp sms.cpp snmp.cpp snmptrap.cpp \
			subnet.cpp syncer.cpp \
			syslogd.cpp template.cpp tools.cpp tracert.cpp \
			uniroot.cpp userdb.cpp userdb_objects.cpp vpnconn.cpp \
			watchdog.cpp webmaps.cpp zone.cpp netmap.cpp
libnxcore_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxcore_la_LIBADD = ../../libnetxms/libnetxms.la \
		      ../../snmp/libnxsnmp/libnxsnmp.la \
		      ../../libnxmap/libnxmap.la \
		      ../../libnxlp/libnxlp.la \
		      ../../db/libnxdb/libnxdb.la \
		      ../libnxsrv/libnxsrv.la \
                      ../../libnxsl/libnxsl.la

EXTRA_DIST = \
	nxcore.vcproj \
	nxcore.h radius.h
