# Copyright (C) 2012 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

EXTRA_DIST = nxagentd.in

REPLACE_CMD = sed "s,~BINDIR~,$(bindir),g" < $@.in | \
	sed "s,~LIBDIR~,$(libdir),g" > $@

all: nxagentd

nxagentd: nxagentd.in
	$(REPLACE_CMD)

clean:
	rm -f nxagentd
