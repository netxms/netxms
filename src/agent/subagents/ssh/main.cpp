/*
** NetXMS SSH subagent
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
#include <netxms-version.h>
#include <netxms-regex.h>
#include <libssh/callbacks.h>

/**
 * Configuration options
 */
uint32_t g_sshConnectTimeout = 2000;
uint32_t g_sshSessionIdleTimeout = 300;
char g_sshConfigFile[MAX_PATH] = "";
bool g_sshChannelReadBugWorkaround = false;

#if defined(_WIN32) || _USE_GNU_PTH

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
   { _T("ConfigFile"), CT_MB_STRING, 0, 0, MAX_PATH, 0, g_sshConfigFile },
   { _T("ConnectTimeout"), CT_LONG, 0, 0, 0, 0, &g_sshConnectTimeout },
   { _T("SessionIdleTimeout"), CT_LONG, 0, 0, 0, 0, &g_sshSessionIdleTimeout },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   if (!config->parseTemplate(_T("SSH"), s_cfgTemplate))
      return false;

#if !defined(_WIN32) && !_USE_GNU_PTH
   ssh_threads_set_callbacks(ssh_threads_get_noop());
#else
   ssh_threads_set_callbacks(&s_threadCallbacks);
#endif
   ssh_init();

   const char *version = ssh_version(0);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Using libssh version %hs"), version);
   const char *eptr;
   int eoffset;
   pcre *re = pcre_compile("^([0-9]+)\\.([0-9]+)\\.([0-9]+)(\\/.*)?", PCRE_COMMON_FLAGS_A, &eptr, &eoffset, nullptr);
   if (re != nullptr)
   {
      int pmatch[30];
      if (pcre_exec(re, nullptr, version, static_cast<int>(strlen(version)), 0, 0, pmatch, 30) == 5)
      {
         uint32_t major = IntegerFromCGroupA(version, pmatch, 1);
         uint32_t minor = IntegerFromCGroupA(version, pmatch, 2);
         uint32_t release = IntegerFromCGroupA(version, pmatch, 3);
         if ((major == 0) && ((minor < 9) || ((minor == 9) && (release < 5))))
            g_sshChannelReadBugWorkaround = true;
      }
      pcre_free(re);
   }
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Workaround for ssh_channel_read bug is %s"), g_sshChannelReadBugWorkaround ? _T("enabled") : _T("disabled"));

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
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("SSH.CheckConnection(*)"), H_SSHConnection, nullptr, DCI_DT_STRING, _T("Result of connectivity check") },
	{ _T("SSH.Command(*)"), H_SSHCommand, nullptr, DCI_DT_STRING, _T("Result of command execution") },
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("SSH.Command(*)"), H_SSHCommandList, nullptr, _T("Result of command execution") },
};

/**
 * Supported actions
 */
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
	{ _T("SSH.Command"), H_SSHCommandAction, nullptr, _T("Result of command execution") },
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("SSH"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, nullptr, nullptr, nullptr,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   m_lists,
   0, nullptr,	// tables
   sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
   m_actions,
   0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(SSH)
{
   *ppInfo = &m_info;
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
