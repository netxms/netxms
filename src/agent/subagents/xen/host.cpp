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

/**
 * Handler for XEN.Host.CPU.Online
 */
LONG H_XenHostOnlineCPUs(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   libxl_ctx *ctx;
   XEN_CONNECT(ctx);
   ret_int(value, libxl_get_online_cpus(ctx));
   libxl_ctx_free(ctx);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get correct number of free pages
 */
inline uint64_t FreePages(const libxl_physinfo &pi)
{
#if LIBXL_HAVE_PHYSINFO_OUTSTANDING_PAGES
   return pi.free_pages - pi.outstanding_pages;
#else
   return pi.free_pages;
#endif
}

/**
 * Handler for XEN.Host.* parameters related to host physical info
 */
LONG H_XenHostPhyInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   libxl_ctx *ctx;
   XEN_CONNECT(ctx);

   LONG result;

   libxl_physinfo pi;
   libxl_physinfo_init(&pi);
   int rc = libxl_get_physinfo(ctx, &pi);
   if (rc == 0)
   {
      result = SYSINFO_RC_SUCCESS;
      switch(*arg)
      {
         case 'C':   // Logical CPUs
            ret_int(value, pi.nr_cpus);
            break;
         case 'c':   // Cores
            ret_int(value, pi.nr_cpus / pi.threads_per_core);
            break;
         case 'F':   // CPU frequency (KHz)
            ret_int(value, pi.cpu_khz);
            break;
         case 'M':   // Free memory
            ret_uint64(value, FreePages(pi) * libxl_get_version_info(ctx)->pagesize);
            break;
         case 'm':   // Free memory %
            ret_double(value, (double)FreePages(pi) * 100.0 / (double)pi.total_pages, 2);
            break;
         case 'O':   // Outstanding memory
#if LIBXL_HAVE_PHYSINFO_OUTSTANDING_PAGES
            ret_uint64(value, pi.outstanding_pages * libxl_get_version_info(ctx)->pagesize);
#else
            result = SYSINFO_RC_UNSUPPORTED;
#endif
            break;
         case 'o':   // Scrub memory %
#if LIBXL_HAVE_PHYSINFO_OUTSTANDING_PAGES
            ret_double(value, (double)pi.outstanding_pages * 100.0 / (double)pi.total_pages, 2);
#else
            result = SYSINFO_RC_UNSUPPORTED;
#endif
            break;
         case 'P':   // Physical CPUs
            ret_int(value, pi.nr_cpus / pi.threads_per_core / pi.cores_per_socket);
            break;
         case 'S':   // Scrub memory
            ret_uint64(value, pi.scrub_pages * libxl_get_version_info(ctx)->pagesize);
            break;
         case 's':   // Scrub memory %
            ret_double(value, (double)pi.scrub_pages * 100.0 / (double)pi.total_pages, 2);
            break;
         case 'T':   // Total memory
            ret_uint64(value, pi.total_pages * libxl_get_version_info(ctx)->pagesize);
            break;
         case 'U':   // Used memory
            ret_uint64(value, (pi.total_pages - FreePages(pi)) * libxl_get_version_info(ctx)->pagesize);
            break;
         case 'u':   // Used memory %
            ret_double(value, (double)(pi.total_pages - FreePages(pi)) * 100.0 / (double)pi.total_pages, 2);
            break;
         default:
            result = SYSINFO_RC_UNSUPPORTED;
            break;
      }
   }
   else
   {
      nxlog_debug(5, _T("XEN: libxl_get_physinfo failed (%d)"), rc);
      result = SYSINFO_RC_ERROR;
   }
   libxl_physinfo_dispose(&pi);

   libxl_ctx_free(ctx);
   return result;
}
