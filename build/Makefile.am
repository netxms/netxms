# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

BUILT_SOURCES = netxms-build-tag.iss netxms-build-tag.h netxms-build-tag.properties

EXTRA_DIST = \
	build.vcxproj \
	build.vcxproj.filters \
	Makefile.w32 \
	msvc_setenv_x64.cmd \
	msvc_setenv_x86.cmd \
	updatetag.pl

netxms-build-tag.properties: netxms-build-tag.iss

netxms-build-tag.iss: netxms-build-tag.h

netxms-build-tag.h: update-tag

.PHONY: update-tag

update-tag:
	./updatetag.pl
