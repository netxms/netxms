/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: libnxcl.h
**
**/

#ifndef _libnxcl_h_
#define _libnxcl_h_

#include <nms_common.h>
#include <nxclapi.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxcscpapi.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


//
// Constants
//

#define OBJECT_CACHE_MAGIC 0x7080ABCD

#define LIBNXCL_VERSION    2

#define MAX_SERVER_NAME    64
#define MAX_LOGIN_NAME     64
#define MAX_PASSWORD_LEN   64

#define VALIDATE_STRING(s) (((s) == NULL) ? _T("") : (s))


//
// Session flags
//

#define NXC_SF_USERDB_LOADED     0x0001
#define NXC_SF_SYNC_FINISHED     0x0002
#define NXC_SF_HAS_OBJECT_CACHE  0x0004


//
// Index structure
//

typedef struct
{
   DWORD dwKey;
   NXC_OBJECT *pObject;
} INDEX;


//
// Object cache file header structure
//

typedef struct
{
   DWORD dwMagic;
   DWORD dwStructSize;  // sizeof(NXC_OBJECT)
   DWORD dwTimeStamp;
   DWORD dwNumObjects;
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
   DWORD m_dwFlags;
   DWORD m_dwMsgId;
   DWORD m_dwState;
   DWORD m_dwTimeStamp;    // Last known server's timestamp
   DWORD m_dwNumObjects;
   INDEX *m_pIndexById;
   MUTEX m_mutexIndexAccess;
   SOCKET m_hSocket;
   MsgWaitQueue m_msgWaitQueue;
   DWORD m_dwReceiverBufferSize;
   NXC_DCI_LIST *m_pItemList;
   THREAD m_hRecvThread;

   NXC_EVENT_TEMPLATE **m_ppEventTemplates;
   DWORD m_dwNumTemplates;
   MUTEX m_mutexEventAccess;

   DWORD m_dwNumUsers;
   NXC_USER *m_pUserList;

#ifdef _WIN32
   HANDLE m_condSyncOp;
#else
   pthread_cond_t m_condSyncOp;
   pthread_mutex_t m_mutexSyncOp;
#endif
   DWORD m_dwSyncExitCode;

public:
   DWORD m_dwCommandTimeout;
   NXC_EVENT_HANDLER m_pEventHandler;
   BYTE m_bsServerId[8];

   // Methods
private:
   void DestroyAllObjects(void);
   void ProcessDCI(CSCPMessage *pMsg);
   void DestroyEventDB(void);
   void DestroyUserDB(void);
   void ProcessUserDBRecord(CSCPMessage *pMsg);
   void ProcessUserDBUpdate(CSCPMessage *pMsg);

   void ProcessObjectUpdate(CSCPMessage *pMsg);
   void AddObject(NXC_OBJECT *pObject, BOOL bSortIndex);
   void LoadObjectsFromCache(TCHAR *pszCacheFile);

public:
   NXCL_Session();
   ~NXCL_Session();

   void Attach(SOCKET hSocket) { m_hSocket = hSocket; }
   void Disconnect(void);

   void SetRecvThread(THREAD hThread) { m_hRecvThread = hThread; }

   BOOL SendMsg(CSCPMessage *pMsg);
   CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut = 0);
   CSCP_MESSAGE *WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut = 0);
   DWORD WaitForRCC(DWORD dwRqId, DWORD dwTimeOut = 0);
   DWORD CreateRqId(void) { return m_dwMsgId++; }
   DWORD SendFile(DWORD dwRqId, TCHAR *pszFileName);

   void CallEventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);

   DWORD WaitForSync(DWORD dwTimeOut);
   void PrepareForSync(void);
   void CompleteSync(DWORD dwRetCode);

   DWORD OpenNodeDCIList(DWORD dwNodeId, NXC_DCI_LIST **ppItemList);

   DWORD LoadEventDB(void);
   void AddEventTemplate(NXC_EVENT_TEMPLATE *pEventTemplate, BOOL bLock);
   void DeleteEDBRecord(DWORD dwEventCode);
   BOOL GetEventDB(NXC_EVENT_TEMPLATE ***pppTemplateList, DWORD *pdwNumRecords);
   const TCHAR *GetEventName(DWORD dwId);
   BOOL GetEventNameEx(DWORD dwId, TCHAR *pszBuffer, DWORD dwBufSize);
   int GetEventSeverity(DWORD dwId);
   BOOL GetEventText(DWORD dwId, TCHAR *pszBuffer, DWORD dwBufSize);

   DWORD LoadUserDB(void);
   BOOL GetUserDB(NXC_USER **ppUserList, DWORD *pdwNumUsers);
   NXC_USER *FindUserById(DWORD dwId);

   DWORD SyncObjects(TCHAR *pszCacheFile);
   void LockObjectIndex(void) { MutexLock(m_mutexIndexAccess, INFINITE); }
   void UnlockObjectIndex(void) { MutexUnlock(m_mutexIndexAccess); }
   NXC_OBJECT *FindObjectById(DWORD dwId, BOOL bLock);
   NXC_OBJECT *FindObjectByName(TCHAR *pszName);
   void EnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *));
   NXC_OBJECT *GetRootObject(DWORD dwId, DWORD dwIndex);
   void *GetObjectIndex(DWORD *pdwNumObjects);

   void SetTimeStamp(DWORD dwTimeStamp) { m_dwTimeStamp = dwTimeStamp; }
   DWORD GetTimeStamp(void) { return m_dwTimeStamp; }
};

inline void NXCL_Session::CallEventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   if (m_pEventHandler != NULL)
      m_pEventHandler(this, dwEvent, dwCode, pArg);
}


//
// Functions
//

void DestroyObject(NXC_OBJECT *pObject);
void UpdateUserFromMessage(CSCPMessage *pMsg, NXC_USER *pUser);

void ProcessAlarmUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessEvent(NXCL_Session *pSession, CSCPMessage *pMsg, CSCP_MESSAGE *pRawMsg);
void ProcessActionUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessEventDBRecord(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessUserDBUpdate(CSCPMessage *pMsg);
void ProcessDCI(NXCL_Session *pSession, CSCPMessage *pMsg);

void DebugPrintf(TCHAR *szFormat, ...);


//
// Global variables
//

extern NXC_DEBUG_CALLBACK g_pDebugCallBack;


#endif   /* _libnxcl_h_ */
