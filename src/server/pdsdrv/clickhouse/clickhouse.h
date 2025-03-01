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
 ** File: clickhouse.h
 **/

#ifndef _clickhouse_h_
#define _clickhouse_h_

#include <nms_core.h>
#include <pdsdrv.h>
#include <nxqueue.h>
#include <nxlibcurl.h>
#include <vector>
#include <chrono>

// debug pdsdrv.clickhouse 1-8
#define DEBUG_TAG _T("pdsdrv.clickhouse")

/**
 * Metric record structure
 */
struct MetricRecord
{
   time_t timestamp;
   std::string name;
   std::string host;
   std::string value;
   std::string instance;
   std::string datasource;
   std::string dataclass;
   std::string datatype;
   std::string deltatype;
   std::string relatedobjecttype;
   std::vector<std::pair<std::string, std::string>> tags;
};

/**
 * ClickHouse connection handler using REST API
 */
class ClickHouseSender
{
private:
   std::vector<MetricRecord> m_activeBuffer;
   std::vector<MetricRecord> m_backendBuffer;
   size_t m_bufferSize;
   size_t m_maxBufferSize;
   
   String m_hostname;
   uint16_t m_port;
   String m_database;
   String m_table;
   String m_user;
   String m_password;
   time_t m_lastConnect;
   uint32_t m_maxCacheWaitTime;
   uint32_t m_batchSize;
   
   bool m_useHttps;
   CURL *m_curl;
   char m_url[4096];
   
#ifdef _WIN32
   CRITICAL_SECTION m_mutex;
   CONDITION_VARIABLE m_condition;
#else
   pthread_mutex_t m_mutex;
   pthread_cond_t m_condition;
#endif
   bool m_shutdown;
   THREAD m_workerThread;
   uint64_t m_messageDrops;
   uint32_t m_queuedMessages;
   uint32_t m_bufferFlushThreshold;

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
   bool buildURL();
   bool sendBatch(const std::vector<MetricRecord>& records);
   curl_slist* prepareHeaders();
   bool initCurl();

public:
   ClickHouseSender(const Config& config);
   virtual ~ClickHouseSender();

   void start();
   void stop();
   void enqueue(const MetricRecord& record);

   uint64_t getQueueSizeInBytes();
   uint32_t getQueueSizeInMessages();
   uint64_t getMessageDrops();
   bool isFull();
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

   virtual const TCHAR *getName() override;
   virtual bool init(Config *config) override;
   virtual void shutdown() override;
   virtual bool saveDCItemValue(DCItem *dcObject, time_t timestamp, const TCHAR *value) override;
   virtual bool saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value) override;
   virtual DataCollectionError getInternalMetric(const TCHAR *metric, TCHAR *value) override;
};

#endif   /* _clickhouse_h_ */