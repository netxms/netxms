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

#endif   /* _WIN32 */

#include <time.h>
#include <stdio.h>
#include <string.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <nms_common.h>
#include <nms_threads.h>
#include <dbdrv.h>
#include "nms_objects.h"
#include "nms_events.h"
#include "messages.h"


//
// Common constants
//

#define VERSION_MAJOR      1
#define VERSION_MINOR      0
#define VERSION_RELEASE    0
#define VERSION_STRING     "1.0.0"

#ifdef _WIN32
#define DEFAULT_CONFIG_FILE   "C:\\nms.conf"
#define DEFAULT_LOG_FILE      "C:\\nms.log"
#define IsStandalone() (g_dwFlags & AF_STANDALONE)
#else    /* _WIN32 */
#define DEFAULT_CONFIG_FILE   "/etc/nms.conf"
#define DEFAULT_LOG_FILE      "/var/log/nms.log"
#define IsStandalone() (1)
#endif   /* _WIN32 */

#define MAX_DB_LOGIN       64
#define MAX_DB_PASSWORD    64
#define MAX_DB_NAME        32
#define MAX_LINE_SIZE      4096


//
// Application flags
//

#define AF_STANDALONE      0x0001
#define AF_USE_EVENT_LOG   0x0002
#define AF_SHUTDOWN        0x8000

#define ShutdownInProgress()  (g_dwFlags & AF_SHUTDOWN)


//
// Win32 service constants
//

#ifdef _WIN32

#define CORE_SERVICE_NAME  "NMSCore"
#define CORE_EVENT_SOURCE  "NMSCore"

#endif   /* _WIN32 */


//
// Queue
//

class Queue
{
private:
   MUTEX m_hQueueAccess;
   void **m_pElements;
   DWORD m_dwNumElements;
   DWORD m_dwBufferSize;
   DWORD m_dwFirst;
   DWORD m_dwLast;

   void Lock(void) { MutexLock(m_hQueueAccess, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hQueueAccess); }

public:
   Queue();
   ~Queue();

   void Put(void *pObject);
   void *Get(void);
};


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

   BOOL Connect(void);
   void Disconnect(void);

   ARP_CACHE *GetArpCache(void);
   INTERFACE_LIST *GetInterfaceList(void);
   DWORD Nop(void) { return ExecuteCommand("NOP"); }
   DWORD GetParameter(char *szParam, DWORD dwBufSize, char *szBuffer);
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

void StrStrip(char *str);
char *IpToStr(DWORD dwAddr, char *szBuffer);
int BitsInMask(DWORD dwMask);

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

BOOL IcmpPing(DWORD dwAddr, int iNumRetries, DWORD dwTimeout);

BOOL SnmpGet(DWORD dwAddr, char *szCommunity, char *szOidStr, oid *oidBinary, 
             size_t iOidLen, void *pValue, DWORD dwBufferSize);
BOOL SnmpEnumerate(DWORD dwAddr, char *szCommunity, char *szRootOid,
                   void (* pHandler)(DWORD, char *, variable_list *, void *), void *pUserArg);

INTERFACE_LIST *GetLocalInterfaceList(void);
INTERFACE_LIST *SnmpGetInterfaceList(DWORD dwAddr, char *szCommunity);
void CleanInterfaceList(INTERFACE_LIST *pIfList);
void DestroyInterfaceList(INTERFACE_LIST *pIfList);

ARP_CACHE *GetLocalArpCache(void);
ARP_CACHE *SnmpGetArpCache(DWORD dwAddr, char *szCommunity);
void DestroyArpCache(ARP_CACHE *pArpCache);

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

#endif   /* _nms_core_h_ */
