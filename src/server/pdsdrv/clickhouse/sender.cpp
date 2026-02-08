/**
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for ClickHouse
 ** Copyright (C) 2025 Raden Solutions
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
 * Write string to byte stream with length encoded as LEB128
 */
static inline void WriteStringWithLength(ByteStream& bs, const char *s)
{
   size_t l = strlen(s);
   bs.writeUnsignedLEB128(l);
   bs.write(s, l);
}

/**
 * Encode column name to byte stream
 */
static void EncodeColumnName(const wchar_t *name, const char *type, ByteStream& names, ByteStream& types, uint32_t *count, uint32_t flag, uint32_t *flags)
{
   if (!wcsicmp(name, L"none") && (flag != 0))
      return;  // Skip disabled non-mandatory column

   char utf8name[256];
   size_t bytes = wchar_to_utf8(name, -1, utf8name, 256);
   names.writeUnsignedLEB128(bytes - 1);  // exclude terminating 0
   names.write(utf8name, bytes - 1);
   WriteStringWithLength(types, type);
   (*count)++;
   (*flags) |= flag;
}

/**
 * Constructor for ClickHouse sender
 */
ClickHouseSender::ClickHouseSender(const Config& config, const StructArray<DataColumn>& dataColumns) : m_dataColumns(dataColumns)
{
   m_queueFlushThreshold = config.getValueAsUInt(L"/ClickHouse/QueueFlushThreshold", 1000);
   m_maxCacheWaitTime = config.getValueAsUInt(L"/ClickHouse/MaxCacheWaitTime", 30000);
   m_queueSizeLimit = config.getValueAsUInt(L"/ClickHouse/QueueSizeLimit", 10000);

   StringBuffer url(config.getValueAsBoolean(L"/ClickHouse/UseHTTPS", false) ? L"https://" : L"http://");
   url.append(config.getValue(L"/ClickHouse/Hostname", L"localhost"));
   url.append(L':');
   url.append(config.getValueAsUInt(L"/ClickHouse/Port", 8123));
   url.append(_T("/?query="));

   StringBuffer query(L"INSERT INTO ");
   query.append(config.getValue(L"/ClickHouse/Database", L"netxms"));
   query.append(L'.');
   query.append(config.getValue(L"/ClickHouse/Table", L"metrics"));
   query.append(_T(" FORMAT RowBinaryWithNamesAndTypes"));
   char utf8string[256];
   wchar_to_utf8(query, -1, utf8string, sizeof(utf8string));
   char *encodedQuery = curl_easy_escape(m_curl, utf8string, 0);
   if (encodedQuery != nullptr)
   {
      url.appendUtf8String(encodedQuery);
      curl_free(encodedQuery);
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Error encoding URL for ClickHouse data sender");
   }

   m_url = WideStringToUtf8(url);

   m_headers = curl_slist_append(nullptr, "Content-Type: application/octet-stream");

   const wchar_t *user = config.getValue(_T("/ClickHouse/User"), _T(""));
   if (*user != 0)
   {
      wchar_t header[256] = L"X-ClickHouse-User: ";
      wcslcat(header, user, 256);
      m_headers = curl_slist_append(m_headers, WideStringToUtf8(header).c_str());
   }

   const wchar_t *password = config.getValue(L"/ClickHouse/Password", L"");
   if (*password != 0)
   {
      wchar_t header[256] = L"X-ClickHouse-Key: ";
      wcslcat(header, password, 256);
      m_headers = curl_slist_append(m_headers, WideStringToUtf8(header).c_str());
   }

   // Standard columns
   m_numColumns = 0;
   m_standardColumnFlags = 0;
   ByteStream header, types;
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.Timestamp", L"timestamp"), "DateTime", header, types, &m_numColumns, 0, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.Host", L"host"), "String", header, types, &m_numColumns, 0, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.Name", L"name"), "String", header, types, &m_numColumns, 0, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.IntValue", L"ivalue"), "Int64", header, types, &m_numColumns, COL_IVALUE, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.FloatValue", L"fvalue"), "Float64", header, types, &m_numColumns, COL_FVALUE, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.Instance", L"instance"), "String", header, types, &m_numColumns, COL_INSTANCE, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.DataType", L"data_type"), "String", header, types, &m_numColumns, COL_DATA_TYPE, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.DataSource", L"data_source"), "String", header, types, &m_numColumns, COL_DATA_SOURCE, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.DeltaType", L"delta_type"), "String", header, types, &m_numColumns, COL_DELTA_TYPE, &m_standardColumnFlags);
   EncodeColumnName(config.getValue(L"/ClickHouse/Column.Tags", L"tags"), "Map(String, String)", header, types, &m_numColumns, COL_TAGS, &m_standardColumnFlags);

   // User-defined columns
   for(int i = 0; i < dataColumns.size(); i++)
   {
      const DataColumn *dc = dataColumns.get(i);
      EncodeColumnName(dc->name, dc->typeName, header, types, &m_numColumns, 0, &m_standardColumnFlags);
   }

   header.write(types.buffer(), types.size());
   m_columnsHeaderSize = header.size();
   m_columnsHeader = MemCopyBlock(header.buffer(), m_columnsHeaderSize);

   m_curl = nullptr;
   m_lastConnect = 0;
   m_messageDrops = 0;
   m_shutdown = false;
   m_workerThread = INVALID_THREAD_HANDLE;
   
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

   // Reserve space for better performance
   m_activeQueue.reserve(m_queueFlushThreshold);
   m_backendQueue.reserve(m_queueFlushThreshold);
}

/**
 * Destructor for ClickHouse sender
 */
ClickHouseSender::~ClickHouseSender()
{
   stop();

   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);
   curl_slist_free_all(m_headers);

   MemFree(m_columnsHeader);

#ifdef _WIN32
   DeleteCriticalSection(&m_mutex);
#else
   pthread_mutex_destroy(&m_mutex);
   pthread_cond_destroy(&m_condition);
#endif
}

/**
 * Initialize cURL handle
 */
bool ClickHouseSender::setupCurlHandle()
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
   curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);

   EnableLibCURLUnexpectedEOFWorkaround(m_curl);

   return true;
}

/**
 * Send batch of records to ClickHouse via HTTP using RowBinary format
 */
bool ClickHouseSender::sendBatch(const std::vector<MetricRecord>& records)
{
   if (m_curl == nullptr && !setupCurlHandle())
      return false;

   ByteStream packet(records.size() * 256); // Pre-allocate with rough estimate

   // Column names
   packet.writeUnsignedLEB128(m_numColumns);
   packet.write(m_columnsHeader, m_columnsHeaderSize);

   for (const MetricRecord& record : records)
   {
      packet.writeL(static_cast<uint32_t>(record.timestamp.asTime()));
      WriteStringWithLength(packet, record.host.c_str());
      WriteStringWithLength(packet, record.name.c_str());
      if (m_standardColumnFlags & COL_IVALUE)
      {
         int64_t v = strtoll(record.value.c_str(), nullptr, 0);
         packet.writeL(v);
      }
      if (m_standardColumnFlags & COL_FVALUE)
         packet.writeL(strtod(record.value.c_str(), nullptr));
      if (m_standardColumnFlags & COL_INSTANCE)
         WriteStringWithLength(packet, record.instance.c_str());
      if (m_standardColumnFlags & COL_DATA_TYPE)
         WriteStringWithLength(packet, record.dataType);
      if (m_standardColumnFlags & COL_DATA_SOURCE)
         WriteStringWithLength(packet, record.dataSource);
      if (m_standardColumnFlags & COL_DELTA_TYPE)
         WriteStringWithLength(packet, record.deltaType);

      if (m_standardColumnFlags & COL_TAGS)
      {
         packet.writeUnsignedLEB128(record.tags.size());
         for (const auto& tag : record.tags)
         {
            WriteStringWithLength(packet, tag.first.c_str());
            WriteStringWithLength(packet, tag.second.c_str());
         }
      }

      for(int i = 0; i < m_dataColumns.size(); i++)
      {
         const DataColumn *dc = m_dataColumns.get(i);
         switch(dc->type)
         {
            case ColumnDataType::Float32:
               packet.writeL(static_cast<float>(strtof(record.columns[i].c_str(), nullptr)));
               break;
            case ColumnDataType::Float64:
               packet.writeL(strtof(record.columns[i].c_str(), nullptr));
               break;
            case ColumnDataType::Int8:
               packet.write(static_cast<int8_t>(strtol(record.columns[i].c_str(), nullptr, 0)));
               break;
            case ColumnDataType::Int16:
               packet.writeL(static_cast<int16_t>(strtol(record.columns[i].c_str(), nullptr, 0)));
               break;
            case ColumnDataType::Int32:
               packet.writeL(static_cast<int32_t>(strtol(record.columns[i].c_str(), nullptr, 0)));
               break;
            case ColumnDataType::Int64:
               packet.writeL(static_cast<int64_t>(strtoll(record.columns[i].c_str(), nullptr, 0)));
               break;
            case ColumnDataType::UInt8:
               packet.write(static_cast<uint8_t>(strtoul(record.columns[i].c_str(), nullptr, 0)));
               break;
            case ColumnDataType::UInt16:
               packet.writeL(static_cast<uint16_t>(strtoul(record.columns[i].c_str(), nullptr, 0)));
               break;
            case ColumnDataType::UInt32:
               packet.writeL(static_cast<uint32_t>(strtoul(record.columns[i].c_str(), nullptr, 0)));
               break;
            case ColumnDataType::UInt64:
               packet.writeL(static_cast<uint64_t>(strtoull(record.columns[i].c_str(), nullptr, 0)));
               break;
            case ColumnDataType::String:
               WriteStringWithLength(packet, record.columns[i].c_str());
               break;
         }
      }
   }

   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, packet.buffer());
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, packet.size());

   ByteStream responseData(4096);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

   char errorText[CURL_ERROR_SIZE];
   curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, errorText);

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Sending batch of %u records to ClickHouse server (%u bytes)"), static_cast<uint32_t>(records.size()), static_cast<uint32_t>(packet.size()));

   // Perform the request
   bool success;
   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      long responseCode;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);

      // Process response
      nxlog_debug_tag(DEBUG_TAG, 7, _T("HTTP response %03ld, %d bytes data"), responseCode, static_cast<int>(responseData.size()));

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

   return success;
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
      if (m_activeQueue.size() < m_queueFlushThreshold)
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
      if (m_activeQueue.size() < m_queueFlushThreshold)
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

      if (m_activeQueue.empty())
      {
#ifdef _WIN32
         LeaveCriticalSection(&m_mutex);
#else
         pthread_mutex_unlock(&m_mutex);
#endif
         continue;
      }

      // Swap buffers
      m_backendQueue.clear();
      m_backendQueue.swap(m_activeQueue);

#ifdef _WIN32
      LeaveCriticalSection(&m_mutex);
#else
      pthread_mutex_unlock(&m_mutex);
#endif

      if (!sendBatch(m_backendQueue))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to send batch (%d messages dropped)"), static_cast<int>(m_backendQueue.size()));
         lock();
         m_messageDrops += m_backendQueue.size();
         unlock();
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Sender thread stopped"));
}

/**
 * Start sender
 */
void ClickHouseSender::start()
{
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
}

/**
 * Enqueue metric record
 */
void ClickHouseSender::enqueue(MetricRecord record)
{
   lock();

   if (m_activeQueue.size() < m_queueSizeLimit)
   {
      m_activeQueue.emplace_back(std::move(record));
      if (m_activeQueue.size() >= m_queueFlushThreshold)
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
 * Get queue size in messages
 */
uint32_t ClickHouseSender::getQueueSize()
{
   lock();
   uint32_t s = static_cast<uint32_t>(m_activeQueue.size());
   unlock();
   return s;
}

/**
 * Check if queue is full and cannot accept new messages
 */
bool ClickHouseSender::isFull()
{
   lock();
   bool result = (m_activeQueue.size() >= m_queueSizeLimit);
   unlock();
   return result;
}
