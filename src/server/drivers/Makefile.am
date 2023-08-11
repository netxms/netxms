# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

DRIVERS = at avaya cambium cisco dell-pwc dlink extreme hirschmann \
	hpe huawei ignitenet juniper mikrotik moxa net-snmp netonix \
	ping3 qtech-olt rittal saf symbol-ws tb ubnt westerstrand

SUBDIRS = $(DRIVERS)

pkglib_LTLIBRARIES = at.la avaya.la cambium.la cisco.la dell-pwc.la dlink.la extreme.la \
	hirschmann.la hpe.la huawei.la ignitenet.la juniper.la mikrotik.la moxa.la \
	net-snmp.la netonix.la ping3.la qtech-olt.la rittal.la saf.la symbol-ws.la \
	tb.la ubnt.la westerstrand.la

at_la_SOURCES = at/at.cpp
at_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
at_la_LDFLAGS = -module -avoid-version
at_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

avaya_la_SOURCES = avaya/avaya.cpp avaya/baystack.cpp avaya/ers8000.cpp avaya/ntws.cpp avaya/vlan-if.cpp
avaya_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
avaya_la_LDFLAGS = -module -avoid-version
avaya_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

cambium_la_SOURCES = cambium/cambium.cpp cambium/cnpilot.cpp cambium/epmp.cpp
cambium_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
cambium_la_LDFLAGS = -module -avoid-version
cambium_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

cisco_la_SOURCES = cisco/cat2900xl.cpp cisco/catalyst.cpp cisco/cisco.cpp cisco/esw.cpp cisco/generic.cpp cisco/nexus.cpp cisco/sb.cpp cisco/wlc.cpp
cisco_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
cisco_la_LDFLAGS = -module -avoid-version
cisco_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

dell_pwc_la_SOURCES = dell-pwc/dell-pwc.cpp
dell_pwc_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
dell_pwc_la_LDFLAGS = -module -avoid-version
dell_pwc_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

dlink_la_SOURCES = dlink/dlink.cpp
dlink_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
dlink_la_LDFLAGS = -module -avoid-version
dlink_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

extreme_la_SOURCES = extreme/extreme.cpp
extreme_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
extreme_la_LDFLAGS = -module -avoid-version
extreme_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

hirschmann_la_SOURCES = hirschmann/classic.cpp hirschmann/hios.cpp hirschmann/hirschmann.cpp
hirschmann_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
hirschmann_la_LDFLAGS = -module -avoid-version
hirschmann_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

hpe_la_SOURCES = hpe/h3c.cpp hpe/hpe.cpp hpe/hpsw.cpp hpe/ilo.cpp hpe/procurve.cpp
hpe_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
hpe_la_LDFLAGS = -module -avoid-version
hpe_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

huawei_la_SOURCES = huawei/huawei.cpp huawei/optix.cpp
huawei_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
huawei_la_LDFLAGS = -module -avoid-version
huawei_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

ignitenet_la_SOURCES = ignitenet/ignitenet.cpp
ignitenet_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
ignitenet_la_LDFLAGS = -module -avoid-version
ignitenet_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

juniper_la_SOURCES = juniper/juniper.cpp juniper/netscreen.cpp
juniper_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
juniper_la_LDFLAGS = -module -avoid-version
juniper_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

mikrotik_la_SOURCES = mikrotik/data.cpp mikrotik/mikrotik.cpp
mikrotik_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
mikrotik_la_LDFLAGS = -module -avoid-version
mikrotik_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

moxa_la_SOURCES = moxa/edr.cpp moxa/moxa.cpp
moxa_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
moxa_la_LDFLAGS = -module -avoid-version
moxa_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

net_snmp_la_SOURCES = net-snmp/generic.cpp net-snmp/net-snmp.cpp net-snmp/teltonika.cpp
net_snmp_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
net_snmp_la_LDFLAGS = -module -avoid-version
net_snmp_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

netonix_la_SOURCES = netonix/netonix.cpp
netonix_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
netonix_la_LDFLAGS = -module -avoid-version
netonix_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

ping3_la_SOURCES = ping3/ping3.cpp
ping3_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
ping3_la_LDFLAGS = -module -avoid-version
ping3_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

qtech_olt_la_SOURCES = qtech-olt/qtech-olt.cpp
qtech_olt_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
qtech_olt_la_LDFLAGS = -module -avoid-version
qtech_olt_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

rittal_la_SOURCES = rittal/data.cpp rittal/rittal.cpp
rittal_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
rittal_la_LDFLAGS = -module -avoid-version
rittal_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

saf_la_SOURCES = saf/integra-b.cpp saf/saf.cpp
saf_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
saf_la_LDFLAGS = -module -avoid-version
saf_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

symbol_ws_la_SOURCES = symbol-ws/symbol-ws.cpp
symbol_ws_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
symbol_ws_la_LDFLAGS = -module -avoid-version
symbol_ws_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

tb_la_SOURCES = tb/tb.cpp
tb_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
tb_la_LDFLAGS = -module -avoid-version
tb_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

ubnt_la_SOURCES = ubnt/airmax.cpp ubnt/edgeswitch.cpp ubnt/ubnt.cpp
ubnt_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
ubnt_la_LDFLAGS = -module -avoid-version
ubnt_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

westerstrand_la_SOURCES = westerstrand/westerstrand.cpp
westerstrand_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
westerstrand_la_LDFLAGS = -module -avoid-version
westerstrand_la_LIBADD = ../../libnetxms/libnetxms.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../libnxsrv/libnxsrv.la

install-exec-hook:
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd ; for _DRIVER in $(DRIVERS); do if test "x`uname -s`" = "xAIX"; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$$_DRIVER.a $(DESTDIR)$(pkglibdir)/$$_DRIVER@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$$_DRIVER.a; fi ; mv -f $(DESTDIR)$(pkglibdir)/$$_DRIVER@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ndd/$$_DRIVER.ndd ; rm -f $(DESTDIR)$(pkglibdir)/$$_DRIVER.la; done
