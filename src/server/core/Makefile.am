INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = netxmsd
netxmsd_SOURCES = acl.cpp actions.cpp admin.cpp alarm.cpp client.cpp config.cpp container.cpp datacoll.cpp dbwrite.cpp dcitem.cpp dcithreshold.cpp dcivalue.cpp debug.cpp discovery.cpp email.cpp entirenet.cpp epp.cpp events.cpp evproc.cpp hk.cpp id.cpp image.cpp interface.cpp locks.cpp main.cpp netinfo.cpp netobj.cpp node.cpp nortel.cpp np.cpp objects.cpp rootobj.cpp session.cpp snmp.cpp snmptrap.cpp status.cpp subnet.cpp syncer.cpp template.cpp tools.cpp uniroot.cpp users.cpp watchdog.cpp winsrv.cpp
netxmsd_LDADD = @PTHREAD_LIBS@ ../../libnetxms/libnetxms.la ../../libnxcscp/libnxcscp.la ../libnxsrv/libnxsrv.la ../../libnxsnmp/libnxsnmp.la

EXTRA_DIST = \
	netxmsd.dsp netxmsd.dsw \
	nms_actions.h nms_alarm.h nms_core.h \
	nms_dcoll.h nms_events.h nms_locks.h \
	nms_objects.h nms_users.h
