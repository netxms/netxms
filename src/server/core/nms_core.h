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

#include <nms_common.h>

#ifndef _WIN32

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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <openssl/ssl.h>

#include <nms_threads.h>
#include <dbdrv.h>
#include <nms_cscp.h>
#include <nms_util.h>
#include "nms_users.h"
#include "nms_dcoll.h"
#include "nms_objects.h"
#include "messages.h"
#include "nms_locks.h"


//
// Common constants and macros
//

#ifdef _WIN32
#define DEFAULT_SHELL         "cmd.exe"
#define DEFAULT_CONFIG_FILE   "C:\\NetXMS.conf"
#define DEFAULT_LOG_FILE      "C:\\NetXMS.log"
#define IsStandalone() (g_dwFlags & AF_STANDALONE)
#else    /* _WIN32 */
#define DEFAULT_SHELL         "/bin/sh"
#define DEFAULT_CONFIG_FILE   "/etc/netxms.conf"
#define DEFAULT_LOG_FILE      "/var/log/netxms.log"
#define IsStandalone() (1)
#endif   /* _WIN32 */

#define MAX_DB_LOGIN          64
#define MAX_DB_PASSWORD       64
#define MAX_DB_NAME           32
#define MAX_LINE_SIZE         4096

#define UNLOCKED           ((DWORD)0xFFFFFFFF)

#define GROUP_FLAG_BIT     ((DWORD)0x80000000)

#define CHECK_NULL(x)      ((x) == NULL ? ((char *)"(null)") : (x))


//
// Event log severity codes (UNIX only)
//

#ifndef _WIN32
#define EVENTLOG_INFORMATION_TYPE   0
#define EVENTLOG_WARNING_TYPE       1
#define EVENTLOG_ERROR_TYPE         2
#endif   /* _WIN32 */


//
// Unique identifier group codes
//

#define IDG_NETWORK_OBJECT    0
#define IDG_NETOBJ_GROUP      1
#define IDG_EVENT             2
#define IDG_ITEM              3
#define IDG_DCT               4
#define IDG_DCT_ITEM          5
#define IDG_ACTION            6
#define IDG_EVENT_GROUP       7
#define IDG_THRESHOLD         8


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
#define AF_DEBUG_EVENTS                   0x00000100
#define AF_DEBUG_CSCP                     0x00000200
#define AF_DEBUG_DISCOVERY                0x00000400
#define AF_DEBUG_DC                       0x00000800
#define AF_DEBUG_HOUSEKEEPER              0x00001000
#define AF_DEBUG_LOCKS                    0x00002000
#define AF_DEBUG_ACTIONS                  0x00004000
#define AF_DEBUG_ALL                      0x0000FF00
#define AF_SHUTDOWN                       0x80000000

#define ShutdownInProgress()  (g_dwFlags & AF_SHUTDOWN)


//
// Win32 service constants
//

#ifdef _WIN32

#define CORE_SERVICE_NAME  "NetXMSCore"
#define CORE_EVENT_SOURCE  "NetXMSCore"

#endif   /* _WIN32 */


//
// Client session states
//

#define STATE_CLOSED          0
#define STATE_CONNECTED       1
#define STATE_AUTHENTICATED   2


//
// Client session flags
//

#define CSF_EVENT_DB_LOCKED      ((DWORD)0x0001)
#define CSF_EPP_LOCKED           ((DWORD)0x0002)
#define CSF_EVENT_DB_MODIFIED    ((DWORD)0x0004)


//
// Information categories for UPDATE_INFO structure
//

#define INFO_CAT_EVENT           1
#define INFO_CAT_OBJECT_CHANGE   2


//
// Event handling subsystem definitions
//

#include "nms_events.h"
#include "nms_actions.h"


//
// Agent connection
//

class AgentConnection
{
private:
   DWORD m_dwAddr;
   int m_iAuthMethod;
   char m_szSecret[MAX_SECRET_LENGTH];
   time_t m_tLastCommandTime;
   SOCKET m_hSocket;
   WORD m_wPort;
   DWORD m_dwNumDataLines;
   char **m_ppDataLines;
   char m_szNetBuffer[MAX_LINE_SIZE * 2];

   int RecvLine(int iBufSize, char *szBuffer);
   DWORD ExecuteCommand(char *szCmd, BOOL bExpectData = FALSE, BOOL bMultiLineData = FALSE);
   void AddDataLine(char *szLine);
   void DestroyResultData(void);

public:
   AgentConnection();
   AgentConnection(DWORD dwAddr, WORD wPort = AGENT_LISTEN_PORT, int iAuthMethod = AUTH_NONE, char *szSecret = NULL);
   ~AgentConnection();

   BOOL Connect(BOOL bVerbose = FALSE);
   void Disconnect(void);

   ARP_CACHE *GetArpCache(void);
   INTERFACE_LIST *GetInterfaceList(void);
   DWORD Nop(void) { return ExecuteCommand("NOP"); }
   DWORD GetParameter(const char *szParam, DWORD dwBufSize, char *szBuffer);
};


//
// Data update structure for client sessions
//

typedef struct
{
   DWORD dwCategory;    // Data category - event, network object, etc.
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
   DWORD m_dwUserId;
   DWORD m_dwSystemAccess;    // User's system access rights
   DWORD m_dwFlags;           // Session flags
   CSCP_BUFFER *m_pMsgBuffer;
   CONDITION m_hCondWriteThreadStopped;
   CONDITION m_hCondProcessingThreadStopped;
   CONDITION m_hCondUpdateThreadStopped;
   MUTEX m_hMutexSendEvents;
   MUTEX m_hMutexSendObjects;
   DWORD m_dwHostAddr;        // IP address of connected host (network byte order)
   char m_szUserName[256];    // String in form login_name@host

   BOOL CheckSysAccessRights(DWORD dwRequiredAccess) 
   { 
      return m_dwUserId == 0 ? TRUE : 
         ((dwRequiredAccess & m_dwSystemAccess) ? TRUE : FALSE);
   }

   void DebugPrintf(char *szFormat, ...);
   void SendAllObjects(void);
   void SendAllEvents(void);
   void SendAllConfigVars(void);
   void SetConfigVariable(CSCPMessage *pMsg);
   void SendEventDB(DWORD dwRqId);
   void SetEventInfo(CSCPMessage *pMsg);

public:
   ClientSession(SOCKET hSocket, DWORD dwHostAddr);
   ~ClientSession();

   void SendMessage(CSCPMessage *pMsg);
   void DispatchMessage(CSCP_MESSAGE *pMsg);

   void ReadThread(void);
   void WriteThread(void);
   void ProcessingThread(void);
   void UpdateThread(void);

   DWORD GetIndex(void) { return m_dwIndex; }
   void SetIndex(DWORD dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }
   int GetState(void) { return m_iState; }
   const char *GetUserName(void) { return m_szUserName; }

   void Kill(void);
   void Notify(DWORD dwCode);

   void OnNewEvent(Event *pEvent);
   void OnObjectChange(NetObj *pObject);
};


//
// Functions
//

BOOL ConfigReadStr(char *szVar, char *szBuffer, int iBufSize, char *szDefault);
int ConfigReadInt(char *szVar, int iDefault);
DWORD ConfigReadULong(char *szVar, DWORD dwDefault);
BOOL ConfigWriteStr(char *szVar, char *szValue, BOOL bCreate);
BOOL ConfigWriteInt(char *szVar, int iValue, BOOL bCreate);
BOOL ConfigWriteULong(char *szVar, DWORD dwValue, BOOL bCreate);

void InitLog(void);
void CloseLog(void);
void WriteLog(DWORD msg, WORD wType, char *format, ...);

BOOL InitLocks(DWORD *pdwIpAddr, char *pszInfo);
BOOL LockComponent(DWORD dwId, DWORD dwLockBy, char *pszOwnerInfo, DWORD *pdwCurrentOwner, char *pszCurrentOwnerInfo);
void UnlockComponent(DWORD dwId);

BOOL ParseCommandLine(int argc, char *argv[]);
BOOL LoadConfig(void);

void Shutdown(void);
BOOL Initialize(void);
void Main(void);

BOOL SleepAndCheckForShutdown(int iSeconds);

void SaveObjects(void);

HMODULE DLOpen(char *szModule);
void *DLGetSymbolAddr(HMODULE hModule, char *szSymbol);
void DLClose(HMODULE hModule);

BOOL DBInit(void);
DB_HANDLE DBConnect(void);
void DBDisconnect(DB_HANDLE hConn);
BOOL DBQuery(DB_HANDLE hConn, char *szQuery);
DB_RESULT DBSelect(DB_HANDLE hConn, char *szQuery);
DB_ASYNC_RESULT DBAsyncSelect(DB_HANDLE hConn, char *szQuery);
BOOL DBFetch(DB_ASYNC_RESULT hResult);
char *DBGetField(DB_RESULT hResult, int iRow, int iColumn);
long DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn);
DWORD DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn);
char *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, char *pBuffer, int iBufSize);
long DBGetFieldAsyncLong(DB_RESULT hResult, int iColumn);
DWORD DBGetFieldAsyncULong(DB_ASYNC_RESULT hResult, int iColumn);
int DBGetNumRows(DB_RESULT hResult);
void DBFreeResult(DB_RESULT hResult);
void DBFreeAsyncResult(DB_ASYNC_RESULT hResult);
void DBUnloadDriver(void);

void QueueSQLRequest(char *szQuery);
void StopDBWriter(void);

void SnmpInit(void);
BOOL SnmpGet(DWORD dwAddr, const char *szCommunity, const char *szOidStr,
             const oid *oidBinary, size_t iOidLen, void *pValue,
             DWORD dwBufferSize, BOOL bVerbose,
             BOOL bStringResult, BOOL bRecursiveCall);
BOOL SnmpEnumerate(DWORD dwAddr, const char *szCommunity, const char *szRootOid,
                   void (* pHandler)(DWORD, const char *, variable_list *, void *), 
                   void *pUserArg, BOOL bVerbose);

ARP_CACHE *GetLocalArpCache(void);
void DestroyArpCache(ARP_CACHE *pArpCache);
ARP_CACHE *SnmpGetArpCache(DWORD dwAddr, const char *szCommunity);

INTERFACE_LIST *SnmpGetInterfaceList(DWORD dwAddr, const char *szCommunity);
INTERFACE_LIST *GetLocalInterfaceList(void);
void CleanInterfaceList(INTERFACE_LIST *pIfList);
void DestroyInterfaceList(INTERFACE_LIST *pIfList);

void WatchdogInit(void);
DWORD WatchdogAddThread(char *szName);
void WatchdogNotify(DWORD dwId);
void WatchdogPrintStatus(void);

void CheckForMgmtNode(void);

void EnumerateClientSessions(void (*pHandler)(ClientSession *, void *), void *pArg);

void BinToStr(BYTE *pData, DWORD dwSize, char *pStr);
DWORD StrToBin(char *pStr, BYTE *pData, DWORD dwSize);

void GetSysInfoStr(char *pszBuffer);
DWORD GetLocalIpAddr(void);

BOOL ExecCommand(char *pszCommand);

BOOL InitIdTable(void);
DWORD CreateUniqueId(int iGroup);

void CreateSHA1Hash(char *pszSource, BYTE *pBuffer);

#ifdef _WIN32

void InitService(void);
void InstallService(char *execName);
void RemoveService(void);
void StartCoreService(void);
void StopCoreService(void);
void InstallEventSource(char *path);
void RemoveEventSource(void);

char *GetSystemErrorText(DWORD error);

#endif   /* _WIN32 */

void DbgTestMutex(MUTEX hMutex, char *szName);
void DbgPrintf(DWORD dwFlags, char *szFormat, ...);
void DumpSessions(void);


//
// Global variables
//

extern DWORD g_dwFlags;
extern char g_szConfigFile[];
extern char g_szLogFile[];

extern char g_szDbDriver[];
extern char g_szDbDriver[];
extern char g_szDbDrvParams[];
extern char g_szDbServer[];
extern char g_szDbLogin[];
extern char g_szDbPassword[];
extern char g_szDbName[];

extern DB_HANDLE g_hCoreDB;
extern Queue *g_pLazyRequestQueue;

#endif   /* _nms_core_h_ */
