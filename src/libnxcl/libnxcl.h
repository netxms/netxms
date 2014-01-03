/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: libnxcl.h
**
**/

#ifndef _libnxcl_h_
#define _libnxcl_h_

#include <nms_common.h>
#include <nxclapi.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxcpapi.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


//
// Constants
//

#define OBJECT_CACHE_MAGIC 0x20120513

#define MAX_SERVER_NAME    64
#define MAX_LOGIN_NAME     64
#define MAX_PASSWORD_LEN   64
#define MAX_LOCKINFO_LEN   256
#define MAX_TZ_LEN         32


//
// Session flags
//

#define NXC_SF_USERDB_LOADED     0x0001
#define NXC_SF_HAS_OBJECT_CACHE  0x0002
#define NXC_SF_CHANGE_PASSWD     0x0004
#define NXC_SF_CONN_BROKEN       0x0008
#define NXC_SF_BAD_DBCONN        0x0010


//
// Sync operations
//

#define SYNC_EVENTS     0
#define SYNC_OBJECTS    1
#define SYNC_SYSLOG     2
#define SYNC_TRAP_LOG   3
#define SYNC_EVENT_DB   4
#define SYNC_USER_DB    5
#define SYNC_DCI_LIST   6

#define SYNC_OP_COUNT   7


//
// Index structure
//

typedef struct
{
   UINT32 dwKey;
   NXC_OBJECT *pObject;
} INDEX;


//
// Object cache file header structure
//

typedef struct
{
   UINT32 dwMagic;
   UINT32 dwStructSize;  // sizeof(NXC_OBJECT)
   UINT32 dwTimeStamp;
   UINT32 dwNumObjects;
   BYTE bsServerId[8];
} OBJECT_CACHE_HEADER;


//
// Session class
//

class NXCL_Session
{
   friend THREAD_RESULT THREAD_CALL NetReceiver(NXCL_Session *pSession);

   // Attributes
private:
   UINT32 m_dwFlags;
   UINT32 m_dwMsgId;
   UINT32 m_dwState;
   UINT32 m_dwTimeStamp;    // Last known server's timestamp
   UINT32 m_dwNumObjects;
   INDEX *m_pIndexById;
   MUTEX m_mutexIndexAccess;
   SOCKET m_hSocket;
	NXCPEncryptionContext *m_pCtx;
   MsgWaitQueue m_msgWaitQueue;
   UINT32 m_dwReceiverBufferSize;
   NXC_DCI_LIST *m_pItemList;
   THREAD m_hRecvThread;
   THREAD m_hWatchdogThread;
   CONDITION m_condStopThreads;
   TCHAR m_szLastLock[MAX_LOCKINFO_LEN];
   void *m_pClientData;       // Client-defined data
	TCHAR m_szServerTimeZone[MAX_TZ_LEN];
	MUTEX m_mutexSendMsg;

   UINT32 m_dwUserId;          // Id of logged-in user
   UINT32 m_dwSystemAccess;    // System access rights for current user

   int m_hCurrFile;
   UINT32 m_dwFileRqId;
   UINT32 m_dwFileRqCompletion;
   CONDITION m_condFileRq;
   MUTEX m_mutexFileRq;

   NXC_EVENT_TEMPLATE **m_ppEventTemplates;
   UINT32 m_dwNumTemplates;
   MUTEX m_mutexEventAccess;

   UINT32 m_dwNumUsers;
   NXC_USER *m_pUserList;

   MUTEX m_mutexSyncOpAccess[SYNC_OP_COUNT];
   UINT32 m_dwSyncExitCode[SYNC_OP_COUNT];
#ifdef _WIN32
   HANDLE m_condSyncOp[SYNC_OP_COUNT];
#else
   pthread_cond_t m_condSyncOp[SYNC_OP_COUNT];
   pthread_mutex_t m_mutexSyncOp[SYNC_OP_COUNT];
   BOOL m_bSyncFinished[SYNC_OP_COUNT];
#endif

public:
   UINT32 m_dwCommandTimeout;
   NXC_EVENT_HANDLER m_pEventHandler;
   BYTE m_bsServerId[8];

   // Methods
private:
   void destroyAllObjects();
   void processDCI(CSCPMessage *pMsg);
   void destroyEventDB();
   void destroyUserDB();
   void processUserDBRecord(CSCPMessage *pMsg);
   void processUserDBUpdate(CSCPMessage *pMsg);

   void processObjectUpdate(CSCPMessage *pMsg);
   void addObject(NXC_OBJECT *pObject, BOOL bSortIndex);
   void loadObjectsFromCache(const TCHAR *pszCacheFile);

   void watchdogThread();
   static THREAD_RESULT THREAD_CALL watchdogThreadStarter(void *pArg);

public:
   NXCL_Session();
   ~NXCL_Session();

   void attach(SOCKET hSocket) { m_hSocket = hSocket; }
   void disconnect();

   void SetRecvThread(THREAD hThread) { m_hRecvThread = hThread; }
   void StartWatchdogThread(void);

   BOOL SendMsg(CSCPMessage *pMsg);
   CSCPMessage *WaitForMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut = 0);
   CSCP_MESSAGE *WaitForRawMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut = 0);
   UINT32 WaitForRCC(UINT32 dwRqId, UINT32 dwTimeOut = 0);
   UINT32 CreateRqId(void) { return m_dwMsgId++; }
   UINT32 SendFile(UINT32 dwRqId, TCHAR *pszFileName, long sizeLimit);
   UINT32 SimpleCommand(WORD wCmd);

   void callEventHandler(UINT32 dwEvent, UINT32 dwCode, void *pArg);

   UINT32 WaitForSync(int nSyncOp, UINT32 dwTimeOut);
   void PrepareForSync(int nSyncOp);
   void CompleteSync(int nSyncOp, UINT32 dwRetCode);
   void UnlockSyncOp(int nSyncOp) { MutexUnlock(m_mutexSyncOpAccess[nSyncOp]); }

   UINT32 OpenNodeDCIList(UINT32 dwNodeId, NXC_DCI_LIST **ppItemList);

   UINT32 LoadEventDB(void);
   void AddEventTemplate(NXC_EVENT_TEMPLATE *pEventTemplate, BOOL bLock);
   void DeleteEDBRecord(UINT32 dwEventCode);
   BOOL GetEventDB(NXC_EVENT_TEMPLATE ***pppTemplateList, UINT32 *pdwNumRecords);
   const TCHAR *GetEventName(UINT32 dwId);
   BOOL GetEventNameEx(UINT32 dwId, TCHAR *pszBuffer, UINT32 dwBufSize);
   int GetEventSeverity(UINT32 dwId);
   BOOL GetEventText(UINT32 dwId, TCHAR *pszBuffer, UINT32 dwBufSize);

   UINT32 LoadUserDB(void);
   BOOL GetUserDB(NXC_USER **ppUserList, UINT32 *pdwNumUsers);
   NXC_USER *FindUserById(UINT32 dwId);

   UINT32 syncObjects(const TCHAR *pszCacheFile, BOOL bSyncComments);
   void lockObjectIndex() { MutexLock(m_mutexIndexAccess); }
   void unlockObjectIndex() { MutexUnlock(m_mutexIndexAccess); }
   NXC_OBJECT *findObjectById(UINT32 id, BOOL lock);
   NXC_OBJECT *findObjectByName(const TCHAR *name, UINT32 currObject);
   NXC_OBJECT *findObjectByComments(const TCHAR *comments, UINT32 currObject);
   NXC_OBJECT *findObjectByIPAddress(UINT32 ipAddr, UINT32 currObject);
   void EnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *));
   NXC_OBJECT *GetRootObject(UINT32 dwId, UINT32 dwIndex);
   void *GetObjectIndex(UINT32 *pdwNumObjects);

   void SetTimeStamp(UINT32 dwTimeStamp) { m_dwTimeStamp = dwTimeStamp; }
   UINT32 GetTimeStamp(void) { return m_dwTimeStamp; }

   UINT32 SetSubscriptionStatus(UINT32 dwChannels, int nOperation);

   UINT32 PrepareFileTransfer(const TCHAR *pszFile, UINT32 dwRqId);
   UINT32 WaitForFileTransfer(UINT32 dwTimeout);
   void AbortFileTransfer(void);

   void SetClientData(void *pData) { m_pClientData = pData; }
   void *GetClientData(void) { return m_pClientData; }

   void OnNotify(CSCPMessage *pMsg);
   void ParseLoginMessage(CSCPMessage *pMsg);
   UINT32 GetCurrentUserId(void) { return m_dwUserId; }
   UINT32 GetCurrentSystemAccess(void) { return m_dwSystemAccess; }
   BOOL NeedPasswordChange(void) { return (m_dwFlags & NXC_SF_CHANGE_PASSWD) ? TRUE : FALSE; }
   BOOL IsDBConnLost() { return (m_dwFlags & NXC_SF_BAD_DBCONN) ? TRUE : FALSE; }

   void setLastLock(const TCHAR *pszLock) { nx_strncpy(m_szLastLock, pszLock, MAX_LOCKINFO_LEN); }
   const TCHAR *getLastLock() { return m_szLastLock; }

	TCHAR *getServerTimeZone() { return m_szServerTimeZone; }
};

inline void NXCL_Session::callEventHandler(UINT32 dwEvent, UINT32 dwCode, void *pArg)
{
   if (m_pEventHandler != NULL)
      m_pEventHandler(this, dwEvent, dwCode, pArg);
}

#define CHECK_SESSION_HANDLE() { if (hSession == NULL) return RCC_INVALID_SESSION_HANDLE; }


//
// Functions
//

void DestroyObject(NXC_OBJECT *pObject);
void UpdateUserFromMessage(CSCPMessage *pMsg, NXC_USER *pUser);

void ProcessAlarmUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessEventDBUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessEventLogRecords(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessSyslogRecords(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessTrapLogRecords(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessTrapCfgUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessActionUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessEventDBRecord(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessUserDBUpdate(CSCPMessage *pMsg);
void ProcessSituationChange(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessDCI(NXCL_Session *pSession, CSCPMessage *pMsg);

void DebugPrintf(const TCHAR *format, ...);


//
// Global variables
//

extern NXC_DEBUG_CALLBACK g_pDebugCallBack;


#endif   /* _libnxcl_h_ */
