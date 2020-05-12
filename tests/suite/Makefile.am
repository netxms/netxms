# Copyright (C) 2004-2020 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_SCRIPTS = netxms-test-suite
EXTRA_DIST = netxms-test-suite.cmd netxms-test-suite.in

REPLACE_CMD = sed -e "s,~BINDIR~,@NETXMS_BINDIR@,g" -e "s,~DATADIR~,@NETXMS_DATADIR@,g" -e "s,~PKGLIBDIR~,@NETXMS_PKGLIBDIR@,g" -e "s,~SYSCONFDIR~,@NETXMS_SYSCONFDIR@,g" < $@.in > $@

netxms-test-suite: netxms-test-suite.in
	$(REPLACE_CMD)

clean-local:
	rm -f netxms-test-suite
