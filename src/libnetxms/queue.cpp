/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
Queue::Queue(size_t initialSize, size_t bufferIncrement, bool owner)
{
   m_initialSize = initialSize;
   m_bufferSize = initialSize;
   m_bufferIncrement = bufferIncrement;
   m_owner = owner;
	commonInit();
}

/**
 * Default queue constructor
 */
Queue::Queue(bool owner)
{
   m_initialSize = 256;
   m_bufferSize = 256;
   m_bufferIncrement = 32;
   m_owner = owner;
	commonInit();
}

/**
 * Common initialization (used by all constructors)
 */
void Queue::commonInit()
{
#ifdef _WIN32
   InitializeCriticalSectionAndSpinCount(&m_lock, 4000);
   InitializeConditionVariable(&m_wakeupCondition);
#else

#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
   pthread_mutexattr_t a;
   pthread_mutexattr_init(&a);
   MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
   pthread_mutex_init(&m_lock, &a);
   pthread_mutexattr_destroy(&a);
#else
   pthread_mutex_init(&m_lock, NULL);
#endif

   pthread_cond_init(&m_wakeupCondition, NULL);

#endif

   m_numElements = 0;
   m_first = 0;
   m_last = 0;
   m_elements = MemAllocArray<void*>(m_bufferSize);
	m_shutdownFlag = false;
	m_destructor = MemFree;
}

/**
 * Destructor
 */
Queue::~Queue()
{
   if (m_owner)
   {
      for(size_t i = 0, pos = m_first; i < m_numElements; i++)
      {
         if (m_elements[pos] != INVALID_POINTER_VALUE)
            m_destructor(m_elements[pos]);
         pos++;
         if (pos == m_bufferSize)
            pos = 0;
      }
   }
   MemFree(m_elements);

#ifdef _WIN32
   DeleteCriticalSection(&m_lock);
#else
   pthread_mutex_destroy(&m_lock);
   pthread_cond_destroy(&m_wakeupCondition);
#endif
}

/**
 * Put new element into queue
 */
void Queue::put(void *element)
{
   lock();
   if (m_numElements == m_bufferSize)
   {
      // Extend buffer
      m_bufferSize += m_bufferIncrement;
      m_elements = MemReallocArray(m_elements, m_bufferSize);
      
      // Move free space
      memmove(&m_elements[m_first + m_bufferIncrement], &m_elements[m_first],
              sizeof(void *) * (m_bufferSize - m_first - m_bufferIncrement));
      m_first += m_bufferIncrement;
   }
   m_elements[m_last++] = element;
   if (m_last == m_bufferSize)
      m_last = 0;
   if (++m_numElements == 1)
   {
#ifdef _WIN32
      WakeConditionVariable(&m_wakeupCondition);
#else
      pthread_cond_signal(&m_wakeupCondition);
#endif
   }
   unlock();
}

/**
 * Insert new element into the beginning of a queue
 */
void Queue::insert(void *pElement)
{
   lock();
   if (m_numElements == m_bufferSize)
   {
      // Extend buffer
      m_bufferSize += m_bufferIncrement;
      m_elements = MemReallocArray(m_elements, m_bufferSize);
      
      // Move free space
      memmove(&m_elements[m_first + m_bufferIncrement], &m_elements[m_first],
              sizeof(void *) * (m_bufferSize - m_first - m_bufferIncrement));
      m_first += m_bufferIncrement;
   }
   if (m_first == 0)
      m_first = m_bufferSize;
   m_elements[--m_first] = pElement;
   if (++m_numElements == 1)
   {
#ifdef _WIN32
      WakeConditionVariable(&m_wakeupCondition);
#else
      pthread_cond_signal(&m_wakeupCondition);
#endif
   }
   unlock();
}

/**
 * Get element from queue. Current thread must own queue lock.
 */
void *Queue::getInternal()
{
   if (m_shutdownFlag)
      return INVALID_POINTER_VALUE;

   void *element = NULL;
   while((m_numElements > 0) && (element == NULL))
   {
      element = m_elements[m_first++];
      if (m_first == m_bufferSize)
         m_first = 0;
      m_numElements--;
   }
   return element;
}

/**
 * Get object from queue. Return NULL if queue is empty
 */
void *Queue::get()
{
   lock();
   void *element = getInternal();
   shrink();
   unlock();
   return element;
}

/**
 * Get object from queue or block with timeout if queue if empty
 */
void *Queue::getOrBlock(UINT32 timeout)
{
   lock();
   void *element = getInternal();
   while(element == NULL)
   {
#ifdef _WIN32
      if (!SleepConditionVariableCS(&m_wakeupCondition, &m_lock, timeout))
         break;
#else
      int rc;
      if (timeout != INFINITE)
      {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
         struct timespec ts;
         ts.tv_sec = timeout / 1000;
         ts.tv_nsec = (timeout % 1000) * 1000000;
         rc = pthread_cond_reltimedwait_np(&m_wakeupCondition, &m_lock, &ts);
#else
         struct timeval now;
         struct timespec ts;
         gettimeofday(&now, NULL);
         ts.tv_sec = now.tv_sec + (timeout / 1000);
         now.tv_usec += (timeout % 1000) * 1000;
         ts.tv_sec += now.tv_usec / 1000000;
         ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
         rc = pthread_cond_timedwait(&m_wakeupCondition, &m_lock, &ts);
#endif
      }
      else
      {
         rc = pthread_cond_wait(&m_wakeupCondition, &m_lock);
      }

      if (rc != 0)
         break;
#endif

      element = getInternal();
   }
   unlock();
   return element;
}

/**
 * Clear queue
 */
void Queue::clear()
{
   lock();
   if (m_owner)
   {
      for(size_t i = 0, pos = m_first; i < m_numElements; i++)
      {
         if (m_elements[pos] != INVALID_POINTER_VALUE)
            m_destructor(m_elements[pos]);
         pos++;
         if (pos == m_bufferSize)
            pos = 0;
      }
   }
   m_numElements = 0;
   m_first = 0;
   m_last = 0;
   shrink();
   unlock();
}

/**
 * Set shutdown flag
 * When this flag is set, Get() always return INVALID_POINTER_VALUE
 */
void Queue::setShutdownMode()
{
	lock();
	m_shutdownFlag = true;
#ifdef _WIN32
   WakeAllConditionVariable(&m_wakeupCondition);
#else
	pthread_cond_broadcast(&m_wakeupCondition);
#endif
	unlock();
}

/**
 * Find element in queue using given key and comparator
 * Returns pointer to element (optionally transformed) or NULL if element was not found.
 * Element remains in the queue
 */
void *Queue::find(const void *key, QueueComparator comparator, void *(*transform)(void*))
{
	void *element = NULL;
	lock();
	for(size_t i = 0, pos = m_first; i < m_numElements; i++)
	{
		if ((m_elements[pos] != NULL) && (m_elements[pos] != INVALID_POINTER_VALUE) && comparator(key, m_elements[pos]))
		{
			element = (transform != NULL) ? transform(m_elements[pos]) : m_elements[pos];
			break;
		}
		pos++;
		if (pos == m_bufferSize)
			pos = 0;
	}
	unlock();
	return element;
}

/**
 * Find element in queue using given key and comparator and remove it.
 * Returns true if element was removed.
 */
bool Queue::remove(const void *key, QueueComparator comparator)
{
	bool success = false;
	lock();
	for(size_t i = 0, pos = m_first; i < m_numElements; i++)
	{
		if ((m_elements[pos] != NULL) && comparator(key, m_elements[pos]))
		{
		   if (m_owner && (m_elements[pos] != INVALID_POINTER_VALUE))
		      m_destructor(m_elements[pos]);
			m_elements[pos] = NULL;
			success = true;
			break;
		}
		pos++;
		if (pos == m_bufferSize)
			pos = 0;
	}
	unlock();
	return success;
}

/**
 * Enumerate queue elements
 */
void Queue::forEach(QueueEnumerationCallback callback, void *context)
{
   lock();
   for(size_t i = 0, pos = m_first; i < m_numElements; i++)
   {
      if ((m_elements[pos] != NULL) && (m_elements[pos] != INVALID_POINTER_VALUE))
      {
         if (callback(m_elements[pos], context) == _STOP)
            break;
      }
      pos++;
      if (pos == m_bufferSize)
         pos = 0;
   }
   unlock();
}

/**
 * Shrink queue if possible
 */
void Queue::shrink()
{
   if ((m_bufferSize == m_initialSize) || (m_numElements > m_initialSize / 2) || ((m_numElements > 0) && (m_last < m_first)))
      return;

   if ((m_numElements > 0) && (m_first > 0))
   {
      memmove(&m_elements[0], &m_elements[m_first], sizeof(void *) * m_numElements);
      m_last -= m_first;
      m_first = 0;
   }
   m_bufferSize = m_initialSize;
   m_elements = MemReallocArray(m_elements, m_bufferSize);
}
