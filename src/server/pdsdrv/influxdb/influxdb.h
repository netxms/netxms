/**
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for InfluxDB
 ** Copyright (C) 2019 Sebastian YEPES FERNANDEZ & Julien DERIVIERE
 ** Copyright (C) 2021-2025 Raden Solutions
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
 ** File: influxdb.h
 **/

#ifndef _influxdb_h_
#define _influxdb_h_

#include <nms_core.h>
#include <pdsdrv.h>
#include <nxqueue.h>
#include <nxlibcurl.h>

// debug pdsdrv.influxdb 1-8
#define DEBUG_TAG _T("pdsdrv.influxdb")

/**
 * Sender data queue
 */
struct SenderDataQueue
{
   char *data;
   size_t size;
};

/**
 * Abstract sender
 */
class InfluxDBSender
{
private:
   SenderDataQueue m_queues[2];
   SenderDataQueue *m_activeQueue;  // Queue being filled by clients
   SenderDataQueue *m_backendQueue; // Queue being processing by network layer or idle

protected:
   uint32_t m_queueFlushThreshold;
   uint32_t m_queueSizeLimit;
   uint32_t m_maxCacheWaitTime;
   String m_hostname;
   uint16_t m_port;
   time_t m_lastConnect;
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

   virtual bool send(const char *data, size_t size) = 0;

public:
   InfluxDBSender(const Config& config);
   virtual ~InfluxDBSender();

   void start();
   void stop();
   void enqueue(const StringBuffer& data);

   uint64_t getQueueSizeInBytes();
   uint32_t getQueueSizeInMessages();
   uint64_t getMessageDrops();
   bool isFull();
};

/**
 * UDP sender
 */
class UDPSender : public InfluxDBSender
{
protected:
   SOCKET m_socket;

   virtual bool send(const char *data, size_t size) override;

   void createSocket();

public:
   UDPSender(const Config& config);
   virtual ~UDPSender();
};

/**
 * Generic API sender
 */
class APISender : public InfluxDBSender
{
private:
   CURL *m_curl;

protected:
   virtual bool send(const char *data, size_t size) override;

   virtual void buildURL(char *url) = 0;
   virtual void addHeaders(curl_slist **headers);

public:
   APISender(const Config& config);
   virtual ~APISender();
};

/**
 * APIv1 sender
 */
class APIv1Sender : public APISender
{
protected:
   String m_db;
   String m_user;
   String m_password;

   virtual void buildURL(char *url) override;

public:
   APIv1Sender(const Config& config);
   virtual ~APIv1Sender();
};

/**
 * APIv2 sender
 */
class APIv2Sender : public APISender
{
protected:
   String m_organization;
   String m_bucket;
   char *m_token;

   virtual void buildURL(char *url) override;
   virtual void addHeaders(curl_slist **headers) override;

public:
   APIv2Sender(const Config& config);
   virtual ~APIv2Sender();
};

/**
 * Driver class definition
 */
class InfluxDBStorageDriver : public PerfDataStorageDriver
{
private:
   ObjectArray<InfluxDBSender> m_senders;
   bool m_enableUnsignedType;
   bool m_validateValues;
   bool m_correctValues;
   bool m_useTemplateAttributes;

public:
   InfluxDBStorageDriver();
   virtual ~InfluxDBStorageDriver();

   virtual const wchar_t *getName() override;
   virtual bool init(Config *config) override;
   virtual void shutdown() override;
   virtual bool saveDCItemValue(DCItem *dcObject, Timestamp timestamp, const wchar_t *value) override;
   virtual DataCollectionError getInternalMetric(const wchar_t *metric, wchar_t *value) override;
};

#endif   /* _influxdb_h_ */
