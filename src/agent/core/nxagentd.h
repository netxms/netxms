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

/**
 * Default files
 */
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
#define AGENT_DEFAULT_DATA_DIR   PREFIX _T("/var/netxms")
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
#define AF_ENABLE_CONTROL_CONNECTOR 0x00080000


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
#define HPIPE int
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
	UINT32 m_requestId;
	MUTEX m_mutexPipeWrite;

	bool sendMessage(CSCPMessage *msg);
	CSCPMessage *waitForMessage(WORD code, UINT32 id);
	UINT32 waitForRCC(UINT32 id);
	NETXMS_SUBAGENT_PARAM *getSupportedParameters(UINT32 *count);
	NETXMS_SUBAGENT_LIST *getSupportedLists(UINT32 *count);
	NETXMS_SUBAGENT_TABLE *getSupportedTables(UINT32 *count);

public:
	ExternalSubagent(const TCHAR *name, const TCHAR *user);
	~ExternalSubagent();

	void connect(HPIPE hPipe);

	bool isConnected() { return m_connected; }
	const TCHAR *getName() { return m_name; }
	const TCHAR *getUserName() { return m_user; }

	UINT32 getParameter(const TCHAR *name, TCHAR *buffer);
	UINT32 getTable(const TCHAR *name, Table *value);
	UINT32 getList(const TCHAR *name, StringList *value);
	void listParameters(CSCPMessage *msg, UINT32 *baseId, UINT32 *count);
	void listParameters(StringList *list);
	void listLists(CSCPMessage *msg, UINT32 *baseId, UINT32 *count);
	void listLists(StringList *list);
	void listTables(CSCPMessage *msg, UINT32 *baseId, UINT32 *count);
	void listTables(StringList *list);
   void shutdown();
};

/**
 * Server information
 */
struct SERVER_INFO
{
   UINT32 dwIpAddr;
	UINT32 dwNetMask;
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
   UINT32 m_dwHostAddr;        // IP address of connected host (network byte order)
   UINT32 m_dwIndex;
   BOOL m_bIsAuthenticated;
   BOOL m_bMasterServer;
   BOOL m_bControlServer;
   BOOL m_bProxyConnection;
   BOOL m_bAcceptTraps;
   int m_hCurrFile;
   UINT32 m_dwFileRqId;
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
   void cancelFileMonitoring(CSCPMessage *pRequest, CSCPMessage *pMsg);
   void getFileDetails(CSCPMessage *pRequest, CSCPMessage *pMsg);
   UINT32 upgrade(CSCPMessage *pRequest);
   UINT32 setupProxyConnection(CSCPMessage *pRequest);

   void readThread();
   void writeThread();
   void processingThread();
   void proxyReadThread();

   static THREAD_RESULT THREAD_CALL readThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL writeThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL processingThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL proxyReadThreadStarter(void *);

public:
   CommSession(SOCKET hSocket, UINT32 dwHostAddr, BOOL bMasterServer, BOOL bControlServer);
   ~CommSession();

   void run();
   void disconnect();

   void sendMessage(CSCPMessage *pMsg) { m_pSendQueue->Put(pMsg->CreateMessage()); }
   void sendRawMessage(CSCP_MESSAGE *pMsg) { m_pSendQueue->Put(nx_memdup(pMsg, ntohl(pMsg->dwSize))); }
	bool sendFile(UINT32 requestId, const TCHAR *file, long offset, long sizeLimit);

	UINT32 getServerAddress() { return m_dwHostAddr; }

   UINT32 getIndex() { return m_dwIndex; }
   void setIndex(UINT32 dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }

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

void ConsolePrintf(const TCHAR *pszFormat, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 1, 2)))
#endif
;

void DebugPrintf(UINT32 dwSessionId, int level, const TCHAR *pszFormat, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 3, 4)))
#endif
;

void DebugPrintfCallback(int level, const TCHAR *pszFormat, va_list args);

void BuildFullPath(TCHAR *pszFileName, TCHAR *pszFullPath);

BOOL DownloadConfig(TCHAR *pszServer);

BOOL InitParameterList();

void AddParameter(const TCHAR *szName, LONG (* fpHandler)(const TCHAR *, const TCHAR *, TCHAR *), const TCHAR *pArg,
                  int iDataType, const TCHAR *pszDescription);
void AddPushParameter(const TCHAR *name, int dataType, const TCHAR *description);
void AddList(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, StringList *), const TCHAR *arg);
void AddTable(const TCHAR *name, LONG (* handler)(const TCHAR *, const TCHAR *, Table *), const TCHAR *arg, const TCHAR *instanceColumns, const TCHAR *description);
BOOL AddExternalParameter(TCHAR *pszCfgLine, BOOL bShellExec);
UINT32 GetParameterValue(UINT32 dwSessionId, TCHAR *pszParam, TCHAR *pszValue);
UINT32 GetListValue(UINT32 dwSessionId, TCHAR *pszParam, StringList *pValue);
UINT32 GetTableValue(UINT32 dwSessionId, TCHAR *pszParam, Table *pValue);
void GetParameterList(CSCPMessage *pMsg);
void GetEnumList(CSCPMessage *pMsg);
void GetTableList(CSCPMessage *pMsg);
BOOL LoadSubAgent(TCHAR *szModuleName);
void UnloadAllSubAgents();
BOOL InitSubAgent(HMODULE hModule, const TCHAR *pszModuleName,
                  BOOL (* SubAgentInit)(NETXMS_SUBAGENT_INFO **, Config *),
                  const TCHAR *pszEntryPoint);
BOOL ProcessCmdBySubAgent(UINT32 dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponse, void *session);
BOOL AddAction(const TCHAR *pszName, int iType, const TCHAR *pArg, 
               LONG (*fpHandler)(const TCHAR *, StringList *, const TCHAR *),
               const TCHAR *pszSubAgent, const TCHAR *pszDescription);
BOOL AddActionFromConfig(TCHAR *pszLine, BOOL bShellExec);
UINT32 ExecAction(const TCHAR *pszAction, StringList *pArgs);
UINT32 ExecuteCommand(TCHAR *pszCommand, StringList *pArgs, pid_t *pid);
UINT32 ExecuteShellCommand(TCHAR *pszCommand, StringList *pArgs);

void StartParamProvidersPoller();
bool AddParametersProvider(const TCHAR *line);
LONG GetParameterValueFromExtProvider(const TCHAR *name, TCHAR *buffer);
void ListParametersFromExtProviders(CSCPMessage *msg, UINT32 *baseId, UINT32 *count);
void ListParametersFromExtProviders(StringList *list);

bool AddExternalSubagent(const TCHAR *config);
UINT32 GetParameterValueFromExtSubagent(const TCHAR *name, TCHAR *buffer);
UINT32 GetTableValueFromExtSubagent(const TCHAR *name, Table *value);
UINT32 GetListValueFromExtSubagent(const TCHAR *name, StringList *value);
void ListParametersFromExtSubagents(CSCPMessage *msg, UINT32 *baseId, UINT32 *count);
void ListParametersFromExtSubagents(StringList *list);
void ListListsFromExtSubagents(CSCPMessage *msg, UINT32 *baseId, UINT32 *count);
void ListListsFromExtSubagents(StringList *list);
void ListTablesFromExtSubagents(CSCPMessage *msg, UINT32 *baseId, UINT32 *count);
void ListTablesFromExtSubagents(StringList *list);
CSCPMessage *ReadMessageFromPipe(HPIPE hPipe, HANDLE hEvent);
bool SendMessageToPipe(HPIPE hPipe, CSCP_MESSAGE *msg);
bool SendMessageToMasterAgent(CSCPMessage *msg);
bool SendRawMessageToMasterAgent(CSCP_MESSAGE *msg);
void ShutdownExtSubagents();

void RegisterApplicationAgent(const TCHAR *name);
UINT32 GetParameterValueFromAppAgent(const TCHAR *name, TCHAR *buffer);

BOOL WaitForProcess(const TCHAR *name);

UINT32 UpgradeAgent(TCHAR *pszPkgFile);

void SendTrap(UINT32 dwEventCode, const TCHAR *eventName, int iNumArgs, TCHAR **ppArgList);
void SendTrap(UINT32 dwEventCode, const TCHAR *eventName, const char *pszFormat, ...);
void SendTrap(UINT32 dwEventCode, const TCHAR *eventName, const char *pszFormat, va_list args);
void ForwardTrap(CSCPMessage *msg);

Config *OpenRegistry();
void CloseRegistry(bool modified);

void StartPushConnector();
bool PushData(const TCHAR *parameter, const TCHAR *value, UINT32 objectId);

void StartStorageDiscoveryConnector();

void StartControlConnector();
bool SendControlMessage(CSCPMessage *msg);

/**
 * File monitoring
 */
struct MONITORED_FILE
{
   TCHAR fileName[MAX_PATH];
   int monitoringCount;
};

struct FollowData
{
   const TCHAR *pszFile;
   long offset;
	void *cbArg;
};

class MonitoredFileList
{
private:
   MUTEX m_mutex;
   ObjectArray<MONITORED_FILE>  m_monitoredFiles;
   MONITORED_FILE* m_newFile;

public:
   MonitoredFileList();
   ~MonitoredFileList();
   void addMonitoringFile(const TCHAR *fileName);
   bool checkFileMonitored(const TCHAR *fileName);
   bool removeMonitoringFile(const TCHAR *fileName);

private:
   void Lock();
   void Unlock();
};

THREAD_RESULT THREAD_CALL SendFileUpdatesOverNXCP(void *arg);

#ifdef _WIN32

void InitService();
void InstallService(TCHAR *execName, TCHAR *confFile);
void RemoveService();
void StartAgentService();
void StopAgentService();
bool WaitForService(DWORD dwDesiredState);
void InstallEventSource(const TCHAR *path);
void RemoveEventSource();

TCHAR *GetPdhErrorText(UINT32 dwError, TCHAR *pszBuffer, int iBufSize);

#endif


//
// Global variables
//

extern UINT32 g_dwFlags;
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
extern UINT32 g_dwServerCount;
extern time_t g_tmAgentStartTime;
extern TCHAR g_szPlatformSuffix[]; 
extern UINT32 g_dwStartupDelay;
extern UINT32 g_dwIdleTimeout;
extern UINT32 g_dwMaxSessions;
extern UINT32 g_dwExecTimeout;
extern UINT32 g_dwSNMPTimeout;
extern UINT32 g_debugLevel;

extern Config *g_config;

extern UINT32 g_dwAcceptErrors;
extern UINT32 g_dwAcceptedConnections;
extern UINT32 g_dwRejectedConnections;

extern CommSession **g_pSessionList;
extern MUTEX g_hSessionListAccess;
extern MonitoredFileList g_monitorFileList;

#ifdef _WIN32
extern TCHAR g_windowsEventSourceName[];
#endif   /* _WIN32 */

#endif   /* _nxagentd_h_ */
