# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

resourcedir = $(datadir)/nxmc
resource_DATA = ObjectBrowser.xrs

EXTRA_DIST = \
	active.xpm \
	disabled.xpm \
	lastval.xpm \
	ObjectBrowser.xrc \
	ObjectBrowser.xrs \
	objectbrowser.xpm \
	unsupported.xpm

ObjectBrowser.xrs: ObjectBrowser.xrc
	wxrc ObjectBrowser.xrc -o ObjectBrowser.xrs
