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
 **/

#ifndef _xen_h_
#define _xen_h_

#include <nms_common.h>
#include <nms_agent.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libxl.h>
#include <libxl_utils.h>

#ifdef __cplusplus
}
#endif

/**
 * Logger definition
 */
extern xentoollog_logger g_xenLogger;

/**
 * Connect to XEN host - common macro for parameter handlers
 */
#define XEN_CONNECT(ctx) do { \
   int rc = libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, &g_xenLogger); \
   if (rc != 0) \
   { \
      nxlog_debug(5, _T("XEN: libxl_ctx_alloc failed (%d)"), rc); \
      return SYSINFO_RC_ERROR; \
   } \
} while(0)

/**
 * CPU collector start/stop
 */
void XenStartCPUCollector();
void XenStopCPUCollector();

/**
 * Resolve domain name to ID
 */
LONG XenResolveDomainName(const char *name, uint32_t *domId);

/**
 * Query domain CPU usage
 */
bool XenQueryDomainCpuUsage(uint32_t domId, INT32 *curr, INT32 *avg1min);

/**
 * Query network traffic for domain
 */
bool XenQueryDomainNetworkTraffic(uint32_t domId, UINT64 *rxBytes, UINT64 *txBytes, UINT64 *rxPackets, UINT64 *txPackets);

#endif
