# Copyright (C) 2004-2022 Raden Solutions <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_SCRIPTS = nx-run-asan-binary
pkgdata_DATA = lsan-suppressions.txt
EXTRA_DIST = lsan-suppressions.txt nx-run-asan-binary.in

REPLACE_CMD = sed -e "s,~BINDIR~,@NETXMS_BINDIR@,g" -e "s,~DATADIR~,@NETXMS_DATADIR@,g" -e "s,~PKGLIBDIR~,@NETXMS_PKGLIBDIR@,g" -e "s,~SYSCONFDIR~,@NETXMS_SYSCONFDIR@,g" < $@.in > $@

nx-run-asan-binary: nx-run-asan-binary.in
	$(REPLACE_CMD)

clean-local:
	rm -f nx-run-asan-binary
