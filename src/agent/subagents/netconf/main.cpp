/*
** NetXMS NETCONF subagent
** Copyright (C) 2026 Raden Solutions
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
** File: main.cpp
**
**/

#include "netconf_subagent.h"
#include <netxms-version.h>
#include <libssh/callbacks.h>

/**
 * Configuration options
 */
uint32_t g_netconfConnectTimeout = 5000;       // milliseconds
uint32_t g_netconfRequestTimeout = 60000;      // milliseconds
uint32_t g_netconfSessionIdleTimeout = 300;    // seconds

#ifdef _WIN32

/**
 * Mutex creation callback
 */
static int cb_mutex_init(void **mutex)
{
   *mutex = new Mutex(MutexType::FAST);
   return 0;
}

/**
 * Mutex destruction callback
 */
static int cb_mutex_destroy(void **mutex)
{
   delete static_cast<Mutex*>(*mutex);
   return 0;
}

/**
 * Mutex lock callback
 */
static int cb_mutex_lock(void **mutex)
{
   static_cast<Mutex*>(*mutex)->lock();
   return 0;
}

/**
 * Mutex unlock callback
 */
static int cb_mutex_unlock(void **mutex)
{
   static_cast<Mutex*>(*mutex)->unlock();
   return 0;
}

/**
 * Thread ID callback
 */
static unsigned long cb_thread_id()
{
   return static_cast<unsigned long>(GetCurrentThreadId());
}

/**
 * Custom callbacks for libssh threading support
 */
static struct ssh_threads_callbacks_struct s_threadCallbacks =
   {
      "netxms", cb_mutex_init, cb_mutex_destroy, cb_mutex_lock, cb_mutex_unlock, cb_thread_id
   };

#endif

/**
 * Configuration file template
 */
static NX_CFG_TEMPLATE s_cfgTemplate[] =
{
   { _T("ConnectTimeout"), CT_LONG, 0, 0, 0, 0, &g_netconfConnectTimeout },
   { _T("RequestTimeout"), CT_LONG, 0, 0, 0, 0, &g_netconfRequestTimeout },
   { _T("SessionIdleTimeout"), CT_LONG, 0, 0, 0, 0, &g_netconfSessionIdleTimeout },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   if (!config->parseTemplate(_T("NETCONF"), s_cfgTemplate))
      return false;

#ifndef _WIN32
   ssh_threads_set_callbacks(ssh_threads_get_noop());
#else
   ssh_threads_set_callbacks(&s_threadCallbacks);
#endif
   ssh_init();

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Using libssh version %hs"), ssh_version(0));

   InitializeSessionPool();
   return true;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   ShutdownSessionPool();
   ssh_finalize();
}

/**
 * Command handler for NETCONF commands
 */
static bool SubagentCommandHandler(uint32_t command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   if (command != CMD_NETCONF_EXECUTE)
      return false;
   ExecuteNETCONFRequest(*request, response, session);
   return true;
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("NETCONF.CheckConnection(*)"), H_NETCONFCheckConnection, nullptr, DCI_DT_INT, _T("Result of NETCONF connection check") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("NETCONF.Capabilities(*)"), H_NETCONFCapabilities, nullptr, _T("List of NETCONF capabilities announced by device") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("NETCONF"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, SubagentCommandHandler, nullptr, nullptr,
   sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   s_parameters,
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   s_lists,
   0, nullptr,	// tables
   0, nullptr,	// actions
   0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(NETCONF)
{
   *ppInfo = &s_info;
   return true;
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
