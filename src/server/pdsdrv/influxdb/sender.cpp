/**
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for InfluxDB
 ** Copyright (C) 2019 Sebastian YEPES FERNANDEZ & Julien DERIVIERE
 ** Copyright (C) 2021 Raden Solutions
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU Lesser General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
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
 **              ,.-----__
 **          ,:::://///,:::-.
 **         /:''/////// ``:::`;/|/
 **        /'   ||||||     :://'`\
 **      .' ,   ||||||     `/(  e \
 **-===~__-'\__X_`````\_____/~`-._ `.
 **            ~~        ~~       `~-'
 ** Armadillo, The Metric Eater - https://www.nationalgeographic.com/animals/mammals/group/armadillos/
 **
 ** File: sender.cpp
 **/

#include "influxdb.h"

/**
 * Constructor for abstract sender
 */
InfluxDBSender::InfluxDBSender(const Config& config) : m_hostname(config.getValue(_T("/InfluxDB/Hostname"), _T("localhost")))
{
   m_queuedMessages = 0;
   m_messageDrops = 0;
   m_queueFlushThreshold = config.getValueAsUInt(_T("/InfluxDB/QueueFlushThreshold"), 32768);   // Flush after 32K
   m_maxCacheWaitTime = config.getValueAsUInt(_T("/InfluxDB/MaxCacheWaitTime"), 30000);
   m_port = static_cast<uint16_t>(config.getValueAsUInt(_T("/InfluxDB/Port"), 0));
   m_lastConnect = 0;
#ifdef _WIN32
   InitializeCriticalSectionAndSpinCount(&m_mutex, 4000);
   InitializeConditionVariable(&m_condition);
#else
#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
   pthread_mutexattr_t a;
   pthread_mutexattr_init(&a);
   MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
   pthread_mutex_init(&m_mutex, &a);
   pthread_mutexattr_destroy(&a);
#else
   pthread_mutex_init(&m_mutex, nullptr);
#endif
   pthread_cond_init(&m_condition, nullptr);
#endif
   m_shutdown = false;
   m_workerThread = INVALID_THREAD_HANDLE;

   m_queueSizeLimit = config.getValueAsUInt(_T("/InfluxDB/QueueSizeLimit"), 4194304);  // 4MB upper limit on queue size
   m_queues[0].data = static_cast<char*>(MemAlloc(m_queueSizeLimit));
   m_queues[0].size = 0;
   m_queues[1].data = static_cast<char*>(MemAlloc(m_queueSizeLimit));
   m_queues[1].size = 0;
   m_activeQueue = &m_queues[0];
   m_backendQueue = &m_queues[1];
}

/**
 * Destructor for abstract sender
 */
InfluxDBSender::~InfluxDBSender()
{
   stop();
#ifdef _WIN32
   DeleteCriticalSection(&m_mutex);
#else
   pthread_mutex_destroy(&m_mutex);
   pthread_cond_destroy(&m_condition);
#endif
   MemFree(m_queues[0].data);
   MemFree(m_queues[1].data);
}

/**
 * Sender worker thread
 */
void InfluxDBSender::workerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Sender thread started"));

   while(!m_shutdown)
   {
#ifdef _WIN32
      EnterCriticalSection(&m_mutex);
      if (m_activeQueue->size < m_queueFlushThreshold)
      {
         // SleepConditionVariableCS is subject to spurious wakeups so we need a loop here
         BOOL signalled = FALSE;
         uint32_t timeout = m_maxCacheWaitTime;
         do
         {
            int64_t startTime = GetCurrentTimeMs();
            signalled = SleepConditionVariableCS(&m_condition, &m_mutex, timeout);
            if (signalled)
               break;
            timeout -= std::min(timeout, static_cast<uint32_t>(GetCurrentTimeMs() - startTime));
         } while (timeout > 0);
      }
#else
      pthread_mutex_lock(&m_mutex);
      if (m_activeQueue->size < m_queueFlushThreshold)
      {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
         struct timespec timeout;
         timeout.tv_sec = m_maxCacheWaitTime / 1000;
         timeout.tv_nsec = (m_maxCacheWaitTime % 1000) * 1000000;
         pthread_cond_reltimedwait_np(&m_condition, &m_mutex, &timeout);
#else
         struct timeval now;
         gettimeofday(&now, nullptr);

         struct timespec timeout;
         timeout.tv_sec = now.tv_sec + (m_maxCacheWaitTime / 1000);
         now.tv_usec += (m_maxCacheWaitTime % 1000) * 1000;
         timeout.tv_sec += now.tv_usec / 1000000;
         timeout.tv_nsec = (now.tv_usec % 1000000) * 1000;
         pthread_cond_timedwait(&m_condition, &m_mutex, &timeout);
#endif
      }
#endif

      if (m_activeQueue->size == 0)
      {
#ifdef _WIN32
         LeaveCriticalSection(&m_mutex);
#else
         pthread_mutex_unlock(&m_mutex);
#endif
         continue;
      }

      std::swap(m_activeQueue, m_backendQueue);
      m_queuedMessages = 0;

#ifdef _WIN32
      LeaveCriticalSection(&m_mutex);
#else
      pthread_mutex_unlock(&m_mutex);
#endif

      m_backendQueue->data[m_backendQueue->size] = 0;
      if (!send(m_backendQueue->data, m_backendQueue->size))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Data block send failed"));
         while(!m_shutdown)
         {
            ThreadSleep(10);
            if (send(m_backendQueue->data, m_backendQueue->size))
               break;
         }
      }
      m_backendQueue->size = 0;
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Sender thread stopped"));
}

/**
 * Start sender
 */
void InfluxDBSender::start()
{
   m_workerThread = ThreadCreateEx(this, &InfluxDBSender::workerThread);
}

/**
 * Stop sender
 */
void InfluxDBSender::stop()
{
   m_shutdown = true;
#ifdef _WIN32
   WakeAllConditionVariable(&m_condition);
#else
   pthread_cond_broadcast(&m_condition);
#endif
   ThreadJoin(m_workerThread);
   m_workerThread = INVALID_THREAD_HANDLE;
}

/**
 * Enqueue data
 */
void InfluxDBSender::enqueue(const StringBuffer& data)
{
   size_t len = wchar_utf8len(data, data.length()) + 2;  // ensure space for new line and trailing zero byte

   lock();

   if (m_activeQueue->size + len < m_queueSizeLimit)
   {
      len = wchar_to_utf8(data, data.length(), m_activeQueue->data + m_activeQueue->size, len);
      m_activeQueue->size += len;
      m_activeQueue->data[m_activeQueue->size++] = '\n';
      m_queuedMessages++;
      if (m_activeQueue->size >= m_queueFlushThreshold)
      {
#ifdef _WIN32
         WakeAllConditionVariable(&m_condition);
#else
         pthread_cond_broadcast(&m_condition);
#endif
      }
   }
   else
   {
      m_messageDrops++;
   }

   unlock();
}

/**
 * Get queue size in bytes
 */
uint64_t InfluxDBSender::getQueueSizeInBytes()
{
   lock();
   uint64_t s = m_activeQueue->size;
   unlock();
   return s;
}

/**
 * Get queue size in messages
 */
uint32_t InfluxDBSender::getQueueSizeInMessages()
{
   lock();
   uint32_t s = m_queuedMessages;
   unlock();
   return s;
}

/**
 * Check if queue is full and cannot accept new messages
 */
bool InfluxDBSender::isFull()
{
   lock();
   bool result = (m_activeQueue->size >= m_queueSizeLimit);
   unlock();
   return result;
}

/**
 * Get number of dropped messages
 */
uint64_t InfluxDBSender::getMessageDrops()
{
   lock();
   uint64_t count = m_messageDrops;
   unlock();
   return count;
}
