/* 
** NetXMS - Network Management System
** Server Library
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
** $module: nxsrvapi.h
**
**/

#ifndef _nxsrvapi_h_
#define _nxsrvapi_h_

#ifdef _WIN32
#ifdef LIBNXSRV_EXPORTS
#define LIBNXSRV_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXSRV_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXSRV_EXPORTABLE
#endif

#ifndef LIBNXCL_NO_DECLARATIONS
#define LIBNXCL_NO_DECLARATIONS 1
#endif
#include <nxclapi.h>
#include <nxcscpapi.h>
#include <nms_agent.h>
#include <rwlock.h>
#include <messages.h>
#include <dbdrv.h>


//
// Default files
//

#ifdef _WIN32

# define DEFAULT_CONFIG_FILE   _T("C:\\netxmsd.conf")

# define DEFAULT_SHELL         "cmd.exe"
# define DEFAULT_LOG_FILE      "C:\\NetXMS.log"
# define DEFAULT_DATA_DIR      "C:\\NetXMS\\var"

# define DDIR_MIBS             "\\mibs"
# define DDIR_IMAGES           "\\images"
# define DDIR_PACKAGES         "\\packages"
# define DFILE_KEYS            "\\server_key"

#else    /* _WIN32 */

# define DEFAULT_CONFIG_FILE   _T("/etc/netxmsd.conf")

# define DEFAULT_SHELL         "/bin/sh"

# ifndef DATADIR
#  define DATADIR              "/var/netxms"
# endif

# define DEFAULT_LOG_FILE      DATADIR"/log/netxmsd.log"
# define DEFAULT_DATA_DIR      DATADIR

# define DDIR_MIBS             "/mibs"
# define DDIR_IMAGES           "/images"
# define DDIR_PACKAGES         "/packages"
# define DFILE_KEYS            "/.server_key"

#endif   /* _WIN32 */


//
// Encryption usage policies
//

#define ENCRYPTION_DISABLED   0
#define ENCRYPTION_ALLOWED    1
#define ENCRYPTION_PREFERRED  2
#define ENCRYPTION_REQUIRED   3


//
// DB-related constants
//

#define MAX_DB_LOGIN          64
#define MAX_DB_PASSWORD       64
#define MAX_DB_NAME           32


//
// Win32 service constants
//

#ifdef _WIN32

#define CORE_SERVICE_NAME  _T("NetXMSCore")
#define CORE_EVENT_SOURCE  _T("NetXMSCore")

#endif   /* _WIN32 */


//
// Single ARP cache entry
//

typedef struct
{
   DWORD dwIndex;       // Interface index
   DWORD dwIpAddr;
   BYTE bMacAddr[MAC_ADDR_LENGTH];
} ARP_ENTRY;


//
// ARP cache structure used by discovery functions and AgentConnection class
//

typedef struct
{
   DWORD dwNumEntries;
   ARP_ENTRY *pEntries;
} ARP_CACHE;


//
// Interface information structure used by discovery functions and AgentConnection class
//

typedef struct
{
   TCHAR szName[MAX_OBJECT_NAME];
   DWORD dwIndex;
   DWORD dwType;
   DWORD dwIpAddr;
   DWORD dwIpNetMask;
   BYTE bMacAddr[MAC_ADDR_LENGTH];
   int iNumSecondary;      // Number of secondary IP's on this interface
} INTERFACE_INFO;


//
// Interface list used by discovery functions and AgentConnection class
//

typedef struct
{
   int iNumEntries;              // Number of entries in pInterfaces
   int iEnumPos;                 // Used by index enumeration handler
   void *pArg;                   // Can be used by custom enumeration handlers
   INTERFACE_INFO *pInterfaces;  // Interface entries
} INTERFACE_LIST;


//
// Route information
//

typedef struct
{
   DWORD dwDestAddr;
   DWORD dwDestMask;
   DWORD dwNextHop;
   DWORD dwIfIndex;
   DWORD dwRouteType;
} ROUTE;


//
// Routing table
//

typedef struct
{
   int iNumEntries;     // Number of entries
   ROUTE *pRoutes;      // Route list
} ROUTING_TABLE;


//
// Agent connection
//

class LIBNXSRV_EXPORTABLE AgentConnection
{
private:
   DWORD m_dwAddr;
   int m_iAuthMethod;
   char m_szSecret[MAX_SECRET_LENGTH];
   time_t m_tLastCommandTime;
   SOCKET m_hSocket;
   WORD m_wPort;
   DWORD m_dwNumDataLines;
   DWORD m_dwRequestId;
   DWORD m_dwCommandTimeout;
   TCHAR **m_ppDataLines;
   MsgWaitQueue *m_pMsgWaitQueue;
   BOOL m_bIsConnected;
   MUTEX m_mutexDataLock;
   THREAD m_hReceiverThread;
   CSCP_ENCRYPTION_CONTEXT *m_pCtx;
   int m_iEncryptionPolicy;

   void ReceiverThread(void);
   static THREAD_RESULT THREAD_CALL ReceiverThreadStarter(void *);

protected:
   void DestroyResultData(void);
   BOOL SendMessage(CSCPMessage *pMsg);
   CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut) { return m_pMsgWaitQueue->WaitForMessage(wCode, dwId, dwTimeOut); }
   DWORD WaitForRCC(DWORD dwRqId, DWORD dwTimeOut);
   DWORD Authenticate(void);
   DWORD SetupEncryption(RSA *pServerKey);

   virtual void PrintMsg(TCHAR *pszFormat, ...);
   virtual void OnTrap(CSCPMessage *pMsg);

   void Lock(void) { MutexLock(m_mutexDataLock, INFINITE); }
   void Unlock(void) { MutexUnlock(m_mutexDataLock); }

public:
   AgentConnection();
   AgentConnection(DWORD dwAddr, WORD wPort = AGENT_LISTEN_PORT, int iAuthMethod = AUTH_NONE, TCHAR *szSecret = NULL);
   ~AgentConnection();

   BOOL Connect(RSA *pServerKey = NULL, BOOL bVerbose = FALSE, DWORD *pdwError = NULL);
   void Disconnect(void);
   BOOL IsConnected(void) { return m_bIsConnected; }

   ARP_CACHE *GetArpCache(void);
   INTERFACE_LIST *GetInterfaceList(void);
   ROUTING_TABLE *GetRoutingTable(void);
   DWORD GetParameter(TCHAR *pszParam, DWORD dwBufSize, TCHAR *pszBuffer);
   DWORD GetList(TCHAR *pszParam);
   DWORD Nop(void);
   DWORD ExecAction(TCHAR *pszAction, int argc, TCHAR **argv);
   DWORD UploadFile(TCHAR *pszFile);
   DWORD StartUpgrade(TCHAR *pszPkgName);
   DWORD CheckNetworkService(DWORD *pdwStatus, DWORD dwIpAddr, int iServiceType, WORD wPort = 0, 
                             WORD wProto = 0, TCHAR *pszRequest = NULL, TCHAR *pszResponse = NULL);
   DWORD GetSupportedParameters(DWORD *pdwNumParams, NXC_AGENT_PARAM **ppParamList);
   DWORD GetConfigFile(TCHAR **ppszConfig, DWORD *pdwSize);
   DWORD UpdateConfigFile(TCHAR *pszConfig);

   DWORD GetNumDataLines(void) { return m_dwNumDataLines; }
   const TCHAR *GetDataLine(DWORD dwIndex) { return dwIndex < m_dwNumDataLines ? m_ppDataLines[dwIndex] : _T("(error)"); }

   void SetCommandTimeout(DWORD dwTimeout) { if (dwTimeout > 500) m_dwCommandTimeout = dwTimeout; }
   void SetEncryptionPolicy(int iPolicy) { m_iEncryptionPolicy = iPolicy; }
};


//
// Functions
//

void LIBNXSRV_EXPORTABLE DestroyArpCache(ARP_CACHE *pArpCache);
void LIBNXSRV_EXPORTABLE DestroyInterfaceList(INTERFACE_LIST *pIfList);
void LIBNXSRV_EXPORTABLE DestroyRoutingTable(ROUTING_TABLE *pRT);
void LIBNXSRV_EXPORTABLE SortRoutingTable(ROUTING_TABLE *pRT);
const TCHAR LIBNXSRV_EXPORTABLE *AgentErrorCodeToText(int iError);
BYTE LIBNXSRV_EXPORTABLE *LoadFile(char *pszFileName, DWORD *pdwFileSize);

void LIBNXSRV_EXPORTABLE InitLog(BOOL bUseSystemLog, char *pszLogFile, BOOL bPrintToScreen);
void LIBNXSRV_EXPORTABLE CloseLog(void);
void LIBNXSRV_EXPORTABLE WriteLog(DWORD msg, WORD wType, char *format, ...);

BOOL LIBNXSRV_EXPORTABLE DBInit(BOOL bWriteLog, BOOL bLogErrors, BOOL bDumpSQL);
DB_HANDLE LIBNXSRV_EXPORTABLE DBConnect(void);
DB_HANDLE LIBNXSRV_EXPORTABLE DBConnectEx(TCHAR *pszServer, TCHAR *pszDBName,
                                          TCHAR *pszLogin, TCHAR *pszPassword);
void LIBNXSRV_EXPORTABLE DBDisconnect(DB_HANDLE hConn);
BOOL LIBNXSRV_EXPORTABLE DBQuery(DB_HANDLE hConn, char *szQuery);
DB_RESULT LIBNXSRV_EXPORTABLE DBSelect(DB_HANDLE hConn, char *szQuery);
DB_ASYNC_RESULT LIBNXSRV_EXPORTABLE DBAsyncSelect(DB_HANDLE hConn, char *szQuery);
BOOL LIBNXSRV_EXPORTABLE DBFetch(DB_ASYNC_RESULT hResult);
char LIBNXSRV_EXPORTABLE *DBGetField(DB_RESULT hResult, int iRow, int iColumn);
long LIBNXSRV_EXPORTABLE DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn);
DWORD LIBNXSRV_EXPORTABLE DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn);
INT64 LIBNXSRV_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int iRow, int iColumn);
QWORD LIBNXSRV_EXPORTABLE DBGetFieldUInt64(DB_RESULT hResult, int iRow, int iColumn);
double LIBNXSRV_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int iRow, int iColumn);
DWORD LIBNXSRV_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int iRow, int iColumn);
char LIBNXSRV_EXPORTABLE *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, char *pBuffer, int iBufSize);
long LIBNXSRV_EXPORTABLE DBGetFieldAsyncLong(DB_RESULT hResult, int iColumn);
DWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncULong(DB_ASYNC_RESULT hResult, int iColumn);
INT64 LIBNXSRV_EXPORTABLE DBGetFieldAsyncInt64(DB_RESULT hResult, int iColumn);
QWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncUInt64(DB_ASYNC_RESULT hResult, int iColumn);
double LIBNXSRV_EXPORTABLE DBGetFieldAsyncDouble(DB_RESULT hResult, int iColumn);
DWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncIPAddr(DB_RESULT hResult, int iColumn);
int LIBNXSRV_EXPORTABLE DBGetNumRows(DB_RESULT hResult);
void LIBNXSRV_EXPORTABLE DBFreeResult(DB_RESULT hResult);
void LIBNXSRV_EXPORTABLE DBFreeAsyncResult(DB_ASYNC_RESULT hResult);
void LIBNXSRV_EXPORTABLE DBUnloadDriver(void);

TCHAR LIBNXSRV_EXPORTABLE *EncodeSQLString(const TCHAR *pszIn);
void LIBNXSRV_EXPORTABLE DecodeSQLString(TCHAR *pszStr);

void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy);


//
// Variables
//

extern char LIBNXSRV_EXPORTABLE g_szDbDriver[];
extern char LIBNXSRV_EXPORTABLE g_szDbDrvParams[];
extern char LIBNXSRV_EXPORTABLE g_szDbServer[];
extern char LIBNXSRV_EXPORTABLE g_szDbLogin[];
extern char LIBNXSRV_EXPORTABLE g_szDbPassword[];
extern char LIBNXSRV_EXPORTABLE g_szDbName[];

#endif   /* _nxsrvapi_h_ */
