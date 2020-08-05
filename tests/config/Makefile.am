# Copyright (C) 2004-2020 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

jenkinsconfdir=$(sysconfdir)/jenkins
jenkinsconf_DATA = netxmsd.conf nxagentd.conf
EXTRA_DIST = netxmsd.conf.in nxagentd.conf.in

netxmsd.conf: netxmsd.conf.in
	sed -e "s,~DB_DRIVER~,${JENKINS_RUN_DB_DRIVER},g" -e "s^~DB_SERVER~^${JENKINS_RUN_DB_SERVER}^g" -e "s,~DB_NAME~,${JENKINS_RUN_DB_NAME},g" -e "s,~DB_LOGIN~,${JENKINS_RUN_DB_LOGIN},g" -e "s,~DB_PASSWORD~,${JENKINS_RUN_DB_PASSWORD},g" -e "s,~STATEDIR~,@NETXMS_STATEDIR@,g" -e "s,~ADDITIONAL_CONFIG_PARAM~,${JENKINS_ADDITIONAL_CONFIG_PARAM},g" < $@.in > $@

nxagentd.conf: nxagentd.conf.in
	sed -e "s!~MASTER_SERVERS~!${JENKINS_RUN_MASTER_SERVERS}!g" -e "s,~LISTEN_PORT~,${JENKINS_RUN_AGENT_LISTEN_PORT},g" -e "s,~STATEDIR~,@NETXMS_STATEDIR@,g" < $@.in > $@

clean-local:
	rm -f netxmsd.conf nxagentd.conf
