INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxcore.la
libnxcore_la_SOURCES = acl.cpp actions.cpp admin.cpp alarm.cpp client.cpp config.cpp container.cpp correlate.cpp datacoll.cpp dbwrite.cpp dcitem.cpp dcithreshold.cpp dcivalue.cpp debug.cpp discovery.cpp email.cpp entirenet.cpp epp.cpp events.cpp evproc.cpp hk.cpp id.cpp image.cpp interface.cpp locks.cpp main.cpp modules.cpp netinfo.cpp netobj.cpp netsrv.cpp node.cpp nortel.cpp np.cpp objects.cpp package.cpp rootobj.cpp session.cpp sms.cpp snmp.cpp snmptrap.cpp status.cpp subnet.cpp syncer.cpp template.cpp tools.cpp tracert.cpp uniroot.cpp users.cpp watchdog.cpp
libnxcore_la_LDFLAGS = -version-info $(LIBNXCORE_LIBRARY_VERSION)

EXTRA_DIST = \
	nxcore.dsp nxcore.dsw \
	nxcore.h
