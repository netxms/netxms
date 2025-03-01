/**
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for ClickHouse
 ** Copyright (C) 2024 Raden Solutions
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
 ** File: sender.cpp
 **/

#include "clickhouse.h"

/**
 * Constructor for ClickHouse sender
 */
ClickHouseSender::ClickHouseSender(const Config& config) : 
    m_hostname(config.getValue(_T("/ClickHouse/Hostname"), _T("localhost"))),
    m_database(config.getValue(_T("/ClickHouse/Database"), _T("netxms"))),
    m_table(config.getValue(_T("/ClickHouse/Table"), _T("metrics"))),
    m_user(config.getValue(_T("/ClickHouse/User"), _T(""))),
    m_password(config.getValue(_T("/ClickHouse/Password"), _T("")))
{
   m_queuedMessages = 0;
   m_messageDrops = 0;
   m_bufferFlushThreshold = config.getValueAsUInt(_T("/ClickHouse/BatchSize"), 1000); // Default batch size
   m_maxCacheWaitTime = config.getValueAsUInt(_T("/ClickHouse/MaxCacheWaitTime"), 30000);
   m_port = static_cast<uint16_t>(config.getValueAsUInt(_T("/ClickHouse/Port"), 9000)); // Default native protocol port
   m_lastConnect = 0;
   m_bufferSize = 0;
   m_maxBufferSize = config.getValueAsUInt(_T("/ClickHouse/MaxBufferSize"), 10000); // Max records in buffer
   m_batchSize = config.getValueAsUInt(_T("/ClickHouse/BatchSize"), 1000); // How many records to insert at once
   
   m_client = nullptr;

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
   
   // Reserve space for better performance
   m_activeBuffer.reserve(m_maxBufferSize);
   m_backendBuffer.reserve(m_maxBufferSize);
}

/**
 * Destructor for ClickHouse sender
 */
ClickHouseSender::~ClickHouseSender()
{
   stop();
#ifdef _WIN32
   DeleteCriticalSection(&m_mutex);
#else
   pthread_mutex_destroy(&m_mutex);
   pthread_cond_destroy(&m_condition);
#endif
}

/**
 * Setup ClickHouse client 
 */
void ClickHouseSender::setupClient()
{
   try
   {
      clickhouse::ClientOptions options;
      
      // Convert String objects to std::string
      char hostname[256];
      wchar_to_utf8(m_hostname, -1, hostname, sizeof(hostname));
      options.SetHost(hostname);
      
      options.SetPort(m_port);
      
      if (!m_user.isEmpty() && !m_password.isEmpty())
      {
         char username[128];
         char password[256];
         
         wchar_to_utf8(m_user, -1, username, sizeof(username));
         
         TCHAR decryptedPassword[256];
         DecryptPassword(m_user, m_password, decryptedPassword, 256);
         wchar_to_utf8(decryptedPassword, -1, password, sizeof(password));
         
         options.SetUser(username);
         options.SetPassword(password);
      }
      
      char dbname[128];
      wchar_to_utf8(m_database, -1, dbname, sizeof(dbname));
      options.SetDefaultDatabase(dbname);
      
      // Set maximum number of connections
      options.SetPingBeforeQuery(true);
      options.SetCompressionMethod(clickhouse::CompressionMethod::LZ4);
      options.SetConnectionTimeout(10); // seconds
      
      m_client = std::make_unique<clickhouse::Client>(options);
      
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Connected to ClickHouse server at %s:%d"), m_hostname.cstr(), m_port);
   }
   catch (const std::exception& e)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to create ClickHouse client: %hs"), e.what());
      m_client = nullptr;
   }
}

/**
 * Connect to ClickHouse
 */
bool ClickHouseSender::connect()
{
   time_t now = time(nullptr);
   if ((m_client == nullptr) && (m_lastConnect + 60 > now))
   {
      return false;  // Attempt to reconnect not more than once per minute
   }
   
   m_lastConnect = now;
   setupClient();
   return m_client != nullptr;
}

/**
 * Send batch of records to ClickHouse
 */
bool ClickHouseSender::sendBatch(const std::vector<MetricRecord>& records)
{
   if (records.empty())
      return true;
      
   if (m_client == nullptr && !connect())
      return false;
   
   try
   {
      // Prepare the block with all columns needed in the metrics table
      clickhouse::Block block;
      
      // Initialize columns
      const size_t numRecords = records.size();
      
      // Prepare columns with array values for all records in the batch
      std::vector<time_t> timestamps;
      std::vector<std::string> names;
      std::vector<std::string> hosts;
      std::vector<std::string> values;
      std::vector<std::string> instances;
      std::vector<std::string> datasources;
      std::vector<std::string> dataclasses;
      std::vector<std::string> datatypes;
      std::vector<std::string> deltatypes;
      std::vector<std::string> relatedobjecttypes;
      
      // Reserve space for better performance
      timestamps.reserve(numRecords);
      names.reserve(numRecords);
      hosts.reserve(numRecords);
      values.reserve(numRecords);
      instances.reserve(numRecords);
      datasources.reserve(numRecords);
      dataclasses.reserve(numRecords);
      datatypes.reserve(numRecords);
      deltatypes.reserve(numRecords);
      relatedobjecttypes.reserve(numRecords);
      
      // Fill in the data from our records
      for (const auto& record : records)
      {
         timestamps.push_back(record.timestamp);
         names.push_back(record.name);
         hosts.push_back(record.host);
         values.push_back(record.value);
         instances.push_back(record.instance);
         datasources.push_back(record.datasource);
         dataclasses.push_back(record.dataclass);
         datatypes.push_back(record.datatype);
         deltatypes.push_back(record.deltatype);
         relatedobjecttypes.push_back(record.relatedobjecttype);
      }
      
      // Add the columns to the block
      block.AppendColumn("timestamp", clickhouse::ColumnDateTime(timestamps));
      block.AppendColumn("name", clickhouse::ColumnString(names));
      block.AppendColumn("host", clickhouse::ColumnString(hosts));
      block.AppendColumn("value", clickhouse::ColumnString(values));
      block.AppendColumn("instance", clickhouse::ColumnString(instances));
      block.AppendColumn("datasource", clickhouse::ColumnString(datasources));
      block.AppendColumn("dataclass", clickhouse::ColumnString(dataclasses));
      block.AppendColumn("datatype", clickhouse::ColumnString(datatypes));
      block.AppendColumn("deltatype", clickhouse::ColumnString(deltatypes));
      block.AppendColumn("relatedobjecttype", clickhouse::ColumnString(relatedobjecttypes));
      
      // Handle tags as a JSON column
      std::vector<std::string> tagsJson;
      tagsJson.reserve(numRecords);
      
      for (const auto& record : records)
      {
         std::string json = "{";
         bool first = true;
         
         for (const auto& tag : record.tags)
         {
            if (!first)
               json += ",";
            
            json += "\"" + tag.first + "\":\"" + tag.second + "\"";
            first = false;
         }
         
         json += "}";
         tagsJson.push_back(json);
      }
      
      block.AppendColumn("tags", clickhouse::ColumnString(tagsJson));
      
      // Get table name in UTF-8
      char tableName[128];
      wchar_to_utf8(m_table, -1, tableName, sizeof(tableName));
      
      // Insert the batch of data
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Sending batch of %d records to ClickHouse"), static_cast<int>(numRecords));
      m_client->Insert(tableName, block);
      
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Successfully inserted %d records into ClickHouse"), static_cast<int>(numRecords));
      return true;
   }
   catch (const std::exception& e)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to send data to ClickHouse: %hs"), e.what());
      
      // Reset client to force reconnection on next attempt
      m_client = nullptr;
      return false;
   }
}

/**
 * Sender worker thread
 */
void ClickHouseSender::workerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Sender thread started"));

   while(!m_shutdown)
   {
#ifdef _WIN32
      EnterCriticalSection(&m_mutex);
      if (m_bufferSize < m_bufferFlushThreshold)
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
      if (m_bufferSize < m_bufferFlushThreshold)
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

      if (m_activeBuffer.empty())
      {
#ifdef _WIN32
         LeaveCriticalSection(&m_mutex);
#else
         pthread_mutex_unlock(&m_mutex);
#endif
         continue;
      }

      // Swap buffers
      m_backendBuffer.clear();
      m_backendBuffer.swap(m_activeBuffer);
      m_bufferSize = 0;
      m_queuedMessages = 0;

#ifdef _WIN32
      LeaveCriticalSection(&m_mutex);
#else
      pthread_mutex_unlock(&m_mutex);
#endif

      // Send data in smaller batches for better performance
      if (!m_backendBuffer.empty())
      {
         size_t offset = 0;
         while (offset < m_backendBuffer.size())
         {
            // Determine batch size
            const size_t remainingRecords = m_backendBuffer.size() - offset;
            const size_t batchSize = std::min(m_batchSize, remainingRecords);
            
            // Create view into the vector for the current batch
            const std::vector<MetricRecord> batch(
               m_backendBuffer.begin() + offset,
               m_backendBuffer.begin() + offset + batchSize
            );
            
            if (!sendBatch(batch))
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Batch send failed, will retry"));
               bool success = false;
               
               // Retry a few times with delay
               for (int retries = 0; retries < 3 && !m_shutdown; retries++)
               {
                  ThreadSleep(5000);  // 5 second delay between retries
                  if (sendBatch(batch))
                  {
                     success = true;
                     break;
                  }
               }
               
               if (!success && !m_shutdown)
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to send batch after retries, messages dropped: %d"), static_cast<int>(batchSize));
                  lock();
                  m_messageDrops += batchSize;
                  unlock();
               }
            }
            
            offset += batchSize;
         }
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Sender thread stopped"));
}

/**
 * Start sender
 */
void ClickHouseSender::start()
{
   // Try initial connection
   connect();
   m_workerThread = ThreadCreateEx(this, &ClickHouseSender::workerThread);
}

/**
 * Stop sender
 */
void ClickHouseSender::stop()
{
   m_shutdown = true;
#ifdef _WIN32
   WakeAllConditionVariable(&m_condition);
#else
   pthread_cond_broadcast(&m_condition);
#endif
   ThreadJoin(m_workerThread);
   m_workerThread = INVALID_THREAD_HANDLE;
   
   // Close client connection
   m_client = nullptr;
}

/**
 * Enqueue metric record
 */
void ClickHouseSender::enqueue(const MetricRecord& record)
{
   lock();

   if (m_bufferSize < m_maxBufferSize)
   {
      m_activeBuffer.push_back(record);
      m_bufferSize++;
      m_queuedMessages++;
      
      if (m_bufferSize >= m_bufferFlushThreshold)
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
 * Get approximate queue size in bytes
 */
uint64_t ClickHouseSender::getQueueSizeInBytes()
{
   lock();
   // Rough estimate - each record is approximately 200 bytes
   uint64_t s = m_bufferSize * 200;
   unlock();
   return s;
}

/**
 * Get queue size in messages
 */
uint32_t ClickHouseSender::getQueueSizeInMessages()
{
   lock();
   uint32_t s = m_queuedMessages;
   unlock();
   return s;
}

/**
 * Check if queue is full and cannot accept new messages
 */
bool ClickHouseSender::isFull()
{
   lock();
   bool result = (m_bufferSize >= m_maxBufferSize);
   unlock();
   return result;
}

/**
 * Get number of dropped messages
 */
uint64_t ClickHouseSender::getMessageDrops()
{
   lock();
   uint64_t count = m_messageDrops;
   unlock();
   return count;
}