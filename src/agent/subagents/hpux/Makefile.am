# Copyright (C) 2006 NetXMS Team <bugs@netxms.org>
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

INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_hpux.la
libnsm_hpux_la_SOURCES = disk.cpp iostat.cpp main.cpp net.cpp system.cpp
libnsm_hpux_la_CPPFLAGS = -D_PSTAT64
libnsm_hpux_la_LDFLAGS = ../../../libnetxms/libnetxms.la

EXTRA_DIST = disk.h hpux.h net.h system.h
