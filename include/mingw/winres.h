/*
** NetXMS - Network Management System
**
** File: mingw/winres.h
**
** Minimal winres.h shim for the legacy mingw32-xp toolchain, whose binutils
** windres ships winresrc.h but not the thin winres.h wrapper that the shared
** resource scripts (#include "winres.h") expect. Only the Windows XP build
** points windres at this directory (see config.mingw.xp); other toolchains use
** their own winres.h.
*/

#ifndef _mingw_winres_h_
#define _mingw_winres_h_

#include <winresrc.h>

#endif
