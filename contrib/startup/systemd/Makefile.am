# Copyright (C) 2003-2017 Raden Solutions
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

EXTRA_DIST = netxms-agent.service.in netxms-server.service.in

REPLACE_CMD = sed "s,~BINDIR~,$(bindir),g" < $@.in | \
    sed "s,~LIBDIR~,$(libdir),g" > $@

all: netxms-server.service netxms-agent.service

netxms-server.service: netxms-server.service.in
	$(REPLACE_CMD)

netxms-agent.service: netxms-agent.service.in
	$(REPLACE_CMD)

clean-local:
	rm -f netxms-agent.service netxms-server.service
