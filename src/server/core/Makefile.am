INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

bin_PROGRAMS = netxmsd
netxmsd_SOURCES = messages.c acl.cpp actions.cpp admin.cpp alarm.cpp client.cpp config.cpp container.cpp datacoll.cpp db.cpp dbwrite.cpp dcitem.cpp dcithreshold.cpp debug.cpp discovery.cpp email.cpp entirenet.cpp epp.cpp events.cpp evproc.cpp hk.cpp id.cpp image.cpp interface.cpp locks.cpp log.cpp main.cpp netinfo.cpp netobj.cpp node.cpp np.cpp objects.cpp session.cpp snmp.cpp srvroot.cpp status.cpp subnet.cpp syncer.cpp tools.cpp users.cpp watchdog.cpp winsrv.cpp
netxmsd_LDADD = @PTHREAD_LIBS@ ../../libnetxms/libnetxms.la ../../libnxcscp/libnxcscp.la ../libnxsrv/libnxsrv.la -lsnmp

messages.c: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl $<
