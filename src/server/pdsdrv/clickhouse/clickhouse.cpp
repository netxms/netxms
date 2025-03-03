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
 ** File: clickhouse.cpp
 **/

#include "clickhouse.h"

/**
 * Driver name
 */
static const wchar_t s_driverName[] = L"ClickHouse";

/**
 * Constructor
 */
ClickHouseStorageDriver::ClickHouseStorageDriver() : m_senders(0, 16, Ownership::True)
{
   m_enableUnsignedType = true;
   m_validateValues = false;
   m_correctValues = false;
}

/**
 * Destructor
 */
ClickHouseStorageDriver::~ClickHouseStorageDriver()
{
}

/**
 * Get name
 */
const TCHAR *ClickHouseStorageDriver::getName()
{
   return s_driverName;
}

/**
 * Initialize driver
 */
bool ClickHouseStorageDriver::init(Config *config)
{
   if (!InitializeLibCURL())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"cURL initialization failed");
      return false;
   }

   m_enableUnsignedType = config->getValueAsBoolean(L"/ClickHouse/EnableUnsignedType", m_enableUnsignedType);
   nxlog_debug_tag(DEBUG_TAG, 2, L"Unsigned integer data type is %s", m_enableUnsignedType ? L"enabled" : L"disabled");

   m_validateValues = config->getValueAsBoolean(L"/ClickHouse/ValidateValues", m_validateValues);
   nxlog_debug_tag(DEBUG_TAG, 2, L"Value validation is %s", m_validateValues ? L"enabled" : L"disabled");

   m_correctValues = config->getValueAsBoolean(L"/ClickHouse/CorrectValues", m_correctValues);
   nxlog_debug_tag(DEBUG_TAG, 2, L"Value correction is %s", m_correctValues ? L"enabled" : L"disabled");

   int queueCount = config->getValueAsInt(L"/ClickHouse/Queues", 1);
   if (queueCount < 1)
      queueCount = 1;
   else if (queueCount > 32)
      queueCount = 32;

   nxlog_debug_tag(DEBUG_TAG, 2, L"Using %d queue%s", queueCount, queueCount > 1 ? L"s" : L"");
   for(int i = 0; i < queueCount; i++)
   {
      ClickHouseSender *sender = new ClickHouseSender(*config);
      m_senders.add(sender);
      sender->start();
   }

   return true;
}

/**
 * Shutdown driver
 */
void ClickHouseStorageDriver::shutdown()
{
   for(int i = 0; i < m_senders.size(); i++)
      m_senders.get(i)->stop();
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Shutdown completed"));
}

/**
 * Parse custom attributes and extract tags
 */
static bool GetTagsFromObject(const NetObj& object, std::vector<std::pair<std::string, std::string>> *columns, std::vector<std::pair<std::string, std::string>> *tags)
{
   StringMap *ca = object.getCustomAttributes();
   if (ca == nullptr)
      return false;

   nxlog_debug_tag(DEBUG_TAG, 8, _T("Object: %s - CMA: #%d"), object.getName(), ca->size());

   bool ignoreMetric = false;
   StringList keys = ca->keys();
   for (int i = 0; i < keys.size(); i++)
   {
      const wchar_t *key = keys.get(i);
      if (!wcsicmp(key, L"pds:ignore") || !wcsicmp(key, L"ch:ignore"))
      {
         ignoreMetric = true;
         break;
      }

      // Only include CA's with the prefix tag: or column:
      if (!wcsnicmp(key, L"tag:", 4) || !wcsnicmp(key, L"tag_", 4))
      {
         const wchar_t *value = ca->get(keys.get(i));
         if ((value != nullptr) && (value[0] != 0))
         {
            nxlog_debug_tag(DEBUG_TAG, 8, _T("Object %s: adding tag: \"%s\" = \"%s\""), object.getName(), &key[4], value);
            tags->emplace_back(WideStringToUtf8(&key[4]), WideStringToUtf8(value));
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 8, _T("Object: %s - CA: K:%s (Ignored)"), object.getName(), key);
         }
      }
      else if (!wcsnicmp(key, L"column:", 7))
      {
         const wchar_t *value = ca->get(keys.get(i));
         if ((value != nullptr) && (value[0] != 0))
         {
            nxlog_debug_tag(DEBUG_TAG, 8, _T("Object %s: adding column: \"%s\" = \"%s\""), object.getName(), &key[7], value);
            columns->emplace_back(WideStringToUtf8(&key[7]), WideStringToUtf8(value));
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 8, _T("Object: %s - CA: K:%s (Ignored)"), object.getName(), key);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("Object: %s - CA: K:%s (Ignored)"), object.getName(), key);
      }
   }

   delete ca;
   return ignoreMetric;
}

/**
 * Build and queue metric from item DCI's
 */
bool ClickHouseStorageDriver::saveDCItemValue(DCItem *dci, time_t timestamp, const wchar_t *value)
{
   nxlog_debug_tag(DEBUG_TAG, 8,
            _T("Raw metric: OwnerName:%s DataSource:%i Type:%i Name:%s Description: %s Instance:%s DataType:%i DeltaCalculationMethod:%i RelatedObject:%i Value:%s timestamp:") INT64_FMT,
            dci->getOwnerName(), dci->getDataSource(), dci->getType(), dci->getName().cstr(), dci->getDescription().cstr(),
            dci->getInstanceName().cstr(), dci->getTransformedDataType(), dci->getDeltaCalculationMethod(), dci->getRelatedObject(),
            value, static_cast<int64_t>(timestamp));

   // Don't try to send empty values
   if (*value == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric %s [%u] not sent: empty value"), dci->getName().cstr(), dci->getId());
      return true;
   }

   // Prepare the metric record
   MetricRecord record;
   record.timestamp = timestamp;

   // Data sources
   switch (dci->getDataSource())
   {
      case DS_DEVICE_DRIVER:
         record.dataSource = "device";
         break;
      case DS_INTERNAL:
         record.dataSource = "internal";
         break;
      case DS_MODBUS:
         record.dataSource = "modbus";
         break;
      case DS_MQTT:
         record.dataSource = "mqtt";
         break;
      case DS_NATIVE_AGENT:
         record.dataSource = "agent";
         break;
      case DS_PUSH_AGENT:
         record.dataSource = "push";
         break;
      case DS_SCRIPT:
         record.dataSource = "script";
         break;
      case DS_SMCLP:
         record.dataSource = "smclp";
         break;
      case DS_SNMP_AGENT:
         record.dataSource = "snmp";
         break;
      case DS_SSH:
         record.dataSource = "ssh";
         break;
      case DS_WEB_SERVICE:
         record.dataSource = "websvc";
         break;
      case DS_WINPERF:
         record.dataSource = "wmi";
         break;
      default:
         record.dataSource = "unknown";
         break;
   }

   // Delta calculation types
   switch (dci->getDeltaCalculationMethod())
   {
      case DCM_ORIGINAL_VALUE:
         record.deltaType = "none";
         break;
      case DCM_SIMPLE:
         record.deltaType = "simple";
         break;
      case DCM_AVERAGE_PER_SECOND:
         record.deltaType = "avg_sec";
         break;
      case DCM_AVERAGE_PER_MINUTE:
         record.deltaType = "avg_min";
         break;
      default:
         record.deltaType = "unknown";
         break;
   }

   // DCI (data collection item) data types
   bool isInteger, isUnsigned = false;
   switch (dci->getTransformedDataType())
   {
      case DCI_DT_INT:
         record.dataType = "signed-integer32";
         isInteger = true;
         break;
      case DCI_DT_UINT:
         record.dataType = "unsigned-integer32";
         isInteger = true;
         isUnsigned = true;
         break;
      case DCI_DT_INT64:
         record.dataType = "signed-integer64";
         isInteger = true;
         break;
      case DCI_DT_UINT64:
         record.dataType = "unsigned-integer64";
         isInteger = true;
         isUnsigned = true;
         break;
      case DCI_DT_FLOAT:
         record.dataType = "float";
         isInteger = false;
         break;
      case DCI_DT_STRING:
         record.dataType = "string";
         isInteger = false;
         break;
      case DCI_DT_NULL:
         record.dataType = "null";
         isInteger = false;
         break;
      case DCI_DT_COUNTER32:
         record.dataType = "counter32";
         isInteger = true;
         isUnsigned = true;
         break;
      case DCI_DT_COUNTER64:
         record.dataType = "counter64";
         isInteger = true;
         isUnsigned = true;
         break;
      default:
         record.dataType = "unknown";
         isInteger = false;
         break;
   }

   // Get custom tags from host
   if (GetTagsFromObject(static_cast<NetObj&>(*dci->getOwner()), &record.columns, &record.tags))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, L"Metric not sent: ignore flag set on owner object");
      return true;
   }

   // Get custom tags from related object (if any)
   shared_ptr<NetObj> relatedObject = FindObjectById(dci->getRelatedObject());
   if (relatedObject != nullptr)
   {
      if (GetTagsFromObject(static_cast<NetObj&>(*relatedObject), &record.columns, &record.tags))
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric not sent: ignore flag set on related object %s"), relatedObject->getName());
         return true;
      }
   }

   // Set metric name
   if ((dci->getDataSource() == DS_SNMP_AGENT) || (dci->getDataSource() == DS_MODBUS) ||
       ((dci->getDataSource() == DS_INTERNAL) && !_tcsnicmp(dci->getName(), _T("Dummy"), 5)))
   {
      record.name = WideStringToUtf8(dci->getDescription());
   }
   else
   {
      record.name = WideStringToUtf8(dci->getName());
   }

   // Host (node) name
   record.host = WideStringToUtf8(dci->getOwner()->getName());

   // Validate value
   wchar_t correctedValue[64];
   correctedValue[0] = 0;
   if (m_validateValues && (dci->getTransformedDataType() != DCI_DT_STRING))
   {
      wchar_t *eptr;

      if (isInteger)
      {
         if (isUnsigned && m_enableUnsignedType)
         {
            errno = 0;
            uint64_t u = wcstoull(value, &eptr, 0);
            if ((*eptr != 0) || (errno == ERANGE))
            {
               if (!m_correctValues)
               {
                  nxlog_debug_tag(DEBUG_TAG, 7, L"Metric not sent: value %hs:%hs:%hs=\"%s\" did not pass unsigned integer validation (%s)",
                     record.dataSource, record.host.c_str(), record.name.c_str(), value, (*eptr != 0) ? L"parse error" : L"value is out of range");
                  return true;
               }
               IntegerToString(u, correctedValue);
            }
         }
         else
         {
            errno = 0;
            int64_t i = wcstoll(value, &eptr, 0);
            if ((*eptr != 0) || (errno == ERANGE))
            {
               if (!m_correctValues)
               {
                  nxlog_debug_tag(DEBUG_TAG, 7, L"Metric not sent: value %s:%s:%s=\"%s\" did not pass signed integer validation (%s)",
                     record.dataSource, record.host.c_str(), record.name.c_str(), value, (*eptr != 0) ? L"parse error" : L"value is out of range");
                  return true;
               }
               IntegerToString(i, correctedValue);
            }
         }
      }
      else
      {
         errno = 0;
         double d = _tcstod(value, &eptr);
         if ((*eptr != 0) || (errno == ERANGE))
         {
            if (!m_correctValues)
            {
               nxlog_debug_tag(DEBUG_TAG, 7, L"Metric not sent: value %hs:%hs:%hs=\"%s\" did not pass number validation (%s)",
                  record.dataSource, record.host.c_str(), record.name.c_str(), value, (*eptr != 0) ? L"parse error" : L"value is out of range");
               return true;
            }
            _sntprintf(correctedValue, 64, L"%f", d);
         }
      }

      if (correctedValue[0] != 0)
         nxlog_debug_tag(DEBUG_TAG, 7, L"Value %hs:%hs:%hs=\"%s\" corrected to \"%s\"", record.dataSource, record.host.c_str(), record.name.c_str(), value, correctedValue);
   }

   record.value = WideStringToUtf8((correctedValue[0] != 0) ? correctedValue : value);
   record.instance = WideStringToUtf8(dci->getInstanceName());

   int senderIndex = dci->getId() % m_senders.size();
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Queuing metric record to sender #%d: %hs:%hs:%hs"), senderIndex, record.dataSource, record.host.c_str(), record.name.c_str());

   m_senders.get(senderIndex)->enqueue(std::move(record));
   return true;
}

/**
 * Get driver's internal metrics
 */
DataCollectionError ClickHouseStorageDriver::getInternalMetric(const wchar_t *metric, wchar_t *value)
{
   DataCollectionError rc = DCE_SUCCESS;
   if (!_tcsicmp(metric, L"senders"))
   {
      ret_int(value, m_senders.size());
   }
   else if (!_tcsicmp(metric, L"overflowQueues"))
   {
      uint32_t count = 0;
      for(int i = 0; i < m_senders.size(); i++)
         if (m_senders.get(i)->isFull())
            count++;
      ret_uint(value, count);
   }
   else if (!_tcsicmp(metric, L"messageDrops"))
   {
      uint64_t count = 0;
      for(int i = 0; i < m_senders.size(); i++)
         count += m_senders.get(i)->getMessageDrops();
      ret_uint64(value, count);
   }
   else if (!_tcsicmp(metric, L"queueSize"))
   {
      uint32_t size = 0;
      for(int i = 0; i < m_senders.size(); i++)
         size += m_senders.get(i)->getQueueSize();
      ret_uint(value, size);
   }
   else
   {
      value[0] = 0;
      rc = DCE_NOT_SUPPORTED;
   }
   nxlog_debug_tag(DEBUG_TAG, 7, _T("getInternalMetric(%s): rc=%d, value=%s"), metric, rc, value);
   return rc;
}

/**
 * Driver entry point
 */
DECLARE_PDSDRV_ENTRY_POINT(s_driverName, ClickHouseStorageDriver);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
