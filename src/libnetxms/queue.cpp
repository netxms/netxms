/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
 * Internal queue buffer
 */
struct QueueBuffer
{
   QueueBuffer *next;
   size_t head;
   size_t tail;
   size_t count;
   void *elements[1];   // actual size determined by Queue class
};

/**
 * Default object destructor
 */
static void DefaultElementDestructor(void *element, Queue *queue)
{
   MemFree(element);
}

/**
 * Queue constructor
 */
Queue::Queue(size_t blockSize, Ownership owner)
{
   m_blockSize = blockSize;
   m_owner = (owner == Ownership::True);
   commonInit();
}

/**
 * Default queue constructor
 */
Queue::Queue()
{
   m_blockSize = 256;
   m_owner = false;
   commonInit();
}

/**
 * Common initialization (used by all constructors)
 */
void Queue::commonInit()
{
#if defined(_WIN32)
   InitializeCriticalSectionAndSpinCount(&m_lock, 4000);
   InitializeConditionVariable(&m_wakeupCondition);
#elif defined(_USE_GNU_PTH)
   pth_mutex_init(&m_lock);
   pth_cond_init(&m_wakeupCondition);
#else

#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
   pthread_mutexattr_t a;
   pthread_mutexattr_init(&a);
   MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
   pthread_mutex_init(&m_lock, &a);
   pthread_mutexattr_destroy(&a);
#else
   pthread_mutex_init(&m_lock, nullptr);
#endif

   pthread_cond_init(&m_wakeupCondition, nullptr);

#endif

   m_blockCount = 1;
   m_size = 0;
   m_readers = 0;
   m_head = static_cast<QueueBuffer*>(MemAllocZeroed(sizeof(QueueBuffer) + (m_blockSize - 1) * sizeof(void*)));
   m_tail = m_head;
   m_shutdownFlag = false;
   m_destructor = DefaultElementDestructor;
}

/**
 * Destructor
 */
Queue::~Queue()
{
   for(auto buffer = m_head; buffer != nullptr;)
   {
      if (m_owner)
      {
         for(size_t i = 0, pos = buffer->head; i < buffer->count; i++)
         {
            if (buffer->elements[pos] != INVALID_POINTER_VALUE)
               m_destructor(buffer->elements[pos], this);
            pos++;
            if (pos == m_blockSize)
               pos = 0;
         }
      }

      auto next = buffer->next;
      MemFree(buffer);
      buffer = next;
   }

   setShutdownMode();

#if defined(_WIN32)
   DeleteCriticalSection(&m_lock);
#elif defined(_USE_GNU_PTH)
   // No cleanup for mutex or condition
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
   if (m_tail->count == m_blockSize)
   {
      // Allocate new buffer
      m_tail->next = static_cast<QueueBuffer*>(MemAllocZeroed(sizeof(QueueBuffer) + (m_blockSize - 1) * sizeof(void*)));
      m_tail = m_tail->next;
      m_blockCount++;
   }
   m_tail->elements[m_tail->tail++] = element;
   if (m_tail->tail == m_blockSize)
      m_tail->tail = 0;
   m_tail->count++;
   m_size++;
   if (m_readers > 0)
   {
#if defined(_WIN32)
      WakeConditionVariable(&m_wakeupCondition);
#elif defined(_USE_GNU_PTH)
      pth_cond_notify(&m_wakeupCondition, false);
#else
      pthread_cond_signal(&m_wakeupCondition);
#endif
   }
   unlock();
}

/**
 * Insert new element into the beginning of a queue
 */
void Queue::insert(void *element)
{
   lock();
   if (m_head->count == m_blockSize)
   {
      // Allocate new buffer
      auto newHead = static_cast<QueueBuffer*>(MemAllocZeroed(sizeof(QueueBuffer) + (m_blockSize - 1) * sizeof(void*)));
      newHead->next = m_head;
      m_head = newHead;
      m_blockCount++;
   }
   if (m_head->head == 0)
      m_head->head = m_blockSize;
   m_head->elements[--m_head->head] = element;
   m_head->count++;
   m_size++;
   if (m_readers > 0)
   {
#if defined(_WIN32)
      WakeConditionVariable(&m_wakeupCondition);
#elif defined(_USE_GNU_PTH)
      pth_cond_notify(&m_wakeupCondition, false);
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

   void *element = nullptr;
   while((m_size > 0) && (element == nullptr))
   {
      element = m_head->elements[m_head->head++];
      if (m_head->head == m_blockSize)
         m_head->head = 0;
      m_size--;
      m_head->count--;
      if ((m_head->count == 0) && (m_head->next != nullptr))
      {
         auto tmp = m_head;
         m_head = m_head->next;
         MemFree(tmp);
         m_blockCount--;
      }
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
   unlock();
   return element;
}

/**
 * Get object from queue or block with timeout if queue if empty
 */
void *Queue::getOrBlock(uint32_t timeout)
{
   lock();
   m_readers++;
   void *element = getInternal();
   while(element == nullptr)
   {
#if defined(_WIN32)
      if (!SleepConditionVariableCS(&m_wakeupCondition, &m_lock, timeout))
         break;
#elif defined(_USE_GNU_PTH)
      pth_event_t ev = pth_event(PTH_EVENT_TIME, pth_timeout(timeout / 1000, (timeout % 1000) * 1000));
      int rc = pth_cond_await(&m_wakeupCondition, &m_lock, ev);
      if ((rc > 0) && (pth_event_status(ev) == PTH_STATUS_OCCURRED))
         rc = 0;   // Timeout
      pth_event_free(ev, PTH_FREE_ALL);
      if (rc == 0)
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
         gettimeofday(&now, nullptr);
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
   m_readers--;
   unlock();
   return element;
}

/**
 * Clear queue
 */
void Queue::clear()
{
   lock();
   for(auto buffer = m_head; buffer != nullptr;)
   {
      if (m_owner)
      {
         for(size_t i = 0, pos = buffer->head; i < buffer->count; i++)
         {
            if (buffer->elements[pos] != INVALID_POINTER_VALUE)
               m_destructor(buffer->elements[pos], this);
            pos++;
            if (pos == m_blockSize)
               pos = 0;
         }
      }

      if (buffer == m_head)
      {
         buffer = buffer->next;
         m_head->next = nullptr;
         m_head->count = 0;
         m_head->head = 0;
         m_head->tail = 0;
      }
      else
      {
         auto next = buffer->next;
         MemFree(buffer);
         buffer = next;
      }
   }
   m_tail = m_head;
   m_blockCount = 1;
   m_size = 0;
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
   if (m_readers > 0)
   {
#if defined(_WIN32)
      WakeAllConditionVariable(&m_wakeupCondition);
#elif defined(_USE_GNU_PTH)
      pth_cond_notify(&m_wakeupCondition, true);
#else
      pthread_cond_broadcast(&m_wakeupCondition);
#endif
   }
   unlock();
}

/**
 * Find element in queue using given key and comparator
 * Returns pointer to element (optionally transformed) or NULL if element was not found.
 * Element remains in the queue
 */
void *Queue::find(const void *key, QueueComparator comparator, void *(*transform)(void*))
{
	void *element = nullptr;
	lock();
   for(auto buffer = m_head; buffer != nullptr; buffer = buffer->next)
   {
      for(size_t i = 0, pos = buffer->head; i < buffer->count; i++)
      {
         void *curr = buffer->elements[pos];
         if ((curr != nullptr) && (curr != INVALID_POINTER_VALUE) && comparator(key, curr))
         {
            element = (transform != nullptr) ? transform(curr) : curr;
            break;
         }
         pos++;
         if (pos == m_blockSize)
            pos = 0;
      }
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
   for(auto buffer = m_head; buffer != nullptr; buffer = buffer->next)
   {
      for(size_t i = 0, pos = buffer->head; i < buffer->count; i++)
      {
         void *curr = buffer->elements[pos];
         if ((curr != nullptr) && (curr != INVALID_POINTER_VALUE) && comparator(key, curr))
         {
            if (m_owner)
               m_destructor(curr, this);
            buffer->elements[pos] = nullptr;
            success = true;
            goto remove_completed;
         }
         pos++;
         if (pos == m_blockSize)
            pos = 0;
      }
	}
remove_completed:
	unlock();
	return success;
}

/**
 * Enumerate queue elements
 */
void Queue::forEach(QueueEnumerationCallback callback, void *context)
{
   lock();
   for(auto buffer = m_head; buffer != nullptr; buffer = buffer->next)
   {
      for(size_t i = 0, pos = buffer->head; i < buffer->count; i++)
      {
         void *curr = buffer->elements[pos];
         if ((curr != nullptr) && (curr != INVALID_POINTER_VALUE))
         {
            if (callback(curr, context) == _STOP)
               goto stop_enumeration;
         }
         pos++;
         if (pos == m_blockSize)
            pos = 0;
      }
   }
stop_enumeration:
   unlock();
}
