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


//
// Constants
//

#define LIBNXCL_VERSION    1

#define MAX_SERVER_NAME    64
#define MAX_LOGIN_NAME     64
#define MAX_PASSWORD_LEN   64

#define VALIDATE_STRING(s) (((s) == NULL) ? _T("") : (s))


//
// Index structure
//

typedef struct
{
   DWORD dwKey;
   NXC_OBJECT *pObject;
} INDEX;


//
// Session class
//

class NXCL_Session
{
   friend THREAD_RESULT THREAD_CALL NetReceiver(NXCL_Session *pSession);

   // Attributes
private:
   DWORD m_dwMsgId;
   DWORD m_dwState;
   DWORD m_dwNumObjects;
   INDEX *m_pIndexById;
   MUTEX m_mutexIndexAccess;
   SOCKET m_hSocket;
   MsgWaitQueue m_msgWaitQueue;
   DWORD m_dwReceiverBufferSize;

public:
   DWORD m_dwCommandTimeout;
   NXC_EVENT_HANDLER m_pEventHandler;

   // Methods
private:
   void DestroyAllObjects(void);

public:
   NXCL_Session();
   ~NXCL_Session();

   void Attach(SOCKET hSocket) { m_hSocket = hSocket; }
   void Disconnect(void);

   BOOL SendMsg(CSCPMessage *pMsg);
   CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut = 0);
   CSCP_MESSAGE *WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut = 0);
   DWORD WaitForRCC(DWORD dwRqId);
   DWORD CreateRqId(void) { return m_dwMsgId++; }

   void CallEventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);
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

void InitEventDB(void);
void ShutdownEventDB(void);
void ProcessEventDBRecord(CSCPMessage *pMsg);

void ProcessAlarmUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessObjectUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);
void ProcessEvent(NXCL_Session *pSession, CSCPMessage *pMsg, CSCP_MESSAGE *pRawMsg);
void ProcessActionUpdate(NXCL_Session *pSession, CSCPMessage *pMsg);

void ProcessUserDBRecord(CSCPMessage *pMsg);
void ProcessUserDBUpdate(CSCPMessage *pMsg);

void ProcessDCI(CSCPMessage *pMsg);

void InitSyncStuff(void);
void SyncCleanup(void);
DWORD WaitForSync(DWORD dwTimeOut);
void PrepareForSync(void);
void CompleteSync(DWORD dwRetCode);

void DebugPrintf(TCHAR *szFormat, ...);


//
// Global variables
//

extern NXC_DEBUG_CALLBACK g_pDebugCallBack;


#endif   /* _libnxcl_h_ */
