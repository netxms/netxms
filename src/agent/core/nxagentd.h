/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: nxagentd.h
**
**/

#ifndef _nxagentd_h_
#define _nxagentd_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <stdio.h>
#include "messages.h"


//
// Version
//

#ifdef _DEBUG
#define DEBUG_SUFFIX          "-debug"
#else
#define DEBUG_SUFFIX
#endif
#define AGENT_VERSION_STRING  NETXMS_VERSION_STRING ".1-alpha1" DEBUG_SUFFIX


//
// Default files
//

#ifdef _WIN32
#define AGENT_DEFAULT_CONFIG  "C:\\nxagentd.conf"
#define AGENT_DEFAULT_LOG     "C:\\nxagentd.log"
#else
#define AGENT_DEFAULT_CONFIG  "/etc/nxagentd.conf"
#define AGENT_DEFAULT_LOG     "/var/log/nxagentd"
#endif


//
// Constants
//

#ifdef _WIN32
#define AGENT_SERVICE_NAME    "NetXMSAgentdW32"
#define AGENT_EVENT_SOURCE    "NetXMS Win32 Agent"
#endif

#define MAX_SERVERS     32

#define AF_DAEMON       0x0001
#define AF_USE_SYSLOG   0x0002
#define AF_DEBUG        0x0004
#define AF_REQUIRE_AUTH 0x0008


//
// Parameter definition structure
//

struct AGENT_PARAM
{
   char name[MAX_PARAM_NAME];              // Parameter's name (wildcard)
   LONG (* handler)(char *,char *,char *); // Handler
   char *arg;                              // Optional argument passsed to the handler function
};


//
// Loaded subagent information
//

struct SUBAGENT
{
   HMODULE hModule;              // Subagent's module handle
   NETXMS_SUBAGENT_INFO *pInfo;  // Information provided by subagent
   char szName[MAX_PATH];        // Name of the module
};


//
// Functions
//

BOOL Initialize(void);
void Shutdown(void);
void Main(void);

void WriteLog(DWORD msg, WORD wType, char *format, ...);
void InitLog(void);
void CloseLog(void);

HMODULE DLOpen(char *szLibName);
void DLClose(HMODULE hModule);
void *DLGetSymbolAddr(HMODULE hModule, char *szSymbol);

void ConsolePrintf(char *pszFormat, ...);

BOOL InitParameterList(void);
void AddParameter(char *szName, LONG (* fpHandler)(char *,char *,char *), char *pArg);

BOOL LoadSubAgent(char *szModuleName);

#ifdef _WIN32

void InitService(void);
void InstallService(char *execName, char *confFile);
void RemoveService(void);
void StartAgentService(void);
void StopAgentService(void);
void InstallEventSource(char *path);
void RemoveEventSource(void);

char *GetSystemErrorText(DWORD dwError);
char *GetPdhErrorText(DWORD dwError);

#endif


//
// Global variables
//

extern DWORD g_dwFlags;
extern char g_szLogFile[];
extern char g_szSharedSecret[];
extern WORD g_wListenPort;
extern DWORD g_dwServerAddr[];
extern DWORD g_dwServerCount;
extern time_t g_dwAgentStartTime;

#endif   /* _nxagentd_h_ */
