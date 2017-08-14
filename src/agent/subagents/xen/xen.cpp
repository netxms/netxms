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
** File: xen.cpp
**
**/

#include "xen.h"

/**
 * Handlers
 */
LONG H_XenDomainCPUUsage(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_XenDomainList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_XenDomainNetIfTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_XenDomainNetStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_XenDomainState(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_XenDomainTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_XenHostCPUUsage(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_XenHostOnlineCPUs(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_XenHostPhyInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_XenHostVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Convert XEN log level into NetXMS debug level
 */
inline int ConvertLogLevel(xentoollog_level level)
{
   if (level >= XTL_ERROR)
      return 1;
   if (level >= XTL_WARN)
      return 3;
   if (level >= XTL_NOTICE)
      return 4;
   return 5;
}

/**
 * Write log message
 */
static void LogMessage(struct xentoollog_logger *logger,
                     xentoollog_level level,
                     int errnoval /* or -1 */,
                     const char *context /* eg "xc", "xl", may be 0 */,
                     const char *format /* without level, context, \n */,
                     va_list al)
{
   char msg[4096];
   vsnprintf(msg, 4096, format, al);
   nxlog_debug(ConvertLogLevel(level), _T("XEN: [%hs] (%hs): %hs"), xtl_level_to_string(level), context, msg);
}

/**
 * Write progress message
 */
static void LogProgress(struct xentoollog_logger *logger,
                     const char *context /* see above */,
                     const char *doing_what /* no \r,\n */,
                     int percent, unsigned long done, unsigned long total)
{
}

/**
 * Destroy logger
 */
static void LogDestroy(struct xentoollog_logger *logger)
{
}

/**
 * Logger definition
 */
xentoollog_logger g_xenLogger = { LogMessage, LogProgress, LogDestroy };

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   XenStopCPUCollector();
}

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
   XenStartCPUCollector();
   return TRUE;
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("XEN.Domain.CPU.CurrentUsage(*)"), H_XenDomainCPUUsage, _T("0"), DCI_DT_FLOAT, _T("XEN domain {instance}: current CPU utilization") },
   { _T("XEN.Domain.CPU.Usage(*)"), H_XenDomainCPUUsage, _T("1"), DCI_DT_FLOAT, _T("XEN domain {instance}: average CPU utilization for last minute") },
   { _T("XEN.Domain.CPU.Usage15(*)"), H_XenDomainCPUUsage, _T("F"), DCI_DT_FLOAT, _T("XEN domain {instance}: average CPU utilization for last 15 minutes") },
   { _T("XEN.Domain.CPU.Usage5(*)"), H_XenDomainCPUUsage, _T("5"), DCI_DT_FLOAT, _T("XEN domain {instance}: average CPU utilization for last 5 minutes") },
   { _T("XEN.Domain.IsExist(*)"), H_XenDomainState, _T("E"), DCI_DT_INT, _T("XEN domain {instance}: existence flag") },
   { _T("XEN.Domain.IsOperational(*)"), H_XenDomainState, _T("O"), DCI_DT_INT, _T("XEN domain {instance}: operational flag") },
   { _T("XEN.Domain.IsPaused(*)"), H_XenDomainState, _T("P"), DCI_DT_INT, _T("XEN domain {instance}: paused flag") },
   { _T("XEN.Domain.Net.RxBytes(*)"), H_XenDomainNetStats, _T("RB"), DCI_DT_UINT64, _T("XEN domain {instance}: network usage (bytes received)") },
   { _T("XEN.Domain.Net.RxPackets(*)"), H_XenDomainNetStats, _T("RP"), DCI_DT_UINT64, _T("XEN domain {instance}: network usage (packets received)") },
   { _T("XEN.Domain.Net.TxBytes(*)"), H_XenDomainNetStats, _T("TB"), DCI_DT_UINT64, _T("XEN domain {instance}: network usage (bytes transmitted)") },
   { _T("XEN.Domain.Net.TxPackets(*)"), H_XenDomainNetStats, _T("TP"), DCI_DT_UINT64, _T("XEN domain {instance}: network usage (packets transmitted)") },
   { _T("XEN.Domain.State(*)"), H_XenDomainState, _T("?"), DCI_DT_INT, _T("XEN domain {instance}: state") },
   { _T("XEN.Host.CPU.Cores"), H_XenHostPhyInfo, _T("c"), DCI_DT_INT, _T("XEN host: number of CPU cores") },
   { _T("XEN.Host.CPU.CurrentUsage"), H_XenHostCPUUsage, _T("0"), DCI_DT_FLOAT, _T("XEN host: current CPU utilization") },
   { _T("XEN.Host.CPU.Frequency"), H_XenHostPhyInfo, _T("F"), DCI_DT_INT, _T("XEN host: number of CPU cores") },
   { _T("XEN.Host.CPU.LogicalCount"), H_XenHostPhyInfo, _T("C"), DCI_DT_INT, _T("XEN host: number of logical CPUs") },
   { _T("XEN.Host.CPU.Online"), H_XenHostOnlineCPUs, NULL, DCI_DT_INT, _T("XEN host: number of online CPUs") },
   { _T("XEN.Host.CPU.PhysicalCount"), H_XenHostPhyInfo, _T("P"), DCI_DT_INT, _T("XEN host: number of physical CPUs") },
   { _T("XEN.Host.CPU.Usage"), H_XenHostCPUUsage, _T("1"), DCI_DT_FLOAT, _T("XEN host: average CPU utilization for last minute") },
   { _T("XEN.Host.CPU.Usage15"), H_XenHostCPUUsage, _T("F"), DCI_DT_FLOAT, _T("XEN host: average CPU utilization for last 15 minutes") },
   { _T("XEN.Host.CPU.Usage5"), H_XenHostCPUUsage, _T("5"), DCI_DT_FLOAT, _T("XEN host: average CPU utilization for last 5 minutes") },
   { _T("XEN.Host.Memory.Free"), H_XenHostPhyInfo, _T("M"), DCI_DT_UINT64, _T("XEN host: free memory") },
   { _T("XEN.Host.Memory.FreePerc"), H_XenHostPhyInfo, _T("m"), DCI_DT_FLOAT, _T("XEN host: free memory (%)") },
   { _T("XEN.Host.Memory.Outstanding"), H_XenHostPhyInfo, _T("O"), DCI_DT_UINT64, _T("XEN host: outstanding memory") },
   { _T("XEN.Host.Memory.OutstandingPerc"), H_XenHostPhyInfo, _T("o"), DCI_DT_FLOAT, _T("XEN host: outstanding memory (%)") },
   { _T("XEN.Host.Memory.Scrub"), H_XenHostPhyInfo, _T("S"), DCI_DT_UINT64, _T("XEN host: scrub memory") },
   { _T("XEN.Host.Memory.ScrubPerc"), H_XenHostPhyInfo, _T("s"), DCI_DT_FLOAT, _T("XEN host: scrub memory (%)") },
   { _T("XEN.Host.Memory.Total"), H_XenHostPhyInfo, _T("T"), DCI_DT_UINT64, _T("XEN host: total memory") },
   { _T("XEN.Host.Memory.Used"), H_XenHostPhyInfo, _T("U"), DCI_DT_UINT64, _T("XEN host: used memory") },
   { _T("XEN.Host.Memory.UsedPerc"), H_XenHostPhyInfo, _T("u"), DCI_DT_FLOAT, _T("XEN host: used memory (%)") },
	{ _T("XEN.Host.Version"), H_XenHostVersion, NULL, DCI_DT_STRING, _T("XEN host: version") }
};

/**
 * Lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
	{ _T("XEN.Domains"), H_XenDomainList, NULL }
};

/**
 * Tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("XEN.Domains"), H_XenDomainTable, NULL, _T("ID"), _T("XEN: domains (virtual machines)") },
   { _T("XEN.Net.DomainInterfaces"), H_XenDomainNetIfTable, NULL, _T("NAME"), _T("XEN: domain network interfaces") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("XEN"), NETXMS_BUILD_TAG,
	SubagentInit, SubagentShutdown, NULL,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	s_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	s_lists,
	sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
	s_tables,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(XEN)
{
	*ppInfo = &m_info;
	return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
