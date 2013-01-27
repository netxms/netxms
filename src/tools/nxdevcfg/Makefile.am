# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = nxdevcfg
nxdevcfg_SOURCES = flags.cpp nxdevcfg.cpp
nxdevcfg_CPPFLAGS=-I@top_srcdir@/include
nxdevcfg_LDADD = ../../libnetxms/libnetxms.la

if USE_INTERNAL_LIBTRE
nxdevcfg_LDADD += ../../libtre/libnxtre.la
endif
