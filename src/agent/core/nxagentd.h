/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
#include <nms_threads.h>
#include <nms_agent.h>
#include <nms_cscp.h>
#include <stdio.h>
#include <nxqueue.h>
#include "messages.h"

#define LIBNXCL_NO_DECLARATIONS
#include <nxclapi.h>

#ifdef _NETWARE
#undef SEVERITY_CRITICAL
#endif


//
// Version
//

#ifdef _DEBUG
#define DEBUG_SUFFIX          "-debug"
#else
#define DEBUG_SUFFIX          ""
#endif
#define AGENT_VERSION_STRING  NETXMS_VERSION_STRING DEBUG_SUFFIX


//
// Default files
//

#if defined(_WIN32)
#define AGENT_DEFAULT_CONFIG     "C:\\nxagentd.conf"
#define AGENT_DEFAULT_LOG        "C:\\nxagentd.log"
#define AGENT_DEFAULT_FILE_STORE "C:\\"
#elif defined(_NETWARE)
#define AGENT_DEFAULT_CONFIG     "SYS:ETC/nxagentd.conf"
#define AGENT_DEFAULT_LOG        "SYS:ETC/nxagentd.log"
#define AGENT_DEFAULT_FILE_STORE "SYS:\\"
#else
#define AGENT_DEFAULT_CONFIG     "/etc/nxagentd.conf"
#define AGENT_DEFAULT_LOG        "/var/log/nxagentd"
#define AGENT_DEFAULT_FILE_STORE "/tmp"
#endif


//
// Constants
//

#ifdef _WIN32
#define AGENT_SERVICE_NAME    "NetXMSAgentdW32"
#define AGENT_EVENT_SOURCE    "NetXMS Win32 Agent"
#endif

#define MAX_PSUFFIX_LENGTH 32
#define MAX_SERVERS        32

#define AF_DAEMON                   0x0001
#define AF_USE_SYSLOG               0x0002
#define AF_DEBUG                    0x0004
#define AF_REQUIRE_AUTH             0x0008
#define AF_LOG_UNRESOLVED_SYMBOLS   0x0010
#define AF_ENABLE_ACTIONS           0x0020
#define AF_REQUIRE_ENCRYPTION       0x0040
#define AF_HIDE_WINDOW              0x0080
#define AF_SHUTDOWN                 0x1000


#ifdef _WIN32


//
// Attributes for H_NetIPStats and H_NetInterfacStats
//

#define NET_IP_FORWARDING        1

#define NET_IF_BYTES_IN          1
#define NET_IF_BYTES_OUT         2
#define NET_IF_DESCR             3
#define NET_IF_IN_ERRORS         4
#define NET_IF_LINK              5
#define NET_IF_OUT_ERRORS        6
#define NET_IF_PACKETS_IN        7
#define NET_IF_PACKETS_OUT       8
#define NET_IF_SPEED             9
#define NET_IF_ADMIN_STATUS      10


//
// Request types for H_MemoryInfo
//

#define MEMINFO_PHYSICAL_FREE    1
#define MEMINFO_PHYSICAL_TOTAL   2
#define MEMINFO_PHYSICAL_USED    3
#define MEMINFO_SWAP_FREE        4
#define MEMINFO_SWAP_TOTAL       5
#define MEMINFO_SWAP_USED        6
#define MEMINFO_VIRTUAL_FREE     7
#define MEMINFO_VIRTUAL_TOTAL    8
#define MEMINFO_VIRTUAL_USED     9


//
// Request types for H_DiskInfo
//

#define DISKINFO_FREE_BYTES      1
#define DISKINFO_USED_BYTES      2
#define DISKINFO_TOTAL_BYTES     3
#define DISKINFO_FREE_SPACE_PCT  4
#define DISKINFO_USED_SPACE_PCT  5

#endif   /* _WIN32 */


//
// Action types
//

#define AGENT_ACTION_EXEC        1
#define AGENT_ACTION_SUBAGENT    2


//
// Action definition structure
//

typedef struct
{
   char szName[MAX_PARAM_NAME];
   int iType;
   union
   {
      char *pszCmdLine;
      struct __subagentAction
      {
         LONG (*fpHandler)(TCHAR *, NETXMS_VALUES_LIST *, TCHAR *);
         char *pArg;
         char szSubagentName[MAX_PATH];
      } sa;
   } handler;
   char szDescription[MAX_DB_STRING];
} ACTION;


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
// Server information
//

struct SERVER_INFO
{
   DWORD dwIpAddr;
   BOOL bMasterServer;
   BOOL bControlServer;
};


//
// Communication session
//

class CommSession
{
private:
   SOCKET m_hSocket;
   Queue *m_pSendQueue;
   Queue *m_pMessageQueue;
   CSCP_BUFFER *m_pMsgBuffer;
   THREAD m_hWriteThread;
   THREAD m_hProcessingThread;
   DWORD m_dwHostAddr;        // IP address of connected host (network byte order)
   DWORD m_dwIndex;
   BOOL m_bIsAuthenticated;
   BOOL m_bMasterServer;
   BOOL m_bControlServer;
   int m_hCurrFile;
   DWORD m_dwFileRqId;
   CSCP_ENCRYPTION_CONTEXT *m_pCtx;
   time_t m_ts;               // Last command timestamp

   void Authenticate(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void GetConfig(CSCPMessage *pMsg);
   void UpdateConfig(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void GetParameter(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void GetList(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void Action(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void RecvFile(CSCPMessage *pRequest, CSCPMessage *pMsg);
   DWORD Upgrade(CSCPMessage *pRequest);

   void ReadThread(void);
   void WriteThread(void);
   void ProcessingThread(void);

   static THREAD_RESULT THREAD_CALL ReadThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL WriteThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL ProcessingThreadStarter(void *);

public:
   CommSession(SOCKET hSocket, DWORD dwHostAddr, BOOL bMasterServer, BOOL bControlServer);
   ~CommSession();

   void Run(void);
   void Disconnect(void);

   void SendMessage(CSCPMessage *pMsg) { m_pSendQueue->Put(pMsg->CreateMessage()); }

   DWORD GetIndex(void) { return m_dwIndex; }
   void SetIndex(DWORD dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }

   time_t GetTimeStamp(void) { return m_ts; }
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

void ConsolePrintf(char *pszFormat, ...);
void DebugPrintf(char *pszFormat, ...);

void BuildFullPath(TCHAR *pszFileName, TCHAR *pszFullPath);

BOOL InitParameterList(void);
void AddParameter(char *szName, LONG (* fpHandler)(char *,char *,char *), char *pArg,
                  int iDataType, char *pszDescription);
void AddEnum(char *szName, LONG (* fpHandler)(char *,char *,NETXMS_VALUES_LIST *), char *pArg);
BOOL AddExternalParameter(char *pszCfgLine);
DWORD GetParameterValue(char *pszParam, char *pszValue);
DWORD GetEnumValue(char *pszParam, NETXMS_VALUES_LIST *pValue);
void GetParameterList(CSCPMessage *pMsg);

BOOL LoadSubAgent(char *szModuleName);
void UnloadAllSubAgents(void);
BOOL InitSubAgent(HMODULE hModule, TCHAR *pszModuleName,
                  BOOL (* SubAgentInit)(NETXMS_SUBAGENT_INFO **, TCHAR *));
BOOL ProcessCmdBySubAgent(DWORD dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponse);

BOOL AddAction(char *pszName, int iType, char *pArg, 
               LONG (*fpHandler)(TCHAR *, NETXMS_VALUES_LIST *, TCHAR *),
               char *pszSubAgent, char *pszDescription);
BOOL AddActionFromConfig(char *pszLine);
DWORD ExecAction(char *pszAction, NETXMS_VALUES_LIST *pArgs);

DWORD ExecuteCommand(char *pszCommand, NETXMS_VALUES_LIST *pArgs);

DWORD UpgradeAgent(TCHAR *pszPkgFile);

#ifdef _WIN32

void InitService(void);
void InstallService(char *execName, char *confFile);
void RemoveService(void);
void StartAgentService(void);
void StopAgentService(void);
BOOL WaitForService(DWORD dwDesiredState);
void InstallEventSource(char *path);
void RemoveEventSource(void);

char *GetPdhErrorText(DWORD dwError, char *pszBuffer, int iBufSize);

#endif


//
// Global variables
//

extern DWORD g_dwFlags;
extern char g_szLogFile[];
extern char g_szSharedSecret[];
extern char g_szConfigFile[];
extern char g_szFileStore[];
extern WORD g_wListenPort;
extern SERVER_INFO g_pServerList[];
extern DWORD g_dwServerCount;
extern time_t g_dwAgentStartTime;
extern char g_szPlatformSuffix[];
extern DWORD g_dwStartupDelay;
extern DWORD g_dwIdleTimeout;
extern DWORD g_dwMaxSessions;

extern DWORD g_dwAcceptErrors;
extern DWORD g_dwAcceptedConnections;
extern DWORD g_dwRejectedConnections;

#ifdef _WIN32
extern BOOL (__stdcall *imp_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
extern DWORD (__stdcall *imp_HrLanConnectionNameFromGuidOrPath)(LPWSTR, LPWSTR, LPWSTR, LPDWORD);
#endif   /* _WIN32 */

#endif   /* _nxagentd_h_ */
