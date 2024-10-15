# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

DRIVERS = dbtable dummy gsm portech shell snmptrap textfile @NCDRV_MODULES@

SUBDIRS = $(DRIVERS)

EXTRA_LTLIBRARIES = anysms.la googlechat.la kannel.la mattermost.la mqtt.la msteams.la mymobile.la nexmo.la nxagent.la slack.la smtp.la smseagle.la telegram.la text2reach.la twilio.la websms.la xmpp.la
pkglib_LTLIBRARIES = dbtable.la dummy.la gsm.la portech.la shell.la snmptrap.la textfile.la @NCDRV_LTLIBRARIES@

anysms_la_SOURCES = anysms/anysms.cpp
anysms_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
anysms_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
anysms_la_LIBADD = ../libnetxms/libnetxms.la

dbtable_la_SOURCES = dbtable/dbtable.cpp
dbtable_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
dbtable_la_LDFLAGS = -module -avoid-version
dbtable_la_LIBADD = ../db/libnxdb/libnxdb.la ../libnetxms/libnetxms.la

dummy_la_SOURCES = dummy/dummy.cpp
dummy_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
dummy_la_LDFLAGS = -module -avoid-version
dummy_la_LIBADD = ../libnetxms/libnetxms.la

gsm_la_SOURCES = gsm/gsm.cpp gsm/pdumode.cpp
gsm_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
gsm_la_LDFLAGS = -module -avoid-version
gsm_la_LIBADD = ../libnetxms/libnetxms.la

googlechat_la_SOURCES = googlechat/googlechat.cpp
googlechat_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
googlechat_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
googlechat_la_LIBADD = ../libnetxms/libnetxms.la

kannel_la_SOURCES = kannel/kannel.cpp
kannel_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
kannel_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
kannel_la_LIBADD = ../libnetxms/libnetxms.la

mattermost_la_SOURCES = mattermost/mattermost.cpp
mattermost_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
mattermost_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
mattermost_la_LIBADD = ../libnetxms/libnetxms.la

mqtt_la_SOURCES = mqtt/mqtt.cpp
mqtt_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
mqtt_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
mqtt_la_LIBADD = ../libnetxms/libnetxms.la -lmosquitto

msteams_la_SOURCES = msteams/msteams.cpp
msteams_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
msteams_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
msteams_la_LIBADD = ../libnetxms/libnetxms.la

mymobile_la_SOURCES = mymobile/mymobile.cpp
mymobile_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
mymobile_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
mymobile_la_LIBADD = ../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
mymobile_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
mymobile_la_LIBADD += -ljansson
endif

nexmo_la_SOURCES = nexmo/nexmo.cpp
nexmo_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
nexmo_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
nexmo_la_LIBADD = ../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
nexmo_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nexmo_la_LIBADD += -ljansson
endif

nxagent_la_SOURCES = nxagent/nxagent.cpp
nxagent_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxagent_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
nxagent_la_LIBADD = ../libnetxms/libnetxms.la ../server/libnxsrv/libnxsrv.la

portech_la_SOURCES = portech/portech.cpp portech/pdumode.cpp
portech_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
portech_la_LDFLAGS = -module -avoid-version
portech_la_LIBADD = ../libnetxms/libnetxms.la

shell_la_SOURCES = shell/shell.cpp
shell_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
shell_la_LDFLAGS = -module -avoid-version
shell_la_LIBADD = ../libnetxms/libnetxms.la

slack_la_SOURCES = slack/slack.cpp
slack_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
slack_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
slack_la_LIBADD = ../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
slack_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
slack_la_LIBADD += -ljansson
endif

smseagle_la_SOURCES = smseagle/smseagle.cpp
smseagle_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
smseagle_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
smseagle_la_LIBADD = ../libnetxms/libnetxms.la

smtp_la_SOURCES = smtp/smtp.cpp
smtp_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
smtp_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
smtp_la_LIBADD = ../libnetxms/libnetxms.la

snmptrap_la_SOURCES = snmptrap/snmptrap.cpp
snmptrap_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
snmptrap_la_LDFLAGS = -module -avoid-version
snmptrap_la_LIBADD = ../libnetxms/libnetxms.la ../snmp/libnxsnmp/libnxsnmp.la

telegram_la_SOURCES = telegram/telegram.cpp
telegram_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
telegram_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
telegram_la_LIBADD = ../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
telegram_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
telegram_la_LIBADD += -ljansson
endif

text2reach_la_SOURCES = text2reach/text2reach.cpp
text2reach_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
text2reach_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
text2reach_la_LIBADD = ../libnetxms/libnetxms.la

textfile_la_SOURCES = textfile/textfile.cpp
textfile_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
textfile_la_LDFLAGS = -module -avoid-version
textfile_la_LIBADD = ../libnetxms/libnetxms.la

twilio_la_SOURCES = twilio/twilio.cpp
twilio_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
twilio_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
twilio_la_LIBADD = ../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
twilio_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
twilio_la_LIBADD += -ljansson
endif

websms_la_SOURCES = websms/websms.cpp
websms_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
websms_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
websms_la_LIBADD = ../libnetxms/libnetxms.la

xmpp_la_SOURCES = xmpp/xmpp.cpp
xmpp_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
xmpp_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
xmpp_la_LIBADD = ../libnetxms/libnetxms.la -lstrophe

install-exec-hook:
	mkdir -p $(DESTDIR)$(pkglibdir)/ncdrv ; for _DRIVER in $(DRIVERS); do if test "x`uname -s`" = "xAIX"; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$$_DRIVER.a $(DESTDIR)$(pkglibdir)/$$_DRIVER@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$$_DRIVER.a; fi ; mv -f $(DESTDIR)$(pkglibdir)/$$_DRIVER@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncdrv/$$_DRIVER.ncd ; rm -f $(DESTDIR)$(pkglibdir)/$$_DRIVER.la; done
