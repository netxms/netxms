/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2015 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: datacoll.cpp
**
**/

#include "nxagentd.h"

/**
 * Data collection item
 */
class DataCollectionItem : public RefCountObject
{
private:
   UINT64 m_serverId;
   UINT32 m_id;
   INT32 m_pollingInterval;
   TCHAR *m_name;
   BYTE m_type;
   BYTE m_origin;
   UINT16 m_snmpPort;
   InetAddress m_snmpTarget;
   time_t m_lastPollTime;

public:
   DataCollectionItem(UINT64 serverId, NXCPMessage *msg, UINT32 baseId);
   virtual ~DataCollectionItem();

   UINT32 getId() { return m_id; }
   UINT64 getServerId() { return m_serverId; }
   const TCHAR *getName() { return m_name; }
   int getType() { return (int)m_type; }
   int getOrigin() { return (int)m_origin; }

   UINT32 getTimeToNextPoll(time_t now)
   {
      time_t diff = now - m_lastPollTime;
      return (diff >= m_pollingInterval) ? 0 : m_pollingInterval - (UINT32)diff;
   }
};

/**
 * Create data collection item from NXCP mesage
 */
DataCollectionItem::DataCollectionItem(UINT64 serverId, NXCPMessage *msg, UINT32 baseId) : RefCountObject()
{
   m_serverId = serverId;
   m_id = msg->getFieldAsInt32(baseId);
   m_type = (BYTE)msg->getFieldAsUInt16(baseId + 1);
   m_origin = (BYTE)msg->getFieldAsUInt16(baseId + 2);
   m_name = msg->getFieldAsString(baseId + 3);
   m_pollingInterval = msg->getFieldAsInt32(baseId + 4);
   m_lastPollTime = msg->getFieldAsTime(baseId + 5);
   m_snmpTarget = msg->getFieldAsInetAddress(baseId + 6);
   m_snmpPort = msg->getFieldAsUInt16(baseId + 7);
}

/**
 * Data collection item destructor
 */
DataCollectionItem::~DataCollectionItem()
{
   safe_free(m_name);
}

/**
 * Collected data
 */
class DataElement
{
private:
   UINT64 m_serverId;
   UINT32 m_dciId;
   time_t m_timestamp;
   int m_type;
   union
   {
      TCHAR *item;
      StringList *list;
      Table *table;
   } m_value;

public:
   DataElement(DataCollectionItem *dci, const TCHAR *value)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_type = DCO_TYPE_ITEM;
      m_value.item = _tcsdup(value);
   }

   DataElement(DataCollectionItem *dci, StringList *value)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_type = DCO_TYPE_LIST;
      m_value.list = value;
   }

   DataElement(DataCollectionItem *dci, Table *value)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_type = DCO_TYPE_TABLE;
      m_value.table = value;
   }

   ~DataElement()
   {
      switch(m_type)
      {
         case DCO_TYPE_ITEM:
            free(m_value.item);
            break;
         case DCO_TYPE_LIST:
            delete m_value.list;
            break;
         case DCO_TYPE_TABLE:
            delete m_value.table;
            break;
      }
   }
};

/**
 * Data sender queue
 */
static Queue s_dataSenderQueue;

/**
 * Data sender
 */
static THREAD_RESULT THREAD_CALL DataSender(void *arg)
{
   DebugPrintf(INVALID_INDEX, 1, _T("Data sender thread started"));
   while(true)
   {
      DataElement *e = (DataElement *)s_dataSenderQueue.GetOrBlock();
      if (e == INVALID_POINTER_VALUE)
         break;

      delete e;
   }
   DebugPrintf(INVALID_INDEX, 1, _T("Data sender thread stopped"));
   return THREAD_OK;
}

/**
 * Pseudo-session for cached data collection
 */
class VirtualSession : public AbstractCommSession
{
private:
   UINT64 m_serverId;

public:
   VirtualSession(UINT64 serverId) { m_serverId = serverId; }

   virtual bool isMasterServer() { return false; }
   virtual bool isControlServer() { return false; }
   virtual bool canAcceptTraps() { return true; }
   virtual UINT64 getServerId() { return m_serverId; };
   virtual const InetAddress& getServerAddress() { return InetAddress::LOOPBACK; }

   virtual bool isIPv6Aware() { return true; }

   virtual void sendMessage(NXCPMessage *pMsg) { }
   virtual void sendRawMessage(NXCP_MESSAGE *pMsg) { }
   virtual bool sendFile(UINT32 requestId, const TCHAR *file, long offset) { return false; }
   virtual UINT32 openFile(TCHAR *fileName, UINT32 requestId) { return ERR_INTERNAL_ERROR; }
};

/**
 * Collect data from agent
 */
DataElement *CollectDataFromAgent(DataCollectionItem *dci)
{
   VirtualSession session(dci->getServerId());

   DataElement *e = NULL;
   if (dci->getType() == DCO_TYPE_ITEM)
   {
      TCHAR value[MAX_RESULT_LENGTH];
      if (GetParameterValue(INVALID_INDEX, dci->getName(), value, &session) == ERR_SUCCESS)
         e = new DataElement(dci, value);
   }
   else if (dci->getType() == DCO_TYPE_LIST)
   {
      StringList *value = new StringList;
      if (GetListValue(INVALID_INDEX, dci->getName(), value, &session) == ERR_SUCCESS)
         e = new DataElement(dci, value);
   }
   else if (dci->getType() == DCO_TYPE_TABLE)
   {
      Table *value = new Table;
      if (GetTableValue(INVALID_INDEX, dci->getName(), value, &session) == ERR_SUCCESS)
         e = new DataElement(dci, value);
   }

   return e;
}

/**
 * Collect data from SNMP
 */
DataElement *CollectDataFromSNMP(DataCollectionItem *dci)
{
   /* TODO: implement SNMP data collection */
   return NULL;
}

/**
 * List of all data collection items
 */
static ObjectArray<DataCollectionItem> s_items(64, 64, false);
static MUTEX s_itemLock = INVALID_MUTEX_HANDLE;

/**
 * Single data collection run - collect data if needed and calculate sleep time
 */
static UINT32 DataCollectionRun()
{
   time_t now = time(NULL);
   UINT32 sleepTime = 60;
   
   MutexLock(s_itemLock);
   for(int i = 0; i < s_items.size(); i++)
   {
      DataCollectionItem *dci = s_items.get(i);
      UINT32 timeToPoll = dci->getTimeToNextPoll(now);
      if (timeToPoll == 0)
      {
         DebugPrintf(INVALID_INDEX, 7, _T("DataCollector: polling DCI %d \"%s\""), dci->getId(), dci->getName());
         DataElement *e;
         if (dci->getOrigin() == DS_NATIVE_AGENT)
         {
            e = CollectDataFromAgent(dci);
         }
         else if (dci->getOrigin() == DS_SNMP_AGENT)
         {
            e = CollectDataFromSNMP(dci);
         }
         else
         {
            DebugPrintf(INVALID_INDEX, 7, _T("DataCollector: unsupported origin %d"), dci->getOrigin());
            e = NULL;
         }

         if (e != NULL)
         {
            s_dataSenderQueue.Put(e);
         }
         else
         {
            DebugPrintf(INVALID_INDEX, 6, _T("DataCollector: collection error for DCI %d \"%s\""), dci->getId(), dci->getName());
         }
      }
      else
      {
         if (sleepTime > timeToPoll)
            sleepTime = timeToPoll;
      }
   }
   MutexUnlock(s_itemLock);
   return sleepTime;
}

/**
 * Data collector thread
 */
static THREAD_RESULT THREAD_CALL DataCollector(void *arg)
{
   DebugPrintf(INVALID_INDEX, 1, _T("Data collector thread started"));

   UINT32 sleepTime = DataCollectionRun();
   while(!AgentSleepAndCheckForShutdown(sleepTime))
   {
      sleepTime = DataCollectionRun();
      DebugPrintf(INVALID_INDEX, 7, _T("DataCollector: sleeping for %d seconds"), sleepTime);
   }

   DebugPrintf(INVALID_INDEX, 1, _T("Data collector thread stopped"));
   return THREAD_OK;
}

/**
 * Configure data collection
 */
void ConfigureDataCollection(UINT64 serverId, NXCPMessage *msg)
{
   ObjectArray<DataCollectionItem> config(32, 32, true);

   int count = msg->getFieldAsInt32(VID_NUM_ELEMENTS);
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < count; i++)
   {
      config.add(new DataCollectionItem(serverId, msg, fieldId));
      fieldId += 10;
   }
   DebugPrintf(INVALID_INDEX, 4, _T("%d data collection elements received from server ") UINT64X_FMT(_T("016")), count, serverId);

   /* TODO: config update */
}

/**
 * Data collector and sender thread handles
 */
static THREAD s_dataCollectorThread = INVALID_THREAD_HANDLE;
static THREAD s_dataSenderThread = INVALID_THREAD_HANDLE;

/**
 * Initialize and start local data collector
 */
void StartLocalDataCollector()
{
   /* TODO: database init and configuration load */

   s_itemLock = MutexCreate();
   s_dataCollectorThread = ThreadCreateEx(DataCollector, 0, NULL);
   s_dataSenderThread = ThreadCreateEx(DataSender, 0, NULL);
}

/**
 * Shutdown local data collector
 */
void ShutdownLocalDataCollector()
{
   DebugPrintf(INVALID_INDEX, 5, _T("Waiting for data collector thread termination"));
   ThreadJoin(s_dataCollectorThread);

   DebugPrintf(INVALID_INDEX, 5, _T("Waiting for data sender thread termination"));
   s_dataSenderQueue.Put(INVALID_POINTER_VALUE);
   ThreadJoin(s_dataSenderThread);

   MutexDestroy(s_itemLock);
}
