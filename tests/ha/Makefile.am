# Copyright (C) 2026 Raden Solutions
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = ha-node-sim
ha_node_sim_SOURCES = ha-node-sim.cpp ../../src/server/core/halease.cpp
ha_node_sim_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
ha_node_sim_LDFLAGS = @EXEC_LDFLAGS@
ha_node_sim_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/db/libnxdb/libnxdb.la @EXEC_LIBS@

EXTRA_DIST = README.md harness.py lsan-suppressions.txt
