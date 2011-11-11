/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: msgwq.cpp
**
**/

#include "libnetxms.h"


//
// Constants
//

#define TTL_CHECK_INTERVAL    200


//
// Constructor
//

MsgWaitQueue::MsgWaitQueue()
{
   m_dwMsgHoldTime = 30000;      // Default message TTL is 30 seconds
   m_dwNumElements = 0;
   m_pElements = NULL;
   m_mutexDataAccess = MutexCreate();
   m_condStop = ConditionCreate(FALSE);
   m_condNewMsg = ConditionCreate(TRUE);
   m_hHkThread = ThreadCreateEx(mwqThreadStarter, 0, this);
}


//
// Destructor
//

MsgWaitQueue::~MsgWaitQueue()
{
   ConditionSet(m_condStop);

   // Wait for housekeeper thread to terminate
   ThreadJoin(m_hHkThread);

   // Housekeeper thread stopped, proceed with object destruction
   clear();
   safe_free(m_pElements);
   MutexDestroy(m_mutexDataAccess);
   ConditionDestroy(m_condStop);
   ConditionDestroy(m_condNewMsg);
}


//
// Clear queue
//

void MsgWaitQueue::clear()
{
   DWORD i;

   lock();

   for(i = 0; i < m_dwNumElements; i++)
      if (m_pElements[i].wIsBinary)
      {
         safe_free(m_pElements[i].pMsg);
      }
      else
      {
         delete (CSCPMessage *)(m_pElements[i].pMsg);
      }
   m_dwNumElements = 0;
   unlock();
}


//
// Put message into queue
//

void MsgWaitQueue::put(CSCPMessage *pMsg)
{
   lock();

	// FIXME possible memory leak/fault
   m_pElements = (WAIT_QUEUE_ELEMENT *)realloc(m_pElements,
			sizeof(WAIT_QUEUE_ELEMENT) * (m_dwNumElements + 1));
	
   m_pElements[m_dwNumElements].wCode = pMsg->GetCode();
   m_pElements[m_dwNumElements].wIsBinary = 0;
   m_pElements[m_dwNumElements].dwId = pMsg->GetId();
   m_pElements[m_dwNumElements].dwTTL = m_dwMsgHoldTime;
   m_pElements[m_dwNumElements].pMsg = pMsg;
   m_dwNumElements++;

   unlock();

   ConditionPulse(m_condNewMsg);
}


//
// Put raw message into queue
//

void MsgWaitQueue::put(CSCP_MESSAGE *pMsg)
{
   lock();

   m_pElements = (WAIT_QUEUE_ELEMENT *)realloc(m_pElements, 
                              sizeof(WAIT_QUEUE_ELEMENT) * (m_dwNumElements + 1));
   m_pElements[m_dwNumElements].wCode = pMsg->wCode;
   m_pElements[m_dwNumElements].wIsBinary = 1;
   m_pElements[m_dwNumElements].dwId = pMsg->dwId;
   m_pElements[m_dwNumElements].dwTTL = m_dwMsgHoldTime;
   m_pElements[m_dwNumElements].pMsg = pMsg;
   m_dwNumElements++;
   unlock();

   ConditionPulse(m_condNewMsg);
}


//
// Wait for message with specific code and ID
// Function return pointer to the message on success or
// NULL on timeout or error
//

void *MsgWaitQueue::waitForMessageInternal(WORD wIsBinary, WORD wCode, DWORD dwId, DWORD dwTimeOut)
{
   DWORD i, dwSleepTime;
   QWORD qwStartTime;

   do
   {
      lock();
      for(i = 0; i < m_dwNumElements; i++)
		{
         if ((m_pElements[i].dwId == dwId) &&
             (m_pElements[i].wCode == wCode) &&
             (m_pElements[i].wIsBinary == wIsBinary))
         {
            void *pMsg;

            pMsg = m_pElements[i].pMsg;
            m_dwNumElements--;
            memmove(&m_pElements[i], &m_pElements[i + 1],
						sizeof(WAIT_QUEUE_ELEMENT) * (m_dwNumElements - i));
            unlock();
            return pMsg;
         }
		}
      unlock();

      qwStartTime = GetCurrentTimeMs();
      ConditionWait(m_condNewMsg, min(dwTimeOut, 100));
      dwSleepTime = (DWORD)(GetCurrentTimeMs() - qwStartTime);
      dwTimeOut -= min(dwSleepTime, dwTimeOut);
   } while(dwTimeOut > 0);

   return NULL;
}


//
// Housekeeping thread
//

void MsgWaitQueue::housekeeperThread()
{
   DWORD i;

   while(1)
   {
      if (ConditionWait(m_condStop, TTL_CHECK_INTERVAL))
         break;

      lock();
      for(i = 0; i < m_dwNumElements; i++)
		{
         if (m_pElements[i].dwTTL <= TTL_CHECK_INTERVAL)
         {
            if (m_pElements[i].wIsBinary)
            {
               safe_free(m_pElements[i].pMsg);
            }
            else
            {
               delete (CSCPMessage *)(m_pElements[i].pMsg);
            }
            m_dwNumElements--;
            memmove(&m_pElements[i], &m_pElements[i + 1], sizeof(WAIT_QUEUE_ELEMENT) * (m_dwNumElements - i));
            i--;
         }
         else
         {
            m_pElements[i].dwTTL -= TTL_CHECK_INTERVAL;
         }
		}
      unlock();
   }
}


//
// Housekeeper thread starter
//

THREAD_RESULT THREAD_CALL MsgWaitQueue::mwqThreadStarter(void *pArg)
{
   ((MsgWaitQueue *)pArg)->housekeeperThread();
   return THREAD_OK;
}
