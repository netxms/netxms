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
#include <fcntl.h>
#endif   /* _WIN32 */

#include <nms_common.h>
#include <nxclapi.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxcscpapi.h>
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
// Functions
//

void ObjectsInit(void);

void ProcessAlarmUpdate(CSCPMessage *pMsg);
void ProcessObjectUpdate(CSCPMessage *pMsg);
void ProcessEvent(CSCPMessage *pMsg, CSCP_MESSAGE *pRawMsg);
void ProcessEventDBRecord(CSCPMessage *pMsg);
void ProcessActionUpdate(CSCPMessage *pMsg);

void ProcessUserDBRecord(CSCPMessage *pMsg);
void ProcessUserDBUpdate(CSCPMessage *pMsg);

void ProcessDCI(CSCPMessage *pMsg);

BOOL SendMsg(CSCPMessage *pMsg);
CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut);
CSCP_MESSAGE *WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut);
DWORD WaitForRCC(DWORD dwRqId);

void InitSyncStuff(void);
void SyncCleanup(void);
DWORD WaitForSync(DWORD dwTimeOut);
void PrepareForSync(void);
void CompleteSync(DWORD dwRetCode);

void ChangeState(DWORD dwState);
void DebugPrintf(TCHAR *szFormat, ...);


//
// Global variables
//

extern NXC_EVENT_HANDLER g_pEventHandler;
extern NXC_DEBUG_CALLBACK g_pDebugCallBack;
extern DWORD g_dwState;
extern DWORD g_dwMsgId;
extern DWORD g_dwCommandTimeout;


//
// Call client's event handler
//

inline void CallEventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   if (g_pEventHandler != NULL)
      g_pEventHandler(dwEvent, dwCode, pArg);
}

#endif   /* _libnxcl_h_ */
