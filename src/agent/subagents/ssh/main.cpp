/*
** NetXMS SSH subagent
** Copyright (C) 2004-2016 Victor Kirhenshtein
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

#include "ssh_subagent.h"
#include <libssh/callbacks.h>

/**
 * Configuration options
 */
UINT32 g_sshConnectTimeout = 2000;
UINT32 g_sshSessionIdleTimeout = 300;

#if defined(_WIN32) || _USE_GNU_PTH

/**
 * Mutex creation callback
 */
static int cb_mutex_init(void **mutex)
{
   *mutex = MutexCreate();
   return 0;
}

/**
 * Mutex destruction callback
 */
static int cb_mutex_destroy(void **mutex)
{
   MutexDestroy(*((MUTEX *)mutex));
   return 0;
}

/**
 * Mutex lock callback
 */
static int cb_mutex_lock(void **mutex)
{
   MutexLock(*((MUTEX *)mutex));
   return 0;
}

/**
 * Mutex unlock callback
 */
static int cb_mutex_unlock(void **mutex)
{
   MutexUnlock(*((MUTEX *)mutex));
   return 0;
}

/**
 * Thread ID callback
 */
static unsigned long cb_thread_id()
{
   return (unsigned long)GetCurrentThreadId();
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
   { _T("ConnectTimeout"), CT_LONG, 0, 0, 0, 0, &g_sshConnectTimeout },
   { _T("SessionIdleTimeout"), CT_LONG, 0, 0, 0, 0, &g_sshSessionIdleTimeout },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
   if (!config->parseTemplate(_T("SSH"), s_cfgTemplate))
      return FALSE;

#if !defined(_WIN32) && !_USE_GNU_PTH
   ssh_threads_set_callbacks(ssh_threads_get_noop());
#else
   ssh_threads_set_callbacks(&s_threadCallbacks);
#endif
   ssh_init();
   nxlog_debug(2, _T("SSH: using libssh version %hs"), ssh_version(0));
   InitializeSessionPool();
	return TRUE;
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
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("SSH.Command(*)"), H_SSHCommand, NULL, DCI_DT_STRING, _T("Result of command execution") },
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("SSH.Command(*)"), H_SSHCommandList, NULL, _T("Result of command execution") },
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("SSH"), NETXMS_BUILD_TAG,
	SubagentInit, SubagentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
   sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   m_lists,
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(SSH)
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
