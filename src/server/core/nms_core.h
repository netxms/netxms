/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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

#include <windows.h>

#else    /* _WIN32 */

#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/select.h>

#define closesocket(x) close(x)
#define WSAGetLastError() (errno)

#endif   /* _WIN32 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <nms_common.h>
#include <nms_threads.h>
#include <dbdrv.h>
#include <nms_cscp.h>
#include <nms_util.h>
#include "nms_users.h"
#include "nms_objects.h"
#include "nms_dcoll.h"
#include "messages.h"


//
// Common constants
//

#ifdef _WIN32
#define DEFAULT_CONFIG_FILE   "C:\\NetXMS.conf"
#define DEFAULT_LOG_FILE      "C:\\NetXMS.log"
#define IsStandalone() (g_dwFlags & AF_STANDALONE)
#else    /* _WIN32 */
#define DEFAULT_CONFIG_FILE   "/etc/netxms.conf"
#define DEFAULT_LOG_FILE      "/var/log/netxms.log"
#define IsStandalone() (1)
#endif   /* _WIN32 */

#define MAX_DB_LOGIN       64
#define MAX_DB_PASSWORD    64
#define MAX_DB_NAME        32
#define MAX_LINE_SIZE      4096


//
// Application flags
//

#define AF_STANDALONE                     0x00000001
#define AF_USE_EVENT_LOG                  0x00000002
#define AF_ENABLE_ACCESS_CONTROL          0x00000004
#define AF_ENABLE_EVENTS_ACCESS_CONTROL   0x00000008
#define AF_LOG_SQL_ERRORS                 0x00000010
#define AF_DEBUG_EVENTS                   0x00000100
#define AF_DEBUG_CSCP                     0x00000200
#define AF_DEBUG_DISCOVERY                0x00000400
#define AF_DEBUG_DC                       0x00000800
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
// Event handling subsystem definitions
//

#include "nms_events.h"


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
   DWORD GetParameter(char *szParam, DWORD dwBufSize, char *szBuffer);
};


//
// Client session
//

class ClientSession
{
private:
   SOCKET m_hSocket;
   Queue *m_pSendQueue;
   Queue *m_pMessageQueue;
   DWORD m_dwIndex;
   int m_iState;
   DWORD m_dwUserId;
   CSCP_BUFFER *m_pMsgBuffer;

   void DebugPrintf(char *szFormat, ...);
   void SendAllObjects(void);

public:
   ClientSession(SOCKET hSocket);
   ~ClientSession();

   void SendMessage(CSCPMessage *pMsg);
   void DispatchMessage(CSCP_MESSAGE *pMsg);

   void ReadThread(void);
   void WriteThread(void);
   void ProcessingThread(void);

   DWORD GetIndex(void) { return m_dwIndex; }
   void SetIndex(DWORD dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }
};


//
// Functions
//

BOOL ConfigReadStr(char *szVar, char *szBuffer, int iBufSize, char *szDefault);
int ConfigReadInt(char *szVar, int iDefault);
BOOL ConfigWriteStr(char *szVar, char *szValue, BOOL bCreate);
BOOL ConfigWriteInt(char *szVar, int iValue, BOOL bCreate);

void InitLog(void);
void CloseLog(void);
void WriteLog(DWORD msg, WORD wType, char *format, ...);

BOOL ParseCommandLine(int argc, char *argv[]);
BOOL LoadConfig(void);

void Shutdown(void);
BOOL Initialize(void);
void Main(void);

BOOL SleepAndCheckForShutdown(int iSeconds);

void StrStrip(char *str);
char *IpToStr(DWORD dwAddr, char *szBuffer);
int BitsInMask(DWORD dwMask);

void SaveObjects(void);

HMODULE DLOpen(char *szModule);
void *DLGetSymbolAddr(HMODULE hModule, char *szSymbol);
void DLClose(HMODULE hModule);

BOOL DBInit(void);
DB_HANDLE DBConnect(void);
void DBDisconnect(DB_HANDLE hConn);
BOOL DBQuery(DB_HANDLE hConn, char *szQuery);
DB_RESULT DBSelect(DB_HANDLE hConn, char *szQuery);
char *DBGetField(DB_RESULT hResult, int iRow, int iColumn);
long DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn);
DWORD DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn);
int DBGetNumRows(DB_RESULT hResult);
void DBFreeResult(DB_RESULT hResult);
void DBUnloadDriver(void);

void QueueSQLRequest(char *szQuery);

BOOL IcmpPing(DWORD dwAddr, int iNumRetries, DWORD dwTimeout);

void SnmpInit(void);
BOOL SnmpGet(DWORD dwAddr, char *szCommunity, char *szOidStr, oid *oidBinary, 
             size_t iOidLen, void *pValue, DWORD dwBufferSize, BOOL bVerbose, BOOL bStringResult);
BOOL SnmpEnumerate(DWORD dwAddr, char *szCommunity, char *szRootOid,
                   void (* pHandler)(DWORD, char *, variable_list *, void *), 
                   void *pUserArg, BOOL bVerbose);

INTERFACE_LIST *GetLocalInterfaceList(void);
INTERFACE_LIST *SnmpGetInterfaceList(DWORD dwAddr, char *szCommunity);
void CleanInterfaceList(INTERFACE_LIST *pIfList);
void DestroyInterfaceList(INTERFACE_LIST *pIfList);

ARP_CACHE *GetLocalArpCache(void);
ARP_CACHE *SnmpGetArpCache(DWORD dwAddr, char *szCommunity);
void DestroyArpCache(ARP_CACHE *pArpCache);

void WatchdogInit(void);
DWORD WatchdogAddThread(char *szName);
void WatchdogNotify(DWORD dwId);
void WatchdogPrintStatus(void);

void CheckForMgmtNode(void);

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
