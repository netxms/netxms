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
#include <nxcscpapi.h>
#include <string.h>
#include <openssl/ssl.h>


//
// Constants
//

#define LIBNXCL_VERSION    1

#define MAX_SERVER_NAME    64
#define MAX_LOGIN_NAME     64
#define MAX_PASSWORD_LEN   64


//
// Index structure
//

typedef struct
{
   DWORD dwKey;
   NXC_OBJECT *pObject;
} INDEX;


//
// Request structure
//

typedef struct
{
   DWORD dwCode;
   void *pArg;
   BOOL bDynamicArg;
   HREQUEST dwHandle;
} REQUEST;


//
// Message waiting queue element structure
//

typedef struct
{
   WORD wCode;       // Message code
   WORD wIsBinary;   // 1 for binary (raw) messages
   DWORD dwId;       // Message ID
   DWORD dwTTL;      // Message time-to-live in milliseconds
   void *pMsg;       // Pointer to message, either to CSCPMessage object or raw message
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
   void *WaitForMessageInternal(WORD wIsBinary, WORD wCode, DWORD dwId, DWORD dwTimeOut);

public:
   MsgWaitQueue();
   ~MsgWaitQueue();

   void Put(CSCPMessage *pMsg);
   void Put(CSCP_MESSAGE *pMsg);
   CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
   {
      return (CSCPMessage *)WaitForMessageInternal(0, wCode, dwId, dwTimeOut);
   }
   CSCP_MESSAGE *WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
   {
      return (CSCP_MESSAGE *)WaitForMessageInternal(1, wCode, dwId, dwTimeOut);
   }
   
   void Clear(void);
   void SetHoldTime(DWORD dwHoldTime) { m_dwMsgHoldTime = dwHoldTime; }
};


//
// Functions
//

void ObjectsInit(void);

void ProcessObjectUpdate(CSCPMessage *pMsg);
void ProcessEvent(CSCPMessage *pMsg, CSCP_MESSAGE *pRawMsg);
void ProcessEventDBRecord(CSCPMessage *pMsg);

void ProcessUserDBRecord(CSCPMessage *pMsg);
void ProcessUserDBUpdate(CSCPMessage *pMsg);

void ProcessDCI(CSCPMessage *pMsg);

BOOL SendMsg(CSCPMessage *pMsg);
CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut);
CSCP_MESSAGE *WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut);
DWORD WaitForRCC(DWORD dwRqId);

void ChangeState(DWORD dwState);
void DebugPrintf(char *szFormat, ...);

void CreateSHA1Hash(char *pszSource, BYTE *pBuffer);


//
// Global variables
//

extern NXC_EVENT_HANDLER g_pEventHandler;
extern NXC_DEBUG_CALLBACK g_pDebugCallBack;
extern DWORD g_dwState;
extern DWORD g_dwMsgId;


//
// Call client's event handler
//

inline void CallEventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   if (g_pEventHandler != NULL)
      g_pEventHandler(dwEvent, dwCode, pArg);
}

#endif   /* _libnxcl_h_ */
