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
#include <netxms-version.h>

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
   m_port = static_cast<uint16_t>(config.getValueAsUInt(_T("/ClickHouse/Port"), 8123)); // Default HTTP protocol port
   m_lastConnect = 0;
   m_bufferSize = 0;
   m_maxBufferSize = config.getValueAsUInt(_T("/ClickHouse/MaxBufferSize"), 10000); // Max records in buffer
   m_batchSize = config.getValueAsUInt(_T("/ClickHouse/BatchSize"), 1000); // How many records to insert at once
   m_useHttps = config.getValueAsBoolean(_T("/ClickHouse/UseHTTPS"), false);
   m_curl = nullptr;
   m_url[0] = 0;
   
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
   
   // Initialize libcurl
   InitializeLibCURL();
}

/**
 * Destructor for ClickHouse sender
 */
ClickHouseSender::~ClickHouseSender()
{
   stop();
   
   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);
   
#ifdef _WIN32
   DeleteCriticalSection(&m_mutex);
#else
   pthread_mutex_destroy(&m_mutex);
   pthread_cond_destroy(&m_condition);
#endif
}

/**
 * Initialize and build the ClickHouse HTTP URL
 */
bool ClickHouseSender::buildURL()
{
   StringBuffer sb(m_useHttps ? _T("https://") : _T("http://"));
   sb.append(m_hostname);
   sb.append(_T(':'));
   sb.append(m_port);
   sb.append(_T("/?query="));
   
   // Build query for inserting data
   StringBuffer query;
   query.append(_T("INSERT INTO "));
   query.append(m_database);
   query.append(_T("."));
   query.append(m_table);
   query.append(_T(" FORMAT JSONEachRow"));
   
   // URL encode the query
   char rawQuery[1024];
   wchar_to_utf8(query, -1, rawQuery, sizeof(rawQuery));
   
   char *encodedQuery = curl_easy_escape(m_curl, rawQuery, 0);
   if (encodedQuery != nullptr)
   {
      sb.appendMBString(encodedQuery);
      curl_free(encodedQuery);
   }
   else
   {
      // Fallback if encoding fails
      sb.appendMBString(rawQuery);
   }
   
   return wchar_to_utf8(sb, -1, m_url, sizeof(m_url)) > 0;
}

/**
 * Initialize cURL handle
 */
bool ClickHouseSender::initCurl()
{
   time_t now = time(nullptr);
   if ((m_curl == nullptr) && (m_lastConnect + 60 > now))
   {
      return false;  // Attempt to reconnect not more than once per minute
   }
   
   m_lastConnect = now;
   
   if (m_curl != nullptr)
   {
      curl_easy_cleanup(m_curl);
   }
   
   m_curl = curl_easy_init();
   if (m_curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return false;
   }
   
   // Build URL for this connection
   if (!buildURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to build ClickHouse URL"));
      curl_easy_cleanup(m_curl);
      m_curl = nullptr;
      return false;
   }
   
   // Common handle setup
#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
#endif

   curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
   curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L); // do not include header in data
   curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 30L); // 30 second timeout
   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
   curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "NetXMS ClickHouse Driver/" NETXMS_VERSION_STRING_A);
   
   // Set the URL
   curl_easy_setopt(m_curl, CURLOPT_URL, m_url);
   
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Connected to ClickHouse server at %s:%d via HTTP%s"),
                 m_hostname.cstr(), m_port, m_useHttps ? _T("S") : _T(""));
   
   EnableLibCURLUnexpectedEOFWorkaround(m_curl);
   
   return true;
}

/**
 * Prepare HTTP headers for ClickHouse request
 */
curl_slist* ClickHouseSender::prepareHeaders()
{
   curl_slist *headers = nullptr;
   
   // Add content type header for JSON
   headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
   
   // Add authentication if needed
   if (!m_user.isEmpty() && !m_password.isEmpty())
   {
      TCHAR decryptedPassword[256];
      DecryptPassword(m_user, m_password, decryptedPassword, 256);
      
      char auth[1024];
      char *encodedAuth = static_cast<char*>(MemAlloc(1024));
      
      StringBuffer authString = m_user;
      authString.append(_T(':'));
      authString.append(decryptedPassword);
      
      size_t len = wchar_to_utf8(authString, -1, encodedAuth, 1024);
      base64_encode((BYTE *)encodedAuth, len, auth, 1024);
      MemFree(encodedAuth);
      
      char header[1100];
      snprintf(header, 1100, "Authorization: Basic %s", auth);
      headers = curl_slist_append(headers, header);
   }
   
   return headers;
}

/**
 * Send batch of records to ClickHouse via HTTP
 */
bool ClickHouseSender::sendBatch(const std::vector<MetricRecord>& records)
{
   if (records.empty())
      return true;
      
   if (m_curl == nullptr && !initCurl())
      return false;
   
   try
   {
      // Create JSON formatted data
      StringBuffer jsonData;
      
      // Process each record
      for (const auto& record : records)
      {
         // Format one record as JSON
         jsonData.append(_T("{"));
         
         // Add timestamp
         jsonData.append(_T("\"timestamp\":"));
         jsonData.append(record.timestamp);
         jsonData.append(_T(","));
         
         // Add name
         jsonData.append(_T("\"name\":"));
         char escapedName[512];
         jsonData.append(_T("\""));
         jsonData.appendMBString(record.name.c_str());
         jsonData.append(_T("\","));
         
         // Add host
         jsonData.append(_T("\"host\":\""));
         jsonData.appendMBString(record.host.c_str());
         jsonData.append(_T("\","));
         
         // Add value
         jsonData.append(_T("\"value\":\""));
         jsonData.appendMBString(record.value.c_str());
         jsonData.append(_T("\","));
         
         // Add instance
         jsonData.append(_T("\"instance\":\""));
         jsonData.appendMBString(record.instance.c_str());
         jsonData.append(_T("\","));
         
         // Add datasource
         jsonData.append(_T("\"datasource\":\""));
         jsonData.appendMBString(record.datasource.c_str());
         jsonData.append(_T("\","));
         
         // Add dataclass
         jsonData.append(_T("\"dataclass\":\""));
         jsonData.appendMBString(record.dataclass.c_str());
         jsonData.append(_T("\","));
         
         // Add datatype
         jsonData.append(_T("\"datatype\":\""));
         jsonData.appendMBString(record.datatype.c_str());
         jsonData.append(_T("\","));
         
         // Add deltatype
         jsonData.append(_T("\"deltatype\":\""));
         jsonData.appendMBString(record.deltatype.c_str());
         jsonData.append(_T("\","));
         
         // Add relatedobjecttype
         jsonData.append(_T("\"relatedobjecttype\":\""));
         jsonData.appendMBString(record.relatedobjecttype.c_str());
         jsonData.append(_T("\","));
         
         // Add tags as a JSON object
         jsonData.append(_T("\"tags\":{"));
         bool firstTag = true;
         for (const auto& tag : record.tags)
         {
            if (!firstTag)
               jsonData.append(_T(","));
            
            jsonData.append(_T("\""));
            jsonData.appendMBString(tag.first.c_str());
            jsonData.append(_T("\":\""));
            jsonData.appendMBString(tag.second.c_str());
            jsonData.append(_T("\""));
            
            firstTag = false;
         }
         jsonData.append(_T("}}"));
         
         // Add newline between records
         jsonData.append(_T("\n"));
      }
      
      // Convert string buffer to UTF-8
      char *utf8Data = jsonData.getUTF8String();
      if (utf8Data == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to convert JSON data to UTF-8"));
         return false;
      }
      
      // Set HTTP headers
      curl_slist *headers = prepareHeaders();
      curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
      
      // Set data to send
      curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, utf8Data);
      curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, strlen(utf8Data));
      
      // Prepare buffer for response
      ByteStream responseData(4096);
      curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);
      
      // Set error buffer
      char errorText[CURL_ERROR_SIZE];
      curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, errorText);
      
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Sending batch of %d records to ClickHouse via HTTP"), 
                     static_cast<int>(records.size()));
      
      // Perform the request
      bool success;
      if (curl_easy_perform(m_curl) == CURLE_OK)
      {
         long responseCode;
         curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
         
         // Process response
         nxlog_debug_tag(DEBUG_TAG, 7, _T("HTTP response %03ld, %d bytes data"), 
                       responseCode, static_cast<int>(responseData.size()));
         
         if (responseData.size() > 0)
         {
            responseData.write('\0');
            nxlog_debug_tag(DEBUG_TAG, 7, _T("API response: %hs"), responseData.buffer());
         }
         
         // Consider only 2xx codes as success
         success = (responseCode >= 200) && (responseCode < 300);
         if (!success)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("HTTP request failed with code %d"), static_cast<int>(responseCode));
            // Cleanup handle on error to force reconnect
            curl_easy_cleanup(m_curl);
            m_curl = nullptr;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed (%hs)"), errorText);
         curl_easy_cleanup(m_curl);
         m_curl = nullptr;
         success = false;
      }
      
      // Cleanup
      curl_slist_free_all(headers);
      free(utf8Data);
      
      return success;
   }
   catch (const std::exception& e)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to send data to ClickHouse: %hs"), e.what());
      
      // Reset client to force reconnection on next attempt
      if (m_curl != nullptr)
      {
         curl_easy_cleanup(m_curl);
         m_curl = nullptr;
      }
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
   initCurl();
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
   if (m_curl != nullptr)
   {
      curl_easy_cleanup(m_curl);
      m_curl = nullptr;
   }
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