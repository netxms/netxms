 # vim:ts=3:sw=3

radiusdir=$(pkgdatadir)
radius_DATA = radius.dict

lib_LTLIBRARIES = libnxcore.la
libnxcore_la_SOURCES = abind_target.cpp accesspoint.cpp acl.cpp actions.cpp addrlist.cpp \
			admin.cpp agent.cpp agent_policy.cpp alarm.cpp alarm_category.cpp audit.cpp \
			beacon.cpp bizservice.cpp bizsvcroot.cpp bridge.cpp cas_validator.cpp \
			ccy.cpp cdp.cpp cert.cpp chassis.cpp client.cpp cluster.cpp columnfilter.cpp \
			condition.cpp config.cpp console.cpp container.cpp correlate.cpp \
			dashboard.cpp datacoll.cpp dbwrite.cpp dc_nxsl.cpp dci_recalc.cpp dcitem.cpp \
			dcithreshold.cpp dcivalue.cpp dcobject.cpp dcowner.cpp dcst.cpp dctable.cpp \
			dctarget.cpp dctcolumn.cpp dctthreshold.cpp debug.cpp devdb.cpp \
			dfile_info.cpp download_task.cpp ef.cpp entirenet.cpp epp.cpp events.cpp \
			evproc.cpp fdb.cpp filemonitoring.cpp geo_areas.cpp \
			graph.cpp hash_index.cpp hdlink.cpp hk.cpp hwcomponent.cpp icmpscan.cpp \
			icmpstat.cpp id.cpp import.cpp inaddr_index.cpp index.cpp interface.cpp \
			isc.cpp job.cpp jobmgr.cpp jobqueue.cpp layer2.cpp \
			ldap.cpp lln.cpp lldp.cpp locks.cpp logfilter.cpp \
			loghandle.cpp logs.cpp macdb.cpp main.cpp maint.cpp \
			market.cpp mdconn.cpp mdsession.cpp mobile.cpp \
			modules.cpp mt.cpp ndd.cpp ndp.cpp \
			netinfo.cpp netmap.cpp netmap_element.cpp netmap_link.cpp \
			netmap_objlist.cpp netobj.cpp netsrv.cpp network_cred.cpp \
			node.cpp nodelink.cpp notification_channel.cpp np.cpp npe.cpp nxsl_classes.cpp \
			nxslext.cpp object_categories.cpp objects.cpp objtools.cpp package.cpp \
			pds.cpp physical_link.cpp poll.cpp ps.cpp rack.cpp \
			radius.cpp reporting.cpp rootobj.cpp schedule.cpp script.cpp \
			sensor.cpp server_stats.cpp session.cpp slmcheck.cpp smclp.cpp \
			snmp.cpp snmptrap.cpp sshkeys.cpp stp.cpp subnet.cpp summary_email.cpp \
			svccontainer.cpp swpkg.cpp syncer.cpp syslogd.cpp \
			template.cpp tools.cpp tracert.cpp tunnel.cpp ua_notification_item.cpp \
			uniroot.cpp upload_job.cpp uptimecalc.cpp userdb.cpp \
			userdb_objects.cpp vobject.cpp vpnconn.cpp vrrp.cpp watchdog.cpp \
			websvc.cpp winevent.cpp winperf.cpp xmpp.cpp zeromq.cpp zone.cpp
libnxcore_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build -I@top_srcdir@/src/server/include -DNXCORE_EXPORTS \
	@MQTT_CPPFLAGS@ @ZEROMQ_CPPFLAGS@ @PYTHON_CPPFLAGS@
libnxcore_la_CXXFLAGS = -I@top_srcdir@/@CUSTOM_INCLUDE@
libnxcore_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) @MQTT_LDFLAGS@ @ZEROMQ_LDFLAGS@ @PYTHON_LDFLAGS@
libnxcore_la_LIBADD = \
	../libnxsrv/libnxsrv.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../../ethernetip/libethernetip/libethernetip.la \
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
