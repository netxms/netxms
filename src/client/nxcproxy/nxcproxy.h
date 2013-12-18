/* 
** NetXMS - Network Management System
** Client proxy
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: nxcproxy.h
**
**/

#ifndef _nxcproxy_h_
#define _nxcproxy_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include "messages.h"

/**
 * Win32 service and syslog constants
 */
#ifdef _WIN32

#define NXCPROXY_SERVICE_NAME    _T("nxcproxy")
#define NXCPROXY_EVENT_SOURCE    _T("nxcproxy")    
#define NXCPROXY_SYSLOG_NAME     NXCPROXY_EVENT_SOURCE

#else

#define NXCPROXY_SYSLOG_NAME     _T("nxcproxy")

#endif

/**
 * Default files
 */
#if defined(_WIN32)
#define NXCPROXY_DEFAULT_CONFIG     _T("C:\\nxcproxy.conf")
#define NXCPROXY_DEFAULT_LOG        _T("C:\\nxcproxy.log")
#else
#define NXCPROXY_DEFAULT_CONFIG     _T("{search}")
#define NXCPROXY_DEFAULT_LOG        _T("/var/log/nxcproxy")
#endif

/**
 * App flags
 */
#define AF_DAEMON                   0x00000001
#define AF_USE_SYSLOG               0x00000002
#define AF_SHUTDOWN                 0x00000004
#define AF_CATCH_EXCEPTIONS         0x00000008
#define AF_WRITE_FULL_DUMP          0x00000010

/**
 * Session
 */
class ProxySession
{
private:
   SOCKET m_client;
   SOCKET m_server;

   static THREAD_RESULT THREAD_CALL clientThreadStarter(void *arg);
   static THREAD_RESULT THREAD_CALL serverThreadStarter(void *arg);

   void clientThread();
   void serverThread();
   void forward(SOCKET rs, SOCKET ws);

public:
   ProxySession(SOCKET s);
   ~ProxySession();

   void run();
};

/**
 * Functions
 */
bool Initialize();
void Main();
void Shutdown();

void ConsolePrintf(const TCHAR *pszFormat, ...);
void DebugPrintf(int level, const TCHAR *pszFormat, ...);

#ifdef _WIN32

void InitService();
void InstallService(const TCHAR *pszExecName, const TCHAR *pszLogin, const TCHAR *pszPassword);
void RemoveService();
void CheckServiceConfig();
void StartProxyService();
void StopProxyService();
void InstallEventSource(const TCHAR *pszPath);
void RemoveEventSource();

#endif   /* _WIN32 */

/**
 * Global variables
 */
extern UINT32 g_flags;
extern TCHAR g_logFile[];
extern TCHAR g_configFile[];
extern TCHAR g_listenAddress[];
extern WORD g_listenPort;
extern TCHAR g_serverAddress[];
extern WORD g_serverPort;
extern UINT32 g_debugLevel;

#endif   /* _nxcproxy_h_ */
