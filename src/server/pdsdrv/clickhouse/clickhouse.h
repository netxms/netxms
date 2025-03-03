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
 ** File: clickhouse.h
 **/

#ifndef _clickhouse_h_
#define _clickhouse_h_

#include <nms_core.h>
#include <pdsdrv.h>
#include <nxqueue.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("pdsdrv.clickhouse")

/**
 * Column enable flags
 */
#define COL_IVALUE         0x01
#define COL_FVALUE         0x02
#define COL_INSTANCE       0x04
#define COL_DATA_SOURCE    0x08
#define COL_DATA_TYPE      0x10
#define COL_DELTA_TYPE     0x20
#define COL_TAGS           0x40

/**
 * Metric record structure
 */
struct MetricRecord
{
   time_t timestamp;
   std::string host;
   std::string name;
   std::string value;
   std::string instance;
   const char *dataSource;
   const char *dataType;
   const char *deltaType;
   std::vector<std::pair<std::string, std::string>> columns;
   std::vector<std::pair<std::string, std::string>> tags;
};

/**
 * ClickHouse connection handler using REST API
 */
class ClickHouseSender
{
private:
   std::vector<MetricRecord> m_activeQueue;
   std::vector<MetricRecord> m_backendQueue;

   size_t m_queueSizeLimit;
   uint32_t m_queueFlushThreshold;
   uint32_t m_maxCacheWaitTime;
   std::string m_url;
   BYTE *m_standardColumnsHeader;
   size_t m_standardColumnsHeaderSize;
   uint32_t m_numStandardColumns;
   uint32_t m_standardColumnFlags;

   CURL *m_curl;
   curl_slist *m_headers;
   time_t m_lastConnect;
   uint64_t m_messageDrops;

#ifdef _WIN32
   CRITICAL_SECTION m_mutex;
   CONDITION_VARIABLE m_condition;
#else
   pthread_mutex_t m_mutex;
   pthread_cond_t m_condition;
#endif
   bool m_shutdown;
   THREAD m_workerThread;

   void lock()
   {
#ifdef _WIN32
      EnterCriticalSection(&m_mutex);
#else
      pthread_mutex_lock(&m_mutex);
#endif
   }

   void unlock()
   {
#ifdef _WIN32
      LeaveCriticalSection(&m_mutex);
#else
      pthread_mutex_unlock(&m_mutex);
#endif
   }

   void workerThread();

   bool setupCurlHandle();
   bool sendBatch(const std::vector<MetricRecord>& records);

public:
   ClickHouseSender(const Config& config);
   virtual ~ClickHouseSender();

   void start();
   void stop();
   void enqueue(MetricRecord record);

   bool isFull();
   uint32_t getQueueSize();
   uint64_t getMessageDrops()
   {
      return m_messageDrops;
   }
};

/**
 * Driver class definition
 */
class ClickHouseStorageDriver : public PerfDataStorageDriver
{
private:
   ObjectArray<ClickHouseSender> m_senders;
   bool m_enableUnsignedType;
   bool m_validateValues;
   bool m_correctValues;

public:
   ClickHouseStorageDriver();
   virtual ~ClickHouseStorageDriver();

   virtual const wchar_t *getName() override;
   virtual bool init(Config *config) override;
   virtual void shutdown() override;
   virtual bool saveDCItemValue(DCItem *dcObject, time_t timestamp, const wchar_t *value) override;
   virtual DataCollectionError getInternalMetric(const wchar_t *metric, wchar_t *value) override;
};

/**
 * Convert wide character string to std::string in UTF-8 encoding
 */
static inline std::string WideStringToUtf8(const wchar_t *ws)
{
   char utf8string[1024];
   wchar_to_utf8(ws, -1, utf8string, sizeof(utf8string));
   return std::string(utf8string);
}

#endif   /* _clickhouse_h_ */
