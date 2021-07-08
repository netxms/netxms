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
   m_queueFlushThreshold = config.getValueAsUInt(_T("/InfluxDB/QueueFlushThreshold"), 32768);   // Flush after 32K
   m_queueSizeLimit = config.getValueAsUInt(_T("/InfluxDB/QueueSizeLimit"), 4194304);  // 4MB upper limit on queue size
   m_maxCacheWaitTime = config.getValueAsUInt(_T("/InfluxDB/MaxCacheWaitTime"), 30000);
   m_port = static_cast<uint16_t>(config.getValueAsUInt(_T("/InfluxDB/Port"), 0));
   m_lastConnect = 0;
#ifdef _WIN32
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
}

/**
 * Destructor for abstract sender
 */
InfluxDBSender::~InfluxDBSender()
{
   stop();
#ifdef _WIN32
#else
   pthread_mutex_destroy(&m_mutex);
   pthread_cond_destroy(&m_condition);
#endif
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
#else
      pthread_mutex_lock(&m_mutex);
      if (m_queue.length() < m_queueFlushThreshold)
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

      if (m_queue.isEmpty())
      {
#ifdef _WIN32
#else
         pthread_mutex_unlock(&m_mutex);
#endif
         continue;
      }

      char *data = m_queue.getUTF8String();
      m_queue.clear();

#ifdef _WIN32
#else
      pthread_mutex_unlock(&m_mutex);
#endif

      if (!send(data))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Data block send failed"));
         while(!m_shutdown)
         {
            ThreadSleep(10);
            if (send(data))
               break;
         }
      }

      MemFree(data);
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
#else
   pthread_cond_broadcast(&m_condition);
#endif
   ThreadJoin(m_workerThread);
   m_workerThread = INVALID_THREAD_HANDLE;
}

/**
 * Enqueue data
 */
void InfluxDBSender::enqueue(const TCHAR *data)
{
#ifdef _WIN32
#else
   pthread_mutex_lock(&m_mutex);
#endif

   if (m_queue.length() < m_queueSizeLimit)
   {
      m_queue.append(data);
      m_queue.append(_T('\n'));
      if (m_queue.length() >= m_queueFlushThreshold)
      {
#ifdef _WIN32
#else
         pthread_cond_broadcast(&m_condition);
#endif
      }
   }

#ifdef _WIN32
#else
   pthread_mutex_unlock(&m_mutex);
#endif
}
