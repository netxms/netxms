/*
** Windows Event Log Synchronization NetXMS subagent
** Copyright (C) 2020-2025 Raden Solutions
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
** File: wineventsync.cpp
**
**/

#include "wineventsync.h"
#include <netxms-version.h>

static ObjectArray<EventLogReader> s_readers(0, 16, Ownership::True);
static StringSet s_eventLogs;
static NX_CFG_TEMPLATE s_cfgTemplate[] =
{
   { _T("EventLog"), CT_STRING_SET, 0, 0, 0, 0, &s_eventLogs, nullptr },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
};

/**
 * Initialize subagent
 */
static bool SubAgentInit(Config *config)
{
   if (!config->parseTemplate(_T("WinEventSync"), s_cfgTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing WinEventSync configuration section"));
      return false;
   }

   auto it = s_eventLogs.begin();
   while (it.hasNext())
   {
      const TCHAR *log = it.next();
      auto reader = new EventLogReader(log, config);
      s_readers.add(reader);
      reader->start();
   }
   return true;
}

/**
 * Handler for subagent unload
 */
static void SubAgentShutdown()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Stopping event log readers"));
   for (int i = 0; i < s_readers.size(); i++)
      s_readers.get(i)->stop();
}

/**
 * Handler for WinEventSync.EventLogs parameter
 */
static LONG H_EventLogs(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for (int i = 0; i < s_readers.size(); i++)
      value->add(s_readers.get(i)->getName());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("WinEventSync.EventLogs"), H_EventLogs, nullptr }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("WinEventSync"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,    // callbacks
   0, nullptr,             // parameters
   sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   m_lists,
   0, nullptr,	// tables
   0, nullptr,	// actions
   0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(WINEVENTSYNC)
{
   *ppInfo = &m_info;
   return true;
}

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}
