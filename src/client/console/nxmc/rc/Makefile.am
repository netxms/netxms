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
resource_DATA = nxmc.xrs

EXTRA_DIST = \
	login.png \
	nxmc.fbp \
	nxmc.xrs \
	processing.png \
	wxfb_code.xrc

nxmc.xrs: resource.xrc manual.xrc
	wxrc resource.xrc manual.xrc -o nxmc.xrs

resource.xrc: wxfb_code.xrc
	$(PERL) @top_builddir@/tools/update_xrc.pl < wxfb_code.xrc > $@
