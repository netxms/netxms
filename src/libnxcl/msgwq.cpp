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
** $module: msgwq.cpp
**
**/

#include "libnxcl.h"


//
// Constants
//

#define TTL_CHECK_INTERVAL    100


//
// Constructor
//

MsgWaitQueue::MsgWaitQueue()
{
   m_dwMsgHoldTime = 30000;      // Default message TTL is 30 seconds
   m_dwNumElements = 0;
   m_pElements = NULL;
   m_hMutex = MutexCreate();
   m_hStopCondition = ConditionCreate();
   ThreadCreate(MWQThreadStarter, 0, this);
}


//
// Destructor
//

MsgWaitQueue::~MsgWaitQueue()
{
   ConditionSet(m_hStopCondition);
   Clear();
   MemFree(m_pElements);
   MutexDestroy(m_hMutex);
   ConditionDestroy(m_hStopCondition);
}


//
// Clear queue
//

void MsgWaitQueue::Clear(void)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwNumElements; i++)
      delete m_pElements[i].pMsg;
   m_dwNumElements = 0;
   Unlock();
}


//
// Put message into queue
//

void MsgWaitQueue::Put(CSCPMessage *pMsg)
{
   Lock();
   m_pElements = (WAIT_QUEUE_ELEMENT *)MemReAlloc(m_pElements, 
                              sizeof(WAIT_QUEUE_ELEMENT) * (m_dwNumElements + 1));
   m_pElements[m_dwNumElements].dwTTL = m_dwMsgHoldTime;
   m_pElements[m_dwNumElements].pMsg = pMsg;
   m_dwNumElements++;
   Unlock();
}


//
// Wait for message with specific code and ID
// Function return pointer to the message on success or
// NULL on timeout or error
//

CSCPMessage *MsgWaitQueue::WaitForMessage(DWORD dwCode, DWORD dwId, DWORD dwTimeOut)
{
   DWORD i, dwSleepTime;

   do
   {
      Lock();
      for(i = 0; i < m_dwNumElements; i++)
         if ((m_pElements[i].pMsg->GetId() == dwId) &&
             (m_pElements[i].pMsg->GetCode() == dwCode))
         {
            CSCPMessage *pMsg;

            pMsg = m_pElements[i].pMsg;
            m_dwNumElements--;
            memmove(&m_pElements[i], &m_pElements[i + 1], sizeof(WAIT_QUEUE_ELEMENT) * (m_dwNumElements - i));
            Unlock();
            return pMsg;
         }
      Unlock();

      dwSleepTime = min(200, dwTimeOut);
      ThreadSleepMs(dwSleepTime);
      dwTimeOut -= dwSleepTime;
   } while(dwTimeOut > 0);

   return NULL;
}


//
// Housekeeping thread
//

void MsgWaitQueue::HousekeeperThread(void)
{
   DWORD i;

   while(1)
   {
      if (ConditionWait(m_hStopCondition, TTL_CHECK_INTERVAL))
         break;

      Lock();
      for(i = 0; i < m_dwNumElements; i++)
         if (m_pElements[i].dwTTL <= TTL_CHECK_INTERVAL)
         {
            delete m_pElements[i].pMsg;
            m_dwNumElements--;
            memmove(&m_pElements[i], &m_pElements[i + 1], sizeof(WAIT_QUEUE_ELEMENT) * (m_dwNumElements - i));
            i--;
         }
         else
         {
            m_pElements[i].dwTTL -= TTL_CHECK_INTERVAL;
         }
      Unlock();
   }
}


//
// Housekeeper thread starter
//

void MWQThreadStarter(void *pArg)
{
   ((MsgWaitQueue *)pArg)->HousekeeperThread();
}
