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
** $module: session.cpp
**
**/

#include "libnxcl.h"


//
// Session class constructor
//

NXCL_Session::NXCL_Session()
{
   m_pEventHandler = NULL;
   m_dwState = STATE_DISCONNECTED;
   m_dwCommandTimeout = 10000;    // Default timeout is 10 seconds
   m_dwNumObjects = 0;
   m_pIndexById = NULL;
   m_mutexIndexAccess = MutexCreate();
   m_dwReceiverBufferSize = 4194304;     // 4MB
   m_hSocket = -1;
}


//
// Session class destructor
//

NXCL_Session::~NXCL_Session()
{
   Disconnect();
   MutexDestroy(m_mutexIndexAccess);
   safe_free(m_pIndexById);
}


//
// Disconnect session
//

void NXCL_Session::Disconnect(void)
{
   // Close socket
   shutdown(m_hSocket, SHUT_RDWR);
   closesocket(m_hSocket);

   // Clear message wait queue
   m_msgWaitQueue.Clear();

   // Cleanup
   DestroyAllObjects();
}


//
// Destroy all objects
//

void NXCL_Session::DestroyAllObjects(void)
{
   DWORD i;

   MutexLock(m_mutexIndexAccess, INFINITE);
   for(i = 0; i < m_dwNumObjects; i++)
      DestroyObject(m_pIndexById[i].pObject);
   m_dwNumObjects = 0;
   free(m_pIndexById);
   m_pIndexById = NULL;
   MutexUnlock(m_mutexIndexAccess);
}


//
// Wait for specific message
//

CSCPMessage *NXCL_Session::WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
{
   return m_msgWaitQueue.WaitForMessage(wCode, dwId, 
      dwTimeOut == 0 ? m_dwCommandTimeout : dwTimeOut);
}


//
// Wait for specific raw message
//

CSCP_MESSAGE *NXCL_Session::WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
{
   return m_msgWaitQueue.WaitForRawMessage(wCode, dwId, 
      dwTimeOut == 0 ? m_dwCommandTimeout : dwTimeOut);
}


//
// Wait for request completion notification and extract result code
// from recived message
//

DWORD NXCL_Session::WaitForRCC(DWORD dwRqId)
{
   CSCPMessage *pResponce;
   DWORD dwRetCode;

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Send CSCP message
//

BOOL NXCL_Session::SendMsg(CSCPMessage *pMsg)
{
   CSCP_MESSAGE *pRawMsg;
   BOOL bResult;
   TCHAR szBuffer[128];

   DebugPrintf(_T("SendMsg(\"%s\"), id:%ld)"), CSCPMessageCodeName(pMsg->GetCode(), szBuffer), pMsg->GetId());
   pRawMsg = pMsg->CreateMessage();
   bResult = (send(m_hSocket, (char *)pRawMsg, ntohl(pRawMsg->dwSize), 0) == (int)ntohl(pRawMsg->dwSize));
   free(pRawMsg);
   return bResult;
}
