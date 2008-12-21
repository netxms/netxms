# Copyright (C) 2006 NetXMS Team
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

INCLUDES=-I@top_srcdir@/include
AM_CXXFLAGS=-fpermissive 

lib_LTLIBRARIES = libnsm_openbsd.la
libnsm_openbsd_la_SOURCES = openbsd.cpp system.cpp disk.cpp net.cpp
libnsm_openbsd_la_LDFLAGS = -lkvm

EXTRA_DIST = disk.h net.h system.h
