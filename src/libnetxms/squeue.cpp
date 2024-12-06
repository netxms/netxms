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
** File: squeue.cpp
**
**/

#include "libnetxms.h"
#include <nxqueue.h>

#define DECLARED_DATA_SIZE 8

/**
 * Internal queue buffer
 */
struct QueueBuffer
{
   QueueBuffer *next;
   size_t head;
   size_t tail;
   size_t count;
   BYTE elements[DECLARED_DATA_SIZE];   // actual size determined by SQueue class
};

/**
 * Queue constructor
 */
SQueueBase::SQueueBase(size_t elementSize, size_t blockSize)
{
   m_elementSize = elementSize;
   m_blockSize = blockSize;

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
   m_head = static_cast<QueueBuffer*>(MemAllocZeroed(sizeof(QueueBuffer) + m_blockSize * elementSize - DECLARED_DATA_SIZE));
   m_tail = m_head;
}

/**
 * Destructor
 */
SQueueBase::~SQueueBase()
{
   for(auto buffer = m_head; buffer != nullptr;)
   {
      auto next = buffer->next;
      MemFree(buffer);
      buffer = next;
   }

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
void SQueueBase::put(const void *element)
{
   lock();
   if (m_tail->count == m_blockSize)
   {
      // Allocate new buffer
      m_tail->next = static_cast<QueueBuffer*>(MemAllocZeroed(sizeof(QueueBuffer) + m_blockSize * m_elementSize - DECLARED_DATA_SIZE));
      m_tail = m_tail->next;
      m_blockCount++;
   }
   memcpy(&m_tail->elements[(m_tail->tail++) * m_elementSize], element, m_elementSize);
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
void SQueueBase::insert(const void *element)
{
   lock();
   if (m_head->count == m_blockSize)
   {
      // Allocate new buffer
      auto newHead = static_cast<QueueBuffer*>(MemAllocZeroed(sizeof(QueueBuffer) + m_blockSize * m_elementSize - DECLARED_DATA_SIZE));
      newHead->next = m_head;
      m_head = newHead;
      m_blockCount++;
   }
   if (m_head->head == 0)
      m_head->head = m_blockSize;
   memcpy(&m_head->elements[(--m_head->head) * m_elementSize], element, m_elementSize);
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
 * Get element from queue. Current thread must own queue lock and ensure that queue is not empty.
 */
void SQueueBase::dequeue(void *buffer)
{
   memcpy(buffer, &m_head->elements[(m_head->head++) * m_elementSize], m_elementSize);
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

/**
 * Get object from queue or block with timeout if queue if empty
 */
bool SQueueBase::getOrBlock(void *buffer, uint32_t timeout)
{
   bool success = false;
   lock();
   m_readers++;
   while(m_size == 0)
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
   }

   // If wait loop aborted by timeout m_size will still be 0
   if (m_size > 0)
   {
      dequeue(buffer);
      success = true;
   }

   m_readers--;
   unlock();
   return success;
}

/**
 * Clear queue
 */
void SQueueBase::clear()
{
   lock();
   for(auto buffer = m_head; buffer != nullptr;)
   {
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
