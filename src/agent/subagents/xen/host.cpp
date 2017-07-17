/*
** NetXMS XEN hypervisor subagent
** Copyright (C) 2017 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: host.cpp
**
**/

#include "xen.h"

/**
 * Handler for XEN.Host.Version
 */
LONG H_XenHostVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   libxl_ctx *ctx;
   XEN_CONNECT(ctx);

   const libxl_version_info *v = libxl_get_version_info(ctx);
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%d%hs"), v->xen_version_major, v->xen_version_minor, v->xen_version_extra);

   libxl_ctx_free(ctx);
   return SYSINFO_RC_SUCCESS;
}
