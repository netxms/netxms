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

/** 
 * Interval between checking messages TTL in milliseconds
 */
#define TTL_CHECK_INTERVAL    500

/**
 * Buffer allocation step
 */
#define ALLOCATION_STEP       16

/**
 * Constructor
 */
MsgWaitQueue::MsgWaitQueue()
{
   m_holdTime = 30000;      // Default message TTL is 30 seconds
   m_size = 0;
   m_allocated = 0;
   m_elements = NULL;
   m_stopCondition = ConditionCreate(FALSE);
#ifdef _WIN32
   InitializeCriticalSectionAndSpinCount(&m_mutex, 4000);
   memset(m_wakeupEvents, 0, MAX_MSGQUEUE_WAITERS * sizeof(HANDLE));
   m_wakeupEvents[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
   memset(m_waiters, 0, MAX_MSGQUEUE_WAITERS);
#else
   pthread_mutex_init(&m_mutex, NULL);
   pthread_cond_init(&m_wakeupCondition, NULL);
#endif
   m_hHkThread = ThreadCreateEx(mwqThreadStarter, 0, this);
}

/**
 * Destructor
 */
MsgWaitQueue::~MsgWaitQueue()
{
   ConditionSet(m_stopCondition);

   // Wait for housekeeper thread to terminate
   ThreadJoin(m_hHkThread);

   // Housekeeper thread stopped, proceed with object destruction
   clear();
   safe_free(m_elements);
   ConditionDestroy(m_stopCondition);

#ifdef _WIN32
   DeleteCriticalSection(&m_mutex);
   for(int i = 0; i < MAX_MSGQUEUE_WAITERS; i++)
      if (m_wakeupEvents[i] != NULL)
         CloseHandle(m_wakeupEvents[i]);
#else
   pthread_mutex_destroy(&m_mutex);
   pthread_cond_destroy(&m_wakeupCondition);
#endif
}

/**
 * Clear queue
 */
void MsgWaitQueue::clear()
{
   lock();

   for(int i = 0; i < m_allocated; i++)
   {
      if (m_elements[i].msg == NULL)
         continue;

      if (m_elements[i].isBinary)
      {
         safe_free(m_elements[i].msg);
      }
      else
      {
         delete (NXCPMessage *)(m_elements[i].msg);
      }
   }
   m_size = 0;
   m_allocated = 0;
   safe_free_and_null(m_elements);
   unlock();
}

/**
 * Put message into queue
 */
void MsgWaitQueue::put(NXCPMessage *pMsg)
{
   lock();

   int pos;
   if (m_size == m_allocated)
   {
      pos = m_allocated;
      m_allocated += ALLOCATION_STEP;
      m_elements = (WAIT_QUEUE_ELEMENT *)realloc(m_elements, sizeof(WAIT_QUEUE_ELEMENT) * m_allocated);
      memset(&m_elements[pos], 0, sizeof(WAIT_QUEUE_ELEMENT) * ALLOCATION_STEP);
   }
   else
   {
      for(pos = 0; m_elements[pos].msg != NULL; pos++);
   }
	
   m_elements[pos].code = pMsg->getCode();
   m_elements[pos].isBinary = 0;
   m_elements[pos].id = pMsg->getId();
   m_elements[pos].ttl = m_holdTime;
   m_elements[pos].msg = pMsg;
   m_size++;

#ifdef _WIN32
   for(int i = 0; i < MAX_MSGQUEUE_WAITERS; i++)
      if (m_waiters[i])
         SetEvent(m_wakeupEvents[i]);
#else
   pthread_cond_broadcast(&m_wakeupCondition);
#endif

   unlock();
}

/**
 * Put raw message into queue
 */
void MsgWaitQueue::put(NXCP_MESSAGE *pMsg)
{
   lock();

   int pos;
   if (m_size == m_allocated)
   {
      pos = m_allocated;
      m_allocated += ALLOCATION_STEP;
      m_elements = (WAIT_QUEUE_ELEMENT *)realloc(m_elements, sizeof(WAIT_QUEUE_ELEMENT) * m_allocated);
      memset(&m_elements[pos], 0, sizeof(WAIT_QUEUE_ELEMENT) * ALLOCATION_STEP);
   }
   else
   {
      for(pos = 0; m_elements[pos].msg != NULL; pos++);
   }

   m_elements[pos].code = pMsg->code;
   m_elements[pos].isBinary = 1;
   m_elements[pos].id = pMsg->id;
   m_elements[pos].ttl = m_holdTime;
   m_elements[pos].msg = pMsg;
   m_size++;

#ifdef _WIN32
   for(int i = 0; i < MAX_MSGQUEUE_WAITERS; i++)
      if (m_waiters[i])
         SetEvent(m_wakeupEvents[i]);
#else
   pthread_cond_broadcast(&m_wakeupCondition);
#endif

   unlock();
}

/**
 * Wait for message with specific code and ID
 * Function return pointer to the message on success or
 * NULL on timeout or error
 */
void *MsgWaitQueue::waitForMessageInternal(UINT16 isBinary, UINT16 wCode, UINT32 dwId, UINT32 dwTimeOut)
{
   lock();

#ifdef _WIN32
   int slot = -1;
#endif

   do
   {
      for(int i = 0; i < m_allocated; i++)
		{
         if ((m_elements[i].msg != NULL) && 
             (m_elements[i].id == dwId) &&
             (m_elements[i].code == wCode) &&
             (m_elements[i].isBinary == isBinary))
         {
            void *msg = m_elements[i].msg;
            m_elements[i].msg = NULL;
            m_size--;
#ifdef _WIN32
            if (slot != -1)
               m_waiters[slot] = 0;    // release waiter slot
#endif
            unlock();
            return msg;
         }
		}

      INT64 startTime = GetCurrentTimeMs();
       
#ifdef _WIN32
      // Find free slot if needed
      if (slot == -1)
      {
         for(slot = 0; slot < MAX_MSGQUEUE_WAITERS; slot++)
            if (!m_waiters[slot])
            {
               m_waiters[slot] = 1;
               if (m_wakeupEvents[slot] == NULL)
                  m_wakeupEvents[slot] = CreateEvent(NULL, FALSE, FALSE, NULL);
               break;
            }

         if (slot == MAX_MSGQUEUE_WAITERS)
         {
            slot = -1;
         }
      }

      LeaveCriticalSection(&m_mutex);
      if (slot != -1)
         WaitForSingleObject(m_wakeupEvents[slot], dwTimeOut);
      else
         Sleep(50);  // Just sleep if there are no waiter slots (highly unlikely during normal operation)
      EnterCriticalSection(&m_mutex);
#else
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP || defined(_NETWARE)
	   struct timespec ts;

	   ts.tv_sec = dwTimeOut / 1000;
	   ts.tv_nsec = (dwTimeOut % 1000) * 1000000;
#ifdef _NETWARE
	   pthread_cond_timedwait(&m_wakeupCondition, &m_mutex, &ts);
#else
      pthread_cond_reltimedwait_np(&m_wakeupCondition, &m_mutex, &ts);
#endif
#else
	   struct timeval now;
	   struct timespec ts;

	   gettimeofday(&now, NULL);
	   ts.tv_sec = now.tv_sec + (dwTimeOut / 1000);

	   now.tv_usec += (dwTimeOut % 1000) * 1000;
	   ts.tv_sec += now.tv_usec / 1000000;
	   ts.tv_nsec = (now.tv_usec % 1000000) * 1000;

	   pthread_cond_timedwait(&m_wakeupCondition, &m_mutex, &ts);
#endif   /* HAVE_PTHREAD_COND_RELTIMEDWAIT_NP */
#endif   /* _WIN32 */

      UINT32 sleepTime = (UINT32)(GetCurrentTimeMs() - startTime);
      dwTimeOut -= min(sleepTime, dwTimeOut);
   } while(dwTimeOut > 0);

#ifdef _WIN32
   if (slot != -1)
      m_waiters[slot] = 0;    // release waiter slot
#endif

   unlock();
   return NULL;
}

/**
 * Housekeeping thread
 */
void MsgWaitQueue::housekeeperThread()
{
   while(!ConditionWait(m_stopCondition, TTL_CHECK_INTERVAL))
   {
      lock();
      if (m_size > 0)
      {
         for(int i = 0; i < m_allocated; i++)
		   {
            if (m_elements[i].msg == NULL)
               continue;

            if (m_elements[i].ttl <= TTL_CHECK_INTERVAL)
            {
               if (m_elements[i].isBinary)
               {
                  safe_free(m_elements[i].msg);
               }
               else
               {
                  delete (NXCPMessage *)(m_elements[i].msg);
               }
               m_elements[i].msg = NULL;
               m_size--;
            }
            else
            {
               m_elements[i].ttl -= TTL_CHECK_INTERVAL;
            }
		   }
      }
      unlock();
   }
}

/**
 * Housekeeper thread starter
 */
THREAD_RESULT THREAD_CALL MsgWaitQueue::mwqThreadStarter(void *arg)
{
   ((MsgWaitQueue *)arg)->housekeeperThread();
   return THREAD_OK;
}
