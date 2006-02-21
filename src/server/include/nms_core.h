/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nms_core.h
**
**/

#ifndef _nms_core_h_
#define _nms_core_h_

#ifdef _WIN32
#ifdef NXCORE_EXPORTS
#define NXCORE_EXPORTABLE __declspec(dllexport)
#else
#define NXCORE_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define NXCORE_EXPORTABLE
#endif

#define LIBNXCL_NO_DECLARATIONS  1

#include <nms_common.h>

#ifndef _WIN32

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#define WSAGetLastError() (errno)

#endif   /* _WIN32 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#define _GETOPT_H_ 1    /* Prevent including getopt.h from net-snmp */
#define HAVE_SOCKLEN_T  /* Prevent defining socklen_t in net-snmp */
#endif   /* _WIN32 */

#define SHOW_FLAG_VALUE(x) _T("  %-32s = %d\n"), _T(#x), (g_dwFlags & x) ? 1 : 0


//
// Common includes
//

#include <nms_threads.h>
#include <dbdrv.h>
#include <nms_cscp.h>
#include <nms_util.h>
#include <nxsrvapi.h>
#include <nxcscpapi.h>
#include <nximage.h>
#include <nxqueue.h>
#include <nxsnmp.h>
#include <nxmodule.h>
#include <nxsl.h>


//
// Console context
//

struct __console_ctx
{
   SOCKET hSocket;
   CSCPMessage *pMsg;
};

typedef __console_ctx * CONSOLE_CTX;


//
// Server includes
//

#include "nms_dcoll.h"
#include "nms_users.h"
#include "nms_objects.h"
#include "messages.h"
#include "nms_locks.h"
#include "nms_pkg.h"
#include "nms_topo.h"
#include "nms_script.h"


//
// Common constants and macros
//

#define MAX_LINE_SIZE         4096

#define GROUP_FLAG_BIT     ((DWORD)0x80000000)

typedef void * HSNMPSESSION;

#define CHECKPOINT_SNMP_PORT  260


//
// Database syntax codes
//

#define DB_SYNTAX_GENERIC     0
#define DB_SYNTAX_MSSQL       1
#define DB_SYNTAX_MYSQL       2
#define DB_SYNTAX_PGSQL       3
#define DB_SYNTAX_ORACLE      4
#define DB_SYNTAX_SQLITE      5


//
// Unique identifier group codes
//

#define IDG_NETWORK_OBJECT    0
#define IDG_CONTAINER_CAT     1
#define IDG_EVENT             2
#define IDG_ITEM              3
#define IDG_SNMP_TRAP         4
#define IDG_IMAGE             5
#define IDG_ACTION            6
#define IDG_EVENT_GROUP       7
#define IDG_THRESHOLD         8
#define IDG_USER              9
#define IDG_USER_GROUP        10
#define IDG_ALARM             11
#define IDG_ALARM_NOTE        12
#define IDG_PACKAGE           13
#define IDG_LPP               14
#define IDG_OBJECT_TOOL       15
#define IDG_SCRIPT            16


//
// Exit codes for console commands
//

#define CMD_EXIT_CONTINUE        0
#define CMD_EXIT_CLOSE_SESSION   1
#define CMD_EXIT_SHUTDOWN        2


//
// Application flags
//

#define AF_STANDALONE                     0x00000001
#define AF_USE_EVENT_LOG                  0x00000002
#define AF_ENABLE_ACCESS_CONTROL          0x00000004
#define AF_ENABLE_EVENTS_ACCESS_CONTROL   0x00000008
#define AF_LOG_SQL_ERRORS                 0x00000010
#define AF_DELETE_EMPTY_SUBNETS           0x00000020
#define AF_ENABLE_SNMP_TRAPD              0x00000040
#define AF_ENABLE_ZONING                  0x00000080
#define AF_DEBUG_EVENTS                   0x00000100
#define AF_DEBUG_CSCP                     0x00000200
#define AF_DEBUG_DISCOVERY                0x00000400
#define AF_DEBUG_DC                       0x00000800
#define AF_DEBUG_HOUSEKEEPER              0x00001000
#define AF_DEBUG_LOCKS                    0x00002000
#define AF_DEBUG_ACTIONS                  0x00004000
#define AF_DEBUG_MISC                     0x00008000
#define AF_DEBUG_SQL                      0x00010000
#define AF_DEBUG_SNMP                     0x00020000
#define AF_DEBUG_OBJECTS                  0x00040000
#define AF_DEBUG_ALL                      0x0006FF00
#define AF_DB_LOCKED                      0x01000000
#define AF_ENABLE_MULTIPLE_DB_CONN        0x02000000
#define AF_SHUTDOWN                       0x80000000

#define IsStandalone() (g_dwFlags & AF_STANDALONE)
#define ShutdownInProgress()  (g_dwFlags & AF_SHUTDOWN)


//
// Client session flags
//

#define CSF_EVENT_DB_LOCKED      ((DWORD)0x0001)
#define CSF_EPP_LOCKED           ((DWORD)0x0002)
#define CSF_PACKAGE_DB_LOCKED    ((DWORD)0x0004)
#define CSF_USER_DB_LOCKED       ((DWORD)0x0008)
#define CSF_EPP_UPLOAD           ((DWORD)0x0010)
#define CSF_ACTION_DB_LOCKED     ((DWORD)0x0020)
#define CSF_TRAP_CFG_LOCKED      ((DWORD)0x0040)
#define CSF_AUTHENTICATED        ((DWORD)0x0080)
#define CSF_OBJECT_TOOLS_LOCKED  ((DWORD)0x0100)


//
// Client session states
//

#define SESSION_STATE_INIT       0
#define SESSION_STATE_IDLE       1
#define SESSION_STATE_PROCESSING 2


//
// Information categories for UPDATE_INFO structure
//

#define INFO_CAT_EVENT           1
#define INFO_CAT_OBJECT_CHANGE   2
#define INFO_CAT_ALARM           3
#define INFO_CAT_ACTION          4
#define INFO_CAT_SYSLOG_MSG      5


//
// Event handling subsystem definitions
//

#include "nms_events.h"
#include "nms_actions.h"
#include "nms_alarm.h"


//
// New node information
//

typedef struct
{
   DWORD dwIpAddr;
   DWORD dwNetMask;
   DWORD dwFlags;
} NEW_NODE;


//
// New node flags
//

#define NNF_IS_SNMP     0x0001
#define NNF_IS_AGENT    0x0002
#define NNF_IS_ROUTER   0x0004
#define NNF_IS_BRIDGE   0x0008
#define NNF_IS_PRINTER  0x0010


//
// Node information for autodiscovery filter
//

typedef struct
{
   DWORD dwIpAddr;
   DWORD dwNetMask;
   DWORD dwSubnetAddr;
   DWORD dwFlags;
   int nSNMPVersion;
   char szObjectId[MAX_OID_LEN * 4];    // SNMP OID
   char szAgentVersion[MAX_AGENT_VERSION_LEN];
   char szPlatform[MAX_PLATFORM_NAME_LEN];
} DISCOVERY_FILTER_DATA;


//
// Data update structure for client sessions
//

typedef struct
{
   DWORD dwCategory;    // Data category - event, network object, etc.
   DWORD dwCode;        // Data-specific update code
   void *pData;         // Pointer to data block
} UPDATE_INFO;


//
// Client session
//

class ClientSession
{
private:
   SOCKET m_hSocket;
   Queue *m_pSendQueue;
   Queue *m_pMessageQueue;
   Queue *m_pUpdateQueue;
   DWORD m_dwIndex;
   int m_iState;
   WORD m_wCurrentCmd;
   DWORD m_dwUserId;
   DWORD m_dwSystemAccess;    // User's system access rights
   DWORD m_dwFlags;           // Session flags
   CSCP_BUFFER *m_pMsgBuffer;
   CSCP_ENCRYPTION_CONTEXT *m_pCtx;
   THREAD m_hWriteThread;
   THREAD m_hProcessingThread;
   THREAD m_hUpdateThread;
   MUTEX m_mutexSendEvents;
   MUTEX m_mutexSendSyslog;
   MUTEX m_mutexSendObjects;
   MUTEX m_mutexSendAlarms;
   MUTEX m_mutexSendActions;
   MUTEX m_mutexPollerInit;
   DWORD m_dwHostAddr;        // IP address of connected host (network byte order)
   TCHAR m_szUserName[256];   // String in form login_name@host
   TCHAR m_szClientInfo[96];  // Client app info string
   DWORD m_dwOpenDCIListSize; // Number of open DCI lists
   DWORD *m_pOpenDCIList;     // List of nodes with DCI lists open
   DWORD m_dwNumRecordsToUpload; // Number of records to be uploaded
   DWORD m_dwRecordsUploaded;
   EPRule **m_ppEPPRuleList;   // List of loaded EPP rules
   int m_hCurrFile;
   DWORD m_dwFileRqId;
   DWORD m_dwUploadCommand;
   DWORD m_dwUploadData;
   TCHAR m_szCurrFileName[MAX_PATH];
   DWORD m_dwRefCount;
   DWORD m_dwEncryptionRqId;
   DWORD m_dwEncryptionResult;
   CONDITION m_condEncryptionSetup;
   DWORD m_dwActiveChannels;     // Active data channels

   static THREAD_RESULT THREAD_CALL ReadThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL WriteThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL ProcessingThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL UpdateThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL PollerThreadStarter(void *);

   void ReadThread(void);
   void WriteThread(void);
   void ProcessingThread(void);
   void UpdateThread(void);
   void PollerThread(Node *pNode, int iPollType, DWORD dwRqId);

   BOOL CheckSysAccessRights(DWORD dwRequiredAccess) 
   { 
      return m_dwUserId == 0 ? TRUE : 
         ((dwRequiredAccess & m_dwSystemAccess) ? TRUE : FALSE);
   }

   void SetupEncryption(DWORD dwRqId);
   void OnFileUpload(BOOL bSuccess);
   void DebugPrintf(char *szFormat, ...);
   void SendServerInfo(DWORD dwRqId);
   void Login(CSCPMessage *pRequest);
   void SendAllObjects(CSCPMessage *pRequest);
   void SendAllEvents(CSCPMessage *pRequest);
   void SendAllConfigVars(DWORD dwRqId);
   void SetConfigVariable(CSCPMessage *pRequest);
   void DeleteConfigVariable(CSCPMessage *pRequest);
   void SendUserDB(DWORD dwRqId);
   void SendAllAlarms(DWORD dwRqId, BOOL bIncludeAck);
   void CreateUser(CSCPMessage *pRequest);
   void UpdateUser(CSCPMessage *pRequest);
   void DeleteUser(CSCPMessage *pRequest);
   void SetPassword(CSCPMessage *pRequest);
   void LockUserDB(DWORD dwRqId, BOOL bLock);
   void SendEventDB(DWORD dwRqId);
   void LockEventDB(DWORD dwRqId);
   void UnlockEventDB(DWORD dwRqId);
   void SetEventInfo(CSCPMessage *pRequest);
   void DeleteEventTemplate(CSCPMessage *pRequest);
   void GenerateEventCode(DWORD dwRqId);
   void ModifyObject(CSCPMessage *pRequest);
   void ChangeObjectMgmtStatus(CSCPMessage *pRequest);
   void OpenNodeDCIList(CSCPMessage *pRequest);
   void CloseNodeDCIList(CSCPMessage *pRequest);
   void ModifyNodeDCI(CSCPMessage *pRequest);
   void CopyDCI(CSCPMessage *pRequest);
   void ApplyTemplate(CSCPMessage *pRequest);
   void GetCollectedData(CSCPMessage *pRequest);
   void ChangeDCIStatus(CSCPMessage *pRequest);
   void SendLastValues(CSCPMessage *pRequest);
   void OpenEPP(DWORD dwRqId);
   void CloseEPP(DWORD dwRqId);
   void SaveEPP(CSCPMessage *pRequest);
   void ProcessEPPRecord(CSCPMessage *pRequest);
   void SendMIBTimestamp(DWORD dwRqId);
   void SendMIB(DWORD dwRqId);
   void CreateObject(CSCPMessage *pRequest);
   void ChangeObjectBinding(CSCPMessage *pRequest, BOOL bBind);
   void DeleteObject(CSCPMessage *pRequest);
   void AcknowlegeAlarm(CSCPMessage *pRequest);
   void DeleteAlarm(CSCPMessage *pRequest);
   void CreateAction(CSCPMessage *pRequest);
   void UpdateAction(CSCPMessage *pRequest);
   void DeleteAction(CSCPMessage *pRequest);
   void LockActionDB(DWORD dwRqId, BOOL bLock);
   void SendAllActions(DWORD dwRqId);
   void SendContainerCategories(DWORD dwRqId);
   void ForcedNodePoll(CSCPMessage *pRequest);
   void OnTrap(CSCPMessage *pRequest);
   void OnWakeUpNode(CSCPMessage *pRequest);
   void QueryParameter(CSCPMessage *pRequest);
   void EditTrap(int iOperation, CSCPMessage *pRequest);
   void LockTrapCfg(DWORD dwRqId, BOOL bLock);
   void SendAllTraps(DWORD dwRqId);
   void LockPackageDB(DWORD dwRqId, BOOL bLock);
   void SendAllPackages(DWORD dwRqId);
   void InstallPackage(CSCPMessage *pRequest);
   void RemovePackage(CSCPMessage *pRequest);
   void DeployPackage(CSCPMessage *pRequest);
   void SendParametersList(CSCPMessage *pRequest);
   void GetUserVariable(CSCPMessage *pRequest);
   void SetUserVariable(CSCPMessage *pRequest);
   void EnumUserVariables(CSCPMessage *pRequest);
   void DeleteUserVariable(CSCPMessage *pRequest);
   void ChangeObjectIP(CSCPMessage *pRequest);
   void GetAgentConfig(CSCPMessage *pRequest);
   void UpdateAgentConfig(CSCPMessage *pRequest);
   void ExecuteAction(CSCPMessage *pRequest);
   void SendObjectTools(DWORD dwRqId);
   void SendObjectToolDetails(CSCPMessage *pRequest);
   void UpdateObjectTool(CSCPMessage *pRequest);
   void DeleteObjectTool(CSCPMessage *pRequest);
   void GenerateObjectToolId(DWORD dwRqId);
   void ExecTableTool(CSCPMessage *pRequest);
   void LockObjectTools(DWORD dwRqId, BOOL bLock);
   void ChangeSubscription(CSCPMessage *pRequest);
   void SendSyslog(CSCPMessage *pRequest);
   void SendLogPoliciesList(DWORD dwRqId);
   void CreateNewLPPID(DWORD dwRqId);
   void OpenLPP(CSCPMessage *pRequest);
   void SendServerStats(DWORD dwRqId);
   void SendScriptList(DWORD dwRqId);
   void SendScript(CSCPMessage *pRequest);
   void UpdateScript(CSCPMessage *pRequest);
   void RenameScript(CSCPMessage *pRequest);
   void DeleteScript(CSCPMessage *pRequest);
   void SendSessionList(DWORD dwRqId);
   void KillSession(CSCPMessage *pRequest);

public:
   ClientSession(SOCKET hSocket, DWORD dwHostAddr);
   ~ClientSession();

   void IncRefCount(void) { m_dwRefCount++; }
   void DecRefCount(void) { if (m_dwRefCount > 0) m_dwRefCount--; }

   void Run(void);

   void SendMessage(CSCPMessage *pMsg) { m_pSendQueue->Put(pMsg->CreateMessage()); }
   void SendPollerMsg(DWORD dwRqId, TCHAR *pszMsg);

   DWORD GetIndex(void) { return m_dwIndex; }
   void SetIndex(DWORD dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }
   int GetState(void) { return m_iState; }
   const TCHAR *GetUserName(void) { return m_szUserName; }
   const TCHAR *GetClientInfo(void) { return m_szClientInfo; }
   DWORD GetUserId(void) { return m_dwUserId; }
   BOOL IsAuthenticated(void) { return (m_dwFlags & CSF_AUTHENTICATED) ? TRUE : FALSE; }
   WORD GetCurrentCmd(void) { return m_wCurrentCmd; }
   int GetCipher(void) { return (m_pCtx == NULL) ? -1 : m_pCtx->nCipher; }

   void Kill(void);
   void Notify(DWORD dwCode, DWORD dwData = 0);

   void OnNewEvent(Event *pEvent);
   void OnSyslogMessage(NX_LOG_RECORD *pRec);
   void OnObjectChange(NetObj *pObject);
   void OnUserDBUpdate(int iCode, DWORD dwUserId, NMS_USER *pUser, NMS_USER_GROUP *pGroup);
   void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);
   void OnActionDBUpdate(DWORD dwCode, NXC_ACTION *pAction);
};


//
// Functions
//

BOOL NXCORE_EXPORTABLE ConfigReadStr(char *szVar, char *szBuffer, int iBufSize, const char *szDefault);
int NXCORE_EXPORTABLE ConfigReadInt(char *szVar, int iDefault);
DWORD NXCORE_EXPORTABLE ConfigReadULong(char *szVar, DWORD dwDefault);
BOOL NXCORE_EXPORTABLE ConfigReadByteArray(char *pszVar, int *pnArray, int nSize, int nDefault);
BOOL NXCORE_EXPORTABLE ConfigWriteStr(char *szVar, char *szValue, BOOL bCreate);
BOOL NXCORE_EXPORTABLE ConfigWriteInt(char *szVar, int iValue, BOOL bCreate);
BOOL NXCORE_EXPORTABLE ConfigWriteULong(char *szVar, DWORD dwValue, BOOL bCreate);
BOOL NXCORE_EXPORTABLE ConfigWriteByteArray(char *pszVar, int *pnArray, int nSize, BOOL bCreate);

BOOL NXCORE_EXPORTABLE LoadConfig(void);

void NXCORE_EXPORTABLE Shutdown(void);
BOOL NXCORE_EXPORTABLE Initialize(void);
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *);
void NXCORE_EXPORTABLE ShutdownDB(void);
void InitiateShutdown(void);

BOOL NXCORE_EXPORTABLE SleepAndCheckForShutdown(int iSeconds);

void ConsolePrintf(CONSOLE_CTX pCtx, char *pszFormat, ...);
int ProcessConsoleCommand(char *pszCmdLine, CONSOLE_CTX pCtx);

void SaveObjects(DB_HANDLE hdb);

void QueueSQLRequest(char *szQuery);
void StartDBWriter(void);
void StopDBWriter(void);

void SnmpInit(void);
DWORD SnmpNewRequestId(void);
DWORD SnmpGet(DWORD dwVersion, DWORD dwAddr, WORD wPort, const char *szCommunity,
              const char *szOidStr, const DWORD *oidBinary, DWORD dwOidLen, void *pValue,
              DWORD dwBufferSize, BOOL bVerbose, BOOL bStringResult);
DWORD SnmpEnumerate(DWORD dwVersion, DWORD dwAddr, WORD wPort, const char *szCommunity,
                    const char *szRootOid, DWORD (* pHandler)(DWORD, DWORD, WORD,
                    const char *, SNMP_Variable *, SNMP_Transport *, void *),
                    void *pUserArg, BOOL bVerbose);
void StrToMac(char *pszStr, BYTE *pBuffer);
DWORD OidToType(TCHAR *pszOid, DWORD *pdwFlags);

void InitLocalNetInfo(void);

ARP_CACHE *GetLocalArpCache(void);
ARP_CACHE *SnmpGetArpCache(DWORD dwVersion, DWORD dwAddr, WORD wPort, const char *szCommunity);

INTERFACE_LIST *SnmpGetInterfaceList(DWORD dwVersion, DWORD dwAddr, WORD wPort,
                                     const char *szCommunity, DWORD dwNodeType);
INTERFACE_LIST *GetLocalInterfaceList(void);
void CleanInterfaceList(INTERFACE_LIST *pIfList);
int SnmpGetInterfaceStatus(DWORD dwVersion, DWORD dwNodeAddr, WORD wPort, char *pszCommunity, DWORD dwIfIndex);

ROUTING_TABLE *SnmpGetRoutingTable(DWORD dwVersion, DWORD dwAddr, WORD wPort, const char *szCommunity);

void WatchdogInit(void);
DWORD WatchdogAddThread(char *szName, time_t tNotifyInterval);
void WatchdogNotify(DWORD dwId);
void WatchdogPrintStatus(CONSOLE_CTX pCtx);

void CheckForMgmtNode(void);
NetObj *PollNewNode(DWORD dwIpAddr, DWORD dwNetMask, DWORD dwFlags, TCHAR *pszName);

void NXCORE_EXPORTABLE EnumerateClientSessions(void (*pHandler)(ClientSession *, void *), void *pArg);
void NXCORE_EXPORTABLE NotifyClientSessions(DWORD dwCode, DWORD dwData);
int GetSessionCount(void);

void GetSysInfoStr(char *pszBuffer);
DWORD GetLocalIpAddr(void);

BOOL ExecCommand(char *pszCommand);
BOOL SendMagicPacket(DWORD dwIpAddr, BYTE *pbMacAddr, int iNumPackets);

BOOL InitIdTable(void);
DWORD CreateUniqueId(int iGroup);
QWORD CreateUniqueEventId(void);

void UpdateImageHashes(void);
void SendImageCatalogue(ClientSession *pSession, DWORD dwRqId, WORD wFormat);
void SendImageFile(ClientSession *pSession, DWORD dwRqId, DWORD dwImageId, WORD wFormat);
void SendDefaultImageList(ClientSession *pSession, DWORD dwRqId);

void InitMailer(void);
void ShutdownMailer(void);
void NXCORE_EXPORTABLE PostMail(char *pszRcpt, char *pszSubject, char *pszText);

void InitSMSSender(void);
void ShutdownSMSSender(void);
void NXCORE_EXPORTABLE PostSMS(TCHAR *pszRcpt, TCHAR *pszText);

void GetAccelarVLANIfList(DWORD dwVersion, DWORD dwIpAddr, WORD wPort,
                          const TCHAR *pszCommunity, INTERFACE_LIST *pIfList);

void InitTraps(void);
void SendTrapsToClient(ClientSession *pSession, DWORD dwRqId);
DWORD CreateNewTrap(DWORD *pdwTrapId);
DWORD UpdateTrapFromMsg(CSCPMessage *pMsg);
DWORD DeleteTrap(DWORD dwId);

BOOL IsTableTool(DWORD dwToolId);
BOOL CheckObjectToolAccess(DWORD dwToolId, DWORD dwUserId);
DWORD ExecuteTableTool(DWORD dwToolId, Node *pNode, DWORD dwRqId, ClientSession *pSession);
DWORD DeleteObjectToolFromDB(DWORD dwToolId);
DWORD UpdateObjectToolFromMessage(CSCPMessage *pMsg);

void CreateMessageFromSyslogMsg(CSCPMessage *pMsg, NX_LOG_RECORD *pRec);

#ifdef _WIN32

char NXCORE_EXPORTABLE *GetSystemErrorText(DWORD error);

#else

THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *);

#endif   /* _WIN32 */

void DbgTestMutex(MUTEX hMutex, TCHAR *szName, CONSOLE_CTX pCtx);
void DbgTestRWLock(RWLOCK hLock, TCHAR *szName, CONSOLE_CTX pCtx);
void DbgPrintf(DWORD dwFlags, TCHAR *szFormat, ...);
void DumpSessions(CONSOLE_CTX pCtx);
void ShowPollerState(CONSOLE_CTX pCtx);
void SetPollerInfo(int nIdx, char *pszMsg);
void ShowServerStats(CONSOLE_CTX pCtx);
void DumpProcess(void);


//
// Global variables
//

extern DWORD NXCORE_EXPORTABLE g_dwFlags;
extern char NXCORE_EXPORTABLE g_szConfigFile[];
extern char NXCORE_EXPORTABLE g_szLogFile[];
#ifndef _WIN32
extern char NXCORE_EXPORTABLE g_szPIDFile[];
#endif
extern char g_szDataDir[];
extern QWORD g_qwServerId;
extern RSA *g_pServerKey;
extern DWORD g_dwPingSize;
extern time_t g_tServerStartTime;

extern DB_HANDLE g_hCoreDB;
extern Queue *g_pLazyRequestQueue;

extern char *g_pszStatusName[];

extern DWORD g_dwDBSyntax;

#endif   /* _nms_core_h_ */
