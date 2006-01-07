INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxcore.la
libnxcore_la_SOURCES = acl.cpp actions.cpp admin.cpp alarm.cpp client.cpp config.cpp container.cpp correlate.cpp datacoll.cpp dbwrite.cpp dcitem.cpp dcithreshold.cpp dcivalue.cpp debug.cpp email.cpp entirenet.cpp epp.cpp events.cpp evproc.cpp hk.cpp id.cpp image.cpp interface.cpp locks.cpp lpp.cpp main.cpp modules.cpp netinfo.cpp netobj.cpp netsrv.cpp node.cpp nortel.cpp np.cpp objects.cpp objtools.cpp package.cpp poll.cpp rootobj.cpp session.cpp script.cpp sms.cpp snmp.cpp snmptrap.cpp subnet.cpp syncer.cpp syslogd.cpp template.cpp tools.cpp tracert.cpp uniroot.cpp users.cpp vpnconn.cpp watchdog.cpp zone.cpp
libnxcore_la_LDFLAGS = -version-info $(LIBNXCORE_LIBRARY_VERSION)

EXTRA_DIST = \
	nxcore.dsp nxcore.dsw \
	nxcore.h
