# vim:ts=3:sw=3

INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxcore.la
libnxcore_la_SOURCES = acl.cpp agent.cpp agent_policy.cpp actions.cpp \
			admin.cpp alarm.cpp \
			audit.cpp beacon.cpp cert.cpp client.cpp \
			cluster.cpp condition.cpp config.cpp \
			container.cpp correlate.cpp datacoll.cpp \
			dbwrite.cpp dcitem.cpp dcithreshold.cpp dcivalue.cpp \
			debug.cpp ef.cpp email.cpp entirenet.cpp epp.cpp \
			events.cpp evproc.cpp hk.cpp id.cpp \
			interface.cpp isc.cpp job.cpp jobmgr.cpp jobqueue.cpp \
			layer2.cpp locks.cpp \
			main.cpp map.cpp modules.cpp netinfo.cpp netobj.cpp \
			netsrv.cpp node.cpp nortel.cpp np.cpp nxmp_data.cpp \
			lex.nxmp_parser.cpp nxmp_lexer.cpp \
			nxmp_parser.cpp nxmp_parser.tab.cpp \
			nxsl_classes.cpp nxslext.cpp objects.cpp \
			objtools.cpp package.cpp poll.cpp radius.cpp \
			rootobj.cpp script.cpp session.cpp situation.cpp \
			sms.cpp snmp.cpp snmptrap.cpp subnet.cpp syncer.cpp \
			syslogd.cpp template.cpp tools.cpp tracert.cpp \
			uniroot.cpp userdb.cpp userdb_objects.cpp vpnconn.cpp \
			watchdog.cpp webmaps.cpp zone.cpp
libnxcore_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxcore_la_LIBADD = ../../libnetxms/libnetxms.la \
		      ../../snmp/libnxsnmp/libnxsnmp.la \
		      ../../libnxmap/libnxmap.la \
		      ../../libnxlp/libnxlp.la \
		      ../libnxsrv/libnxsrv.la ../../libnxsl/libnxsl.la

EXTRA_DIST = \
	nxcore.vcproj \
	nxcore.h radius.h \
	nxmp_parser.l nxmp_parser.y nxmp_parser.h nxmp_parser.tab.hpp

nxmp_data.cpp: nxmp_parser.tab.cpp

lex.nxmp_parser.cpp: nxmp_parser.l nxmp_parser.tab.cpp nxmp_parser.tab.hpp
	$(LEX) -B -f -8 -olex.nxmp_parser.cpp nxmp_parser.l

nxmp_parser.tab.hpp: nxmp_parser.tab.cpp

nxmp_parser.tab.cpp: nxmp_parser.y
	$(YACC) -b nxmp_parser -o nxmp_parser.tab.cpp -d -t -v nxmp_parser.y
	mv nxmp_parser.tab.cpp nxmp_parser.tab.xx
	sed -e "s/__attribute__ ((__unused__))//" nxmp_parser.tab.xx > nxmp_parser.tab.cpp
	rm nxmp_parser.tab.xx
