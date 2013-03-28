/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#ifdef _WIN32
#include <aclapi.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif


//
// Version
//

#ifdef _DEBUG
#define DEBUG_SUFFIX          _T("-debug")
#else
#define DEBUG_SUFFIX          _T("")
#endif
#define AGENT_VERSION_STRING  NETXMS_VERSION_STRING DEBUG_SUFFIX


//
// Default files
//

#if defined(_WIN32)
#define AGENT_DEFAULT_CONFIG     _T("C:\\nxagentd.conf")
#define AGENT_DEFAULT_CONFIG_D   _T("C:\\nxagentd.conf.d")
#define AGENT_DEFAULT_LOG        _T("C:\\nxagentd.log")
#define AGENT_DEFAULT_FILE_STORE _T("C:\\")
#define AGENT_DEFAULT_DATA_DIR   _T("C:\\")
#elif defined(_NETWARE)
#define AGENT_DEFAULT_CONFIG     "SYS:ETC/nxagentd.conf"
#define AGENT_DEFAULT_CONFIG_D   "SYS:ETC/nxagentd.conf.d"
#define AGENT_DEFAULT_LOG        "SYS:ETC/nxagentd.log"
#define AGENT_DEFAULT_FILE_STORE "SYS:\\"
#define AGENT_DEFAULT_DATA_DIR   "SYS:ETC"
#elif defined(_IPSO)
#define AGENT_DEFAULT_CONFIG     _T("/opt/netxms/etc/nxagentd.conf")
#define AGENT_DEFAULT_CONFIG_D   _T("/opt/netxms/etc/nxagentd.conf.d")
#define AGENT_DEFAULT_LOG        _T("/opt/netxms/log/nxagentd.log")
#define AGENT_DEFAULT_FILE_STORE _T("/opt/netxms/store")
#define AGENT_DEFAULT_DATA_DIR   _T("/opt/netxms/data")
#else
#define AGENT_DEFAULT_CONFIG     _T("{search}")
#define AGENT_DEFAULT_CONFIG_D   _T("{search}")
#define AGENT_DEFAULT_LOG        _T("/var/log/nxagentd")
#define AGENT_DEFAULT_FILE_STORE _T("/tmp")
#define AGENT_DEFAULT_DATA_DIR   _T(PREFIX "/var/netxms")
#endif

#define REGISTRY_FILE_NAME       _T("registry.dat")


//
// Constants
//

#ifdef _WIN32
#define DEFAULT_AGENT_SERVICE_NAME    _T("NetXMSAgentdW32")
#define DEFAULT_AGENT_EVENT_SOURCE    _T("NetXMS Win32 Agent")
#define NXAGENTD_SYSLOG_NAME          g_windowsEventSourceName
#else
#define NXAGENTD_SYSLOG_NAME          _T("nxagentd")
#endif

#define MAX_PSUFFIX_LENGTH 32
#define MAX_SERVERS        32
#define MAX_ESA_USER_NAME  64

#define AF_DAEMON                   0x00000001
#define AF_USE_SYSLOG               0x00000002
#define AF_SUBAGENT_LOADER          0x00000004
#define AF_REQUIRE_AUTH             0x00000008
#define AF_LOG_UNRESOLVED_SYMBOLS   0x00000010
#define AF_ENABLE_ACTIONS           0x00000020
#define AF_REQUIRE_ENCRYPTION       0x00000040
#define AF_HIDE_WINDOW              0x00000080
#define AF_ENABLE_AUTOLOAD          0x00000100
#define AF_ENABLE_PROXY             0x00000200
#define AF_CENTRAL_CONFIG           0x00000400
#define AF_ENABLE_SNMP_PROXY        0x00000800
#define AF_SHUTDOWN                 0x00001000
#define AF_INTERACTIVE_SERVICE      0x00002000
#define AF_REGISTER                 0x00004000
#define AF_ENABLE_WATCHDOG          0x00008000
#define AF_CATCH_EXCEPTIONS         0x00010000
#define AF_WRITE_FULL_DUMP          0x00020000
#define AF_ARBITRARY_FILE_UPLOAD    0x00040000


#ifdef _WIN32

//
// Request types for H_MemoryInfo
//

#define MEMINFO_PHYSICAL_FREE       1
#define MEMINFO_PHYSICAL_FREE_PCT   2
#define MEMINFO_PHYSICAL_TOTAL      3
#define MEMINFO_PHYSICAL_USED       4
#define MEMINFO_PHYSICAL_USED_PCT   5
#define MEMINFO_VIRTUAL_FREE        6
#define MEMINFO_VIRTUAL_FREE_PCT    7
#define MEMINFO_VIRTUAL_TOTAL       8
#define MEMINFO_VIRTUAL_USED        9
#define MEMINFO_VIRTUAL_USED_PCT    10


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

/**
 * Pipe handle
 */
#ifdef _WIN32
#define HPIPE HANDLE
#define INVALID_PIPE_HANDLE INVALID_HANDLE_VALUE
#else
#define HPIPE SOCKET
#define INVALID_PIPE_HANDLE (-1)
#endif

/**
 * Action definition structure
 */
typedef struct
{
   TCHAR szName[MAX_PARAM_NAME];
   int iType;
   union
   {
      TCHAR *pszCmdLine;      // to TCHAR 
      struct __subagentAction
      {
         LONG (*fpHandler)(const TCHAR *, StringList *, const TCHAR *);
         const TCHAR *pArg;
         TCHAR szSubagentName[MAX_PATH];
      } sa;
   } handler;
   TCHAR szDescription[MAX_DB_STRING];
} ACTION;

/**
 * Loaded subagent information
 */
struct SUBAGENT
{
   HMODULE hModule;              // Subagent's module handle
   NETXMS_SUBAGENT_INFO *pInfo;  // Information provided by subagent
   TCHAR szName[MAX_PATH];        // Name of the module  // to TCHAR by LWX
#ifdef _NETWARE
   char szEntryPoint[256];       // Name of agent's entry point
#endif
};

/**
 * External subagent information
 */
class ExternalSubagent
{
private:
	TCHAR m_name[MAX_SUBAGENT_NAME];
	TCHAR m_user[MAX_ESA_USER_NAME];
	HPIPE m_pipe;
#ifdef _WIN32
	HANDLE m_readEvent;
#endif
	bool m_connected;
	MsgWaitQueue *m_msgQueue;
	DWORD m_requestId;
	MUTEX m_mutexPipeWrite;

	bool sendMessage(CSCPMessage *msg);
	CSCPMessage *waitForMessage(WORD code, DWORD id);
	DWORD waitForRCC(DWORD id);
	NETXMS_SUBAGENT_PARAM *getSupportedParameters(DWORD *count);
	NETXMS_SUBAGENT_LIST *getSupportedLists(DWORD *count);
	NETXMS_SUBAGENT_TABLE *getSupportedTables(DWORD *count);

public:
	ExternalSubagent(const TCHAR *name, const TCHAR *user);
	~ExternalSubagent();

	void connect(HPIPE hPipe);

	bool isConnected() { return m_connected; }
	const TCHAR *getName() { return m_name; }
	const TCHAR *getUserName() { return m_user; }

	DWORD getParameter(const TCHAR *name, TCHAR *buffer);
	DWORD getTable(const TCHAR *name, Table *value);
	DWORD getList(const TCHAR *name, StringList *value);
	void listParameters(CSCPMessage *msg, DWORD *baseId, DWORD *count);
	void listParameters(StringList *list);
	void listLists(CSCPMessage *msg, DWORD *baseId, DWORD *count);
	void listLists(StringList *list);
	void listTables(CSCPMessage *msg, DWORD *baseId, DWORD *count);
	void listTables(StringList *list);
};

/**
 * Server information
 */
struct SERVER_INFO
{
   DWORD dwIpAddr;
	DWORD dwNetMask;
   BOOL bMasterServer;
   BOOL bControlServer;
};

/**
 * Communication session
 */
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
	NXCPEncryptionContext *m_pCtx;
   time_t m_ts;               // Last activity timestamp
   SOCKET m_hProxySocket;     // Socket for proxy connection
	MUTEX m_socketWriteMutex;

	BOOL sendRawMessage(CSCP_MESSAGE *pMsg, NXCPEncryptionContext *pCtx);
   void authenticate(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void getConfig(CSCPMessage *pMsg);
   void updateConfig(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void getParameter(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void getList(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void getTable(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void action(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void recvFile(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void getLocalFile(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void getFileDetails(CSCPMessage *pRequest, CSCPMessage *pMsg);
   DWORD upgrade(CSCPMessage *pRequest);
   DWORD setupProxyConnection(CSCPMessage *pRequest);

   void readThread();
   void writeThread();
   void processingThread();
   void proxyReadThread();

   static THREAD_RESULT THREAD_CALL readThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL writeThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL processingThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL proxyReadThreadStarter(void *);

public:
   CommSession(SOCKET hSocket, DWORD dwHostAddr, BOOL bMasterServer, BOOL bControlServer);
   ~CommSession();

   void run();
   void disconnect();

   void sendMessage(CSCPMessage *pMsg) { m_pSendQueue->Put(pMsg->CreateMessage()); }
   void sendRawMessage(CSCP_MESSAGE *pMsg) { m_pSendQueue->Put(nx_memdup(pMsg, ntohl(pMsg->dwSize))); }
	bool sendFile(DWORD requestId, const TCHAR *file, long offset);

	DWORD getServerAddress() { return m_dwHostAddr; }

   DWORD getIndex() { return m_dwIndex; }
   void setIndex(DWORD dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }

   time_t getTimeStamp() { return m_ts; }
	void updateTimeStamp() { m_ts = time(NULL); }

   BOOL canAcceptTraps() { return m_bAcceptTraps; }
};

/**
 * Functions
 */
BOOL Initialize();
void Shutdown();
void Main();
void ConsolePrintf(const TCHAR *pszFormat, ...);
void DebugPrintf(DWORD dwSessionId, int level, const TCHAR *pszFormat, ...);

void BuildFullPath(TCHAR *pszFileName, TCHAR *pszFullPath);

BOOL DownloadConfig(TCHAR *pszServer);

BOOL InitParameterList();

void AddParameter(const TCHAR *szName, LONG (* fpHandler)(const TCHAR *, const TCHAR *, TCHAR *), const TCHAR *pArg,
                  int iDataType, const TCHAR *pszDescription);
void AddPushParameter(const TCHAR *name, int dataType, const TCHAR *description);
void AddList(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, StringList *), const TCHAR *arg);
void AddTable(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, Table *), const TCHAR *arg, const TCHAR *instanceColumn, const TCHAR *description);
BOOL AddExternalParameter(TCHAR *pszCfgLine, BOOL bShellExec);
DWORD GetParameterValue(DWORD dwSessionId, TCHAR *pszParam, TCHAR *pszValue);
DWORD GetListValue(DWORD dwSessionId, TCHAR *pszParam, StringList *pValue);
DWORD GetTableValue(DWORD dwSessionId, TCHAR *pszParam, Table *pValue);
void GetParameterList(CSCPMessage *pMsg);
void GetEnumList(CSCPMessage *pMsg);
void GetTableList(CSCPMessage *pMsg);
BOOL LoadSubAgent(TCHAR *szModuleName);
void UnloadAllSubAgents();
BOOL InitSubAgent(HMODULE hModule, const TCHAR *pszModuleName,
                  BOOL (* SubAgentInit)(NETXMS_SUBAGENT_INFO **, Config *),
                  const TCHAR *pszEntryPoint);
BOOL ProcessCmdBySubAgent(DWORD dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponse, void *session);
BOOL AddAction(const TCHAR *pszName, int iType, const TCHAR *pArg, 
               LONG (*fpHandler)(const TCHAR *, StringList *, const TCHAR *),
               const TCHAR *pszSubAgent, const TCHAR *pszDescription);
BOOL AddActionFromConfig(TCHAR *pszLine, BOOL bShellExec);
DWORD ExecAction(const TCHAR *pszAction, StringList *pArgs);
DWORD ExecuteCommand(TCHAR *pszCommand, StringList *pArgs, pid_t *pid);
DWORD ExecuteShellCommand(TCHAR *pszCommand, StringList *pArgs);

void StartParamProvidersPoller();
bool AddParametersProvider(const TCHAR *line);
LONG GetParameterValueFromExtProvider(const TCHAR *name, TCHAR *buffer);
void ListParametersFromExtProviders(CSCPMessage *msg, DWORD *baseId, DWORD *count);
void ListParametersFromExtProviders(StringList *list);

bool AddExternalSubagent(const TCHAR *config);
DWORD GetParameterValueFromExtSubagent(const TCHAR *name, TCHAR *buffer);
DWORD GetTableValueFromExtSubagent(const TCHAR *name, Table *value);
DWORD GetListValueFromExtSubagent(const TCHAR *name, StringList *value);
void ListParametersFromExtSubagents(CSCPMessage *msg, DWORD *baseId, DWORD *count);
void ListParametersFromExtSubagents(StringList *list);
void ListListsFromExtSubagents(CSCPMessage *msg, DWORD *baseId, DWORD *count);
void ListListsFromExtSubagents(StringList *list);
void ListTablesFromExtSubagents(CSCPMessage *msg, DWORD *baseId, DWORD *count);
void ListTablesFromExtSubagents(StringList *list);
CSCPMessage *ReadMessageFromPipe(HPIPE hPipe, HANDLE hEvent);

BOOL WaitForProcess(const TCHAR *name);

DWORD UpgradeAgent(TCHAR *pszPkgFile);

void SendTrap(DWORD dwEventCode, const TCHAR *eventName, int iNumArgs, TCHAR **ppArgList);
void SendTrap(DWORD dwEventCode, const TCHAR *eventName, const char *pszFormat, ...);
void SendTrap(DWORD dwEventCode, const TCHAR *eventName, const char *pszFormat, va_list args);

Config *OpenRegistry();
void CloseRegistry(bool modified);

void StartPushConnector();
BOOL PushData(const TCHAR *parameter, const TCHAR *value);

void StartStorageDiscoveryConnector();


#ifdef _WIN32

void InitService();
void InstallService(TCHAR *execName, TCHAR *confFile);
void RemoveService();
void StartAgentService();
void StopAgentService();
BOOL WaitForService(DWORD dwDesiredState);
void InstallEventSource(const TCHAR *path);
void RemoveEventSource();

TCHAR *GetPdhErrorText(DWORD dwError, TCHAR *pszBuffer, int iBufSize);

#endif


//
// Global variables
//

extern DWORD g_dwFlags;
extern TCHAR g_szLogFile[];
extern TCHAR g_szSharedSecret[]; 
extern TCHAR g_szConfigFile[];  
extern TCHAR g_szFileStore[];     
extern TCHAR g_szConfigServer[]; 
extern TCHAR g_szRegistrar[];  
extern TCHAR g_szListenAddress[]; 
extern TCHAR g_szConfigIncludeDir[];
extern TCHAR g_masterAgent[];
extern WORD g_wListenPort;
extern SERVER_INFO g_pServerList[];
extern DWORD g_dwServerCount;
extern time_t g_tmAgentStartTime;
extern TCHAR g_szPlatformSuffix[]; 
extern DWORD g_dwStartupDelay;
extern DWORD g_dwIdleTimeout;
extern DWORD g_dwMaxSessions;
extern DWORD g_dwExecTimeout;
extern DWORD g_dwSNMPTimeout;
extern DWORD g_debugLevel;

extern Config *g_config;

extern DWORD g_dwAcceptErrors;
extern DWORD g_dwAcceptedConnections;
extern DWORD g_dwRejectedConnections;

extern CommSession **g_pSessionList;
extern MUTEX g_hSessionListAccess;

#ifdef _WIN32
extern TCHAR g_windowsEventSourceName[];
#endif   /* _WIN32 */

#endif   /* _nxagentd_h_ */
