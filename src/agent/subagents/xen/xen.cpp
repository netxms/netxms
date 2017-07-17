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
LONG H_XenDomainList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_XenDomainTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
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
}

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
   return TRUE;
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
	{ _T("XEN.Host.Version"), H_XenHostVersion, NULL, DCI_DT_STRING, _T("XEN host: version") }
};

/**
 * Lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
	{ _T("XEN.VirtualMachines"), H_XenDomainList, NULL }
};

/**
 * Tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
    { _T("XEN.VirtualMachines"), H_XenDomainTable, NULL, _T("ID"), _T("XEN: virtual machines (domains)") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PING"), NETXMS_VERSION_STRING,
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
