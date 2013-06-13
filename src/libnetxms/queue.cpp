/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: queue.cpp
**
**/

#include "libnetxms.h"
#include <nxqueue.h>

/**
 * Queue constructor
 */
Queue::Queue(UINT32 dwInitialSize, UINT32 dwBufferIncrement)
{
   m_dwBufferSize = dwInitialSize;
   m_dwBufferIncrement = dwBufferIncrement;
	CommonInit();
}

/**
 * Default queue constructor
 */
Queue::Queue()
{
   m_dwBufferSize = 256;
   m_dwBufferIncrement = 32;
	CommonInit();
}


//
// Common initialization (used by all constructors)
//

void Queue::CommonInit()
{
   m_mutexQueueAccess = MutexCreate();
   m_condWakeup = ConditionCreate(FALSE);
   m_dwNumElements = 0;
   m_dwFirst = 0;
   m_dwLast = 0;
   m_pElements = (void **)malloc(sizeof(void *) * m_dwBufferSize);
	m_bShutdownFlag = FALSE;
}


//
// Destructor
//

Queue::~Queue()
{
   MutexDestroy(m_mutexQueueAccess);
   ConditionDestroy(m_condWakeup);
   safe_free(m_pElements);
}


//
// Put new element into queue
//

void Queue::Put(void *pElement)
{
   Lock();
   if (m_dwNumElements == m_dwBufferSize)
   {
      // Extend buffer
      m_dwBufferSize += m_dwBufferIncrement;
      m_pElements = (void **)realloc(m_pElements, sizeof(void *) * m_dwBufferSize);
      
      // Move free space
      memmove(&m_pElements[m_dwFirst + m_dwBufferIncrement], &m_pElements[m_dwFirst],
              sizeof(void *) * (m_dwBufferSize - m_dwFirst - m_dwBufferIncrement));
      m_dwFirst += m_dwBufferIncrement;
   }
   m_pElements[m_dwLast++] = pElement;
   if (m_dwLast == m_dwBufferSize)
      m_dwLast = 0;
   m_dwNumElements++;
   ConditionSet(m_condWakeup);
   Unlock();
}


//
// Insert new element into the beginning of a queue
//

void Queue::Insert(void *pElement)
{
   Lock();
   if (m_dwNumElements == m_dwBufferSize)
   {
      // Extend buffer
      m_dwBufferSize += m_dwBufferIncrement;
      m_pElements = (void **)realloc(m_pElements, sizeof(void *) * m_dwBufferSize);
      
      // Move free space
      memmove(&m_pElements[m_dwFirst + m_dwBufferIncrement], &m_pElements[m_dwFirst],
              sizeof(void *) * (m_dwBufferSize - m_dwFirst - m_dwBufferIncrement));
      m_dwFirst += m_dwBufferIncrement;
   }
   if (m_dwFirst == 0)
      m_dwFirst = m_dwBufferSize;
   m_pElements[--m_dwFirst] = pElement;
   m_dwNumElements++;
   ConditionSet(m_condWakeup);
   Unlock();
}


//
// Get object from queue. Return NULL if queue is empty
//

void *Queue::Get()
{
   void *pElement = NULL;

   Lock();
	if (m_bShutdownFlag)
	{
		pElement = INVALID_POINTER_VALUE;
	}
	else
   {
		while((m_dwNumElements > 0) && (pElement == NULL))
		{
			pElement = m_pElements[m_dwFirst++];
			if (m_dwFirst == m_dwBufferSize)
				m_dwFirst = 0;
			m_dwNumElements--;
		}
   }
   Unlock();
   return pElement;
}


//
// Get object from queue or block if queue if empty
//

void *Queue::GetOrBlock()
{
   void *pElement;

   pElement = Get();
   if (pElement != NULL)
   {
      return pElement;
   }

   do
   {
      ConditionWait(m_condWakeup, INFINITE);
      pElement = Get();
   } while(pElement == NULL);
   return pElement;
}


//
// Clear queue
//

void Queue::Clear()
{
   Lock();
   m_dwNumElements = 0;
   m_dwFirst = 0;
   m_dwLast = 0;
   Unlock();
}


//
// Set shutdown flag
// When this flag is set, Get() always return INVALID_POINTER_VALUE
//

void Queue::SetShutdownMode()
{
	Lock();
	m_bShutdownFlag = TRUE;
	ConditionSet(m_condWakeup);
	Unlock();
}


//
// Find element in queue using given key and comparator
// Returns pointer to element or NULL if element was not found.
// Element remains in the queue
//

void *Queue::find(void *key, QUEUE_COMPARATOR comparator)
{
	void *element = NULL;
	UINT32 i, pos;

	Lock();
	for(i = 0, pos = m_dwFirst; i < m_dwNumElements; i++)
	{
		if ((m_pElements[pos] != NULL) && comparator(key, m_pElements[pos]))
		{
			element = m_pElements[pos];
			break;
		}
		pos++;
		if (pos == m_dwBufferSize)
			pos = 0;
	}
	Unlock();
	return element;
}


//
// Find element in queue using given key and comparator
// Returns pointer to element or NULL if element was not found.
// Element remains in the queue
//

bool Queue::remove(void *key, QUEUE_COMPARATOR comparator)
{
	bool success = false;
	UINT32 i, pos;

	Lock();
	for(i = 0, pos = m_dwFirst; i < m_dwNumElements; i++)
	{
		if ((m_pElements[pos] != NULL) && comparator(key, m_pElements[pos]))
		{
			m_pElements[pos] = NULL;
			success = true;
			break;
		}
		pos++;
		if (pos == m_dwBufferSize)
			pos = 0;
	}
	Unlock();
	return success;
}
