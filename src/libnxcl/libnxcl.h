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

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else    /* _WIN32 */
#include <unistd.h>
#endif   /* _WIN32 */

#include <nms_common.h>
#include <nxclapi.h>
#include <nms_util.h>
#include <nms_threads.h>


//
// Constants
//

#define MAX_SERVER_NAME    64
#define MAX_LOGIN_NAME     64
#define MAX_PASSWORD_LEN   64


//
// Request codes
//

#define RQ_CONNECT         1
#define RQ_SYNC_OBJECTS    2
#define RQ_SYNC_EVENTS     3


//
// Request structure
//

typedef struct
{
   DWORD dwCode;
   void *pArg;
   BOOL bDynamicArg;
} REQUEST;


//
// Message waiting queue element structure
//

typedef struct
{
   CSCPMessage *pMsg;
   DWORD dwTTL;
} WAIT_QUEUE_ELEMENT;


//
// Message waiting queue class
//

class MsgWaitQueue
{
   friend void MWQThreadStarter(void *);
private:
   MUTEX m_hMutex;
   CONDITION m_hStopCondition;
   DWORD m_dwMsgHoldTime;
   DWORD m_dwNumElements;
   WAIT_QUEUE_ELEMENT *m_pElements;

   void Lock(void) { MutexLock(m_hMutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hMutex); }
   void HousekeeperThread(void);

public:
   MsgWaitQueue();
   ~MsgWaitQueue();

   void Put(CSCPMessage *pMsg);
   CSCPMessage *WaitForMessage(DWORD dwCode, DWORD dwId, DWORD dwTimeOut);
   
   void Clear(void);
   void SetHoldTime(DWORD dwHoldTime) { m_dwMsgHoldTime = dwHoldTime; }
};


//
// Functions
//

void CreateRequest(DWORD dwCode, void *pArg, BOOL bDynamicArg);
void ChangeState(DWORD dwState);
BOOL Connect(void);
void ProcessObjectUpdate(CSCPMessage *pMsg);
void SyncObjects(void);
BOOL SendMsg(CSCPMessage *pMsg);
void DebugPrintf(char *szFormat, ...);


//
// Global variables
//

extern NXC_EVENT_HANDLER g_pEventHandler;
extern NXC_DEBUG_CALLBACK g_pDebugCallBack;
extern DWORD g_dwState;
extern Queue *g_pRequestQueue;


//
// Call client's event handler
//

inline void CallEventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   if (g_pEventHandler != NULL)
      g_pEventHandler(dwEvent, dwCode, pArg);
}

#endif   /* _libnxcl_h_ */
