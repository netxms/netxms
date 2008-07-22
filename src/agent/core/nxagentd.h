/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: nxagentd.h
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
#include <stdarg.h>
#include <nxqueue.h>
#include <nxlog.h>
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
#elif defined(_IPSO)
#define AGENT_DEFAULT_CONFIG     "/opt/netxms/etc/nxagentd.conf"
#define AGENT_DEFAULT_LOG        "/opt/netxms/log/nxagentd.log"
#define AGENT_DEFAULT_FILE_STORE "/opt/netxms/store"
#else
#define AGENT_DEFAULT_CONFIG     "{search}"
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
#define AF_ENABLE_AUTOLOAD          0x0100
#define AF_ENABLE_PROXY             0x0200
#define AF_CENTRAL_CONFIG           0x0400
#define AF_ENABLE_SNMP_PROXY			0x0800
#define AF_SHUTDOWN                 0x1000
#define AF_RUNNING_ON_NT4           0x2000


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
// Request types for H_DirInfo
//

#define DIRINFO_FILE_COUNT       1
#define DIRINFO_FILE_SIZE        2


//
// Request types for H_FileTime
//
 
#define FILETIME_ATIME           1
#define FILETIME_MTIME           2
#define FILETIME_CTIME           3


//
// Action types
//

#define AGENT_ACTION_EXEC        1
#define AGENT_ACTION_SUBAGENT    2
#define AGENT_ACTION_SHELLEXEC	3


//
// Action definition structure
//

typedef struct
{
   TCHAR szName[MAX_PARAM_NAME];
   int iType;
   union
   {
      char *pszCmdLine;
      struct __subagentAction
      {
         LONG (*fpHandler)(const TCHAR *, NETXMS_VALUES_LIST *, const TCHAR *);
         const TCHAR *pArg;
         TCHAR szSubagentName[MAX_PATH];
      } sa;
   } handler;
   TCHAR szDescription[MAX_DB_STRING];
} ACTION;


//
// Loaded subagent information
//

struct SUBAGENT
{
   HMODULE hModule;              // Subagent's module handle
   NETXMS_SUBAGENT_INFO *pInfo;  // Information provided by subagent
   char szName[MAX_PATH];        // Name of the module
#ifdef _NETWARE
   char szEntryPoint[256];       // Name of agent's entry point
#endif
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
   THREAD m_hProxyReadThread;
   DWORD m_dwHostAddr;        // IP address of connected host (network byte order)
   DWORD m_dwIndex;
   BOOL m_bIsAuthenticated;
   BOOL m_bMasterServer;
   BOOL m_bControlServer;
   BOOL m_bProxyConnection;
   BOOL m_bAcceptTraps;
   int m_hCurrFile;
   DWORD m_dwFileRqId;
   CSCP_ENCRYPTION_CONTEXT *m_pCtx;
   time_t m_ts;               // Last command timestamp
   SOCKET m_hProxySocket;     // Socket for proxy connection

   BOOL SendRawMessage(CSCP_MESSAGE *pMsg, CSCP_ENCRYPTION_CONTEXT *pCtx);
   void Authenticate(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void GetConfig(CSCPMessage *pMsg);
   void UpdateConfig(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void GetParameter(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void GetList(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void Action(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void RecvFile(CSCPMessage *pRequest, CSCPMessage *pMsg);
   DWORD Upgrade(CSCPMessage *pRequest);
   DWORD ApplyLogPolicy(CSCPMessage *pRequest);
   DWORD SetupProxyConnection(CSCPMessage *pRequest);

   void ReadThread(void);
   void WriteThread(void);
   void ProcessingThread(void);
   void ProxyReadThread(void);

   static THREAD_RESULT THREAD_CALL ReadThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL WriteThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL ProcessingThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL ProxyReadThreadStarter(void *);

public:
   CommSession(SOCKET hSocket, DWORD dwHostAddr, BOOL bMasterServer, BOOL bControlServer);
   ~CommSession();

   void Run(void);
   void Disconnect(void);

   void SendMessage(CSCPMessage *pMsg) { m_pSendQueue->Put(pMsg->CreateMessage()); }
   void SendRawMessage(CSCP_MESSAGE *pMsg) { m_pSendQueue->Put(nx_memdup(pMsg, ntohl(pMsg->dwSize))); }

   DWORD GetIndex(void) { return m_dwIndex; }
   void SetIndex(DWORD dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }

   time_t GetTimeStamp(void) { return m_ts; }

   BOOL AcceptTraps(void) { return m_bAcceptTraps; }
};


//
// Functions
//

BOOL Initialize(void);
void Shutdown(void);
void Main(void);

void WriteLog(DWORD msg, WORD wType, const char *format, ...);
void InitLog(void);
void CloseLog(void);

void ConsolePrintf(const char *pszFormat, ...);
void DebugPrintf(DWORD dwSessionId, const char *pszFormat, ...);

void BuildFullPath(TCHAR *pszFileName, TCHAR *pszFullPath);

BOOL DownloadConfig(TCHAR *pszServer);

BOOL InitParameterList(void);
void AddParameter(const char *szName, LONG (* fpHandler)(const char *, const char *, char *), const char *pArg,
                  int iDataType, const char *pszDescription);
void AddEnum(const char *szName, LONG (* fpHandler)(const char *, const char *, NETXMS_VALUES_LIST *), const char *pArg);
BOOL AddExternalParameter(char *pszCfgLine, BOOL bShellExec);
DWORD GetParameterValue(DWORD dwSessionId, char *pszParam, char *pszValue);
DWORD GetEnumValue(DWORD dwSessionId, char *pszParam, NETXMS_VALUES_LIST *pValue);
void GetParameterList(CSCPMessage *pMsg);

BOOL LoadSubAgent(char *szModuleName);
void UnloadAllSubAgents(void);
BOOL InitSubAgent(HMODULE hModule, TCHAR *pszModuleName,
                  BOOL (* SubAgentInit)(NETXMS_SUBAGENT_INFO **, TCHAR *),
                  TCHAR *pszEntryPoint);
BOOL ProcessCmdBySubAgent(DWORD dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponse,
                          SOCKET sock, CSCP_ENCRYPTION_CONTEXT *ctx);

BOOL AddAction(const char *pszName, int iType, const char *pArg, 
               LONG (*fpHandler)(const TCHAR *, NETXMS_VALUES_LIST *, const TCHAR *),
               const char *pszSubAgent, const char *pszDescription);
BOOL AddActionFromConfig(char *pszLine, BOOL bShellExec);
DWORD ExecAction(char *pszAction, NETXMS_VALUES_LIST *pArgs);

DWORD ExecuteCommand(char *pszCommand, NETXMS_VALUES_LIST *pArgs);
DWORD ExecuteShellCommand(char *pszCommand, NETXMS_VALUES_LIST *pArgs);

DWORD UpgradeAgent(TCHAR *pszPkgFile);

DWORD InstallLogPolicy(NX_LPP *pPolicy);

void SendTrap(DWORD dwEventCode, int iNumArgs, TCHAR **ppArgList);
void SendTrap(DWORD dwEventCode, const char *pszFormat, ...);
void SendTrap(DWORD dwEventCode, const char *pszFormat, va_list args);

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
extern char g_szListenAddress[];
extern WORD g_wListenPort;
extern SERVER_INFO g_pServerList[];
extern DWORD g_dwServerCount;
extern time_t g_tmAgentStartTime;
extern char g_szPlatformSuffix[];
extern DWORD g_dwStartupDelay;
extern DWORD g_dwIdleTimeout;
extern DWORD g_dwMaxSessions;
extern DWORD g_dwExecTimeout;
extern DWORD g_dwSNMPTimeout;

extern DWORD g_dwAcceptErrors;
extern DWORD g_dwAcceptedConnections;
extern DWORD g_dwRejectedConnections;

extern CommSession **g_pSessionList;
extern MUTEX g_hSessionListAccess;

#ifdef _WIN32
extern BOOL (__stdcall *imp_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
extern DWORD (__stdcall *imp_HrLanConnectionNameFromGuidOrPath)(LPWSTR, LPWSTR, LPWSTR, LPDWORD);
#endif   /* _WIN32 */

#endif   /* _nxagentd_h_ */
