# Copyright (C) 2004-2021 Raden Solutions <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_SCRIPTS = nxreportd
EXTRA_DIST = nxreportd.in
SUBDIRS=java

nxreportd: nxreportd.in
	sed -e "s,~PACKAGE_VERSION~,$(PACKAGE_VERSION),g" -e "s,~BINDIR~,@NETXMS_BINDIR@,g" -e "s,~DATADIR~,@NETXMS_DATADIR@,g" -e "s,~PKGLIBDIR~,@NETXMS_PKGLIBDIR@,g" -e "s,~STATEDIR~,@NETXMS_STATEDIR@,g" -e "s,~SYSCONFDIR~,@NETXMS_SYSCONFDIR@,g" < $@.in > $@

clean-local:
	rm -f nxreportd
