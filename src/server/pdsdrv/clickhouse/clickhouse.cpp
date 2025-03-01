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
 ** File: clickhouse.cpp
 **/

#include "clickhouse.h"

/**
 * Driver name
 */
static const TCHAR *s_driverName = _T("ClickHouse");

/**
 * Constructor
 */
ClickHouseStorageDriver::ClickHouseStorageDriver() : m_senders(0, 16, Ownership::True)
{
   m_enableUnsignedType = false;
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
   m_enableUnsignedType = config->getValueAsBoolean(_T("/ClickHouse/EnableUnsignedType"), m_enableUnsignedType);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Unsigned integer data type is %s"), m_enableUnsignedType ? _T("enabled") : _T("disabled"));

   m_validateValues = config->getValueAsBoolean(_T("/ClickHouse/ValidateValues"), m_validateValues);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Value validation is %s"), m_validateValues ? _T("enabled") : _T("disabled"));

   m_correctValues = config->getValueAsBoolean(_T("/ClickHouse/CorrectValues"), m_correctValues);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Value correction is %s"), m_correctValues ? _T("enabled") : _T("disabled"));

   int queueCount = config->getValueAsInt(_T("/ClickHouse/Queues"), 1);
   if (queueCount < 1)
      queueCount = 1;
   else if (queueCount > 32)
      queueCount = 32;

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Using %d queue%s"), queueCount, queueCount > 1 ? _T("s") : _T(""));
   for(int i = 0; i < queueCount; i++)
   {
      ClickHouseSender *sender = new ClickHouseSender(*config);
      sender->start();
      m_senders.add(sender);
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
 * Normalize all the metric and tag names
 */
static StringBuffer NormalizeString(const TCHAR *src)
{
   StringBuffer dst(src);
   dst.toLowercase();
   dst.trim();
   dst.replace(_T(" "), _T(""));
   dst.replace(_T(":"), _T("_"));
   dst.replace(_T("-"), _T("_"));
   dst.replace(_T("."), _T("_"));
   dst.replace(_T(","), _T("_"));
   dst.replace(_T("#"), _T("_"));
   dst.replace(_T("\\"), _T("/"));
   ssize_t index = dst.find(_T("("));
   if (index != String::npos)
      dst.shrink(dst.length() - index);
   return dst;
}

/**
 * Find and replace all occurrences of given sub-string
 */
static void FindAndReplaceAll(StringBuffer *data, const TCHAR *toSearch, const TCHAR *replaceStr)
{
   // Get the first occurrence
   ssize_t pos = data->find(toSearch);

   // Repeat till end is reached
   while (pos != String::npos)
   {
      // Replace this occurrence of Sub String
      data->replace(toSearch, replaceStr);
      // Get the next occurrence from the current position
      pos = data->find(toSearch);
   }
}

/**
 * Get tags from given object. Returns true if ignore flag is set
 */
static bool GetTagsFromObject(const NetObj& object, StringBuffer *tags)
{
   StringMap *ca = object.getCustomAttributes();
   if (ca == nullptr)
      return false;

   nxlog_debug_tag(DEBUG_TAG, 8, _T("Object: %s - CMA: #%d"), object.getName(), ca->size());

   bool ignoreMetric = false;
   StringList *keys = ca->keys();
   for (int i = 0; i < keys->size(); i++)
   {
      const TCHAR *key = keys->get(i);
      if (!_tcsnicmp(_T("ignore_clickhouse"), key, 16))
      {
         ignoreMetric = true;
         break;
      }

      // Only include CA's with the prefix tag_
      if (!_tcsnicmp(_T("tag_"), key, 4))
      {
         StringBuffer value = NormalizeString(ca->get(keys->get(i)));
         if (!value.isEmpty())
         {
            StringBuffer name = NormalizeString(&key[4]);
            nxlog_debug_tag(DEBUG_TAG, 8, _T("Object: %s - CA: K:%s = V:%s"), object.getName(), name.cstr(), value.cstr());
            tags->append(_T('\t'));
            tags->append(name);
            tags->append(_T('='));
            tags->append(value);
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
   delete keys;
   delete ca;
   return ignoreMetric;
}

/**
 * Parse custom attributes and extract tags
 */
static bool ParseCustomAttributesForTags(const NetObj& object, std::vector<std::pair<std::string, std::string>>& tags, bool checkIgnoreFlag = true)
{
   StringMap *ca = object.getCustomAttributes();
   if (ca == nullptr)
      return false;

   nxlog_debug_tag(DEBUG_TAG, 8, _T("Object: %s - CMA: #%d"), object.getName(), ca->size());

   bool ignoreMetric = false;
   StringList *keys = ca->keys();
   for (int i = 0; i < keys->size(); i++)
   {
      const TCHAR *key = keys->get(i);
      if (checkIgnoreFlag && !_tcsnicmp(_T("ignore_clickhouse"), key, 16))
      {
         ignoreMetric = true;
         break;
      }

      // Only include CA's with the prefix tag_
      if (!_tcsnicmp(_T("tag_"), key, 4))
      {
         StringBuffer value = NormalizeString(ca->get(keys->get(i)));
         if (!value.isEmpty())
         {
            StringBuffer tagName = NormalizeString(&key[4]);
            nxlog_debug_tag(DEBUG_TAG, 8, _T("Object: %s - CA: K:%s = V:%s"), object.getName(), tagName.cstr(), value.cstr());
            
            // Convert tag name and value to std::string (UTF-8)
            char utf8TagName[256], utf8TagValue[256];
            wchar_to_utf8(tagName, -1, utf8TagName, sizeof(utf8TagName));
            wchar_to_utf8(value, -1, utf8TagValue, sizeof(utf8TagValue));
            
            tags.emplace_back(std::string(utf8TagName), std::string(utf8TagValue));
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
   delete keys;
   delete ca;
   return ignoreMetric;
}

/**
 * Build and queue metric from item DCI's
 */
bool ClickHouseStorageDriver::saveDCItemValue(DCItem *dci, time_t timestamp, const TCHAR *value)
{
   nxlog_debug_tag(DEBUG_TAG, 8,
            _T("Raw metric: OwnerName:%s DataSource:%i Type:%i Name:%s Description: %s Instance:%s DataType:%i DeltaCalculationMethod:%i RelatedObject:%i Value:%s timestamp:") INT64_FMT,
            dci->getOwnerName(), dci->getDataSource(), dci->getType(), dci->getName().cstr(), dci->getDescription().cstr(),
            dci->getInstanceName().cstr(), dci->getTransformedDataType(), dci->getDeltaCalculationMethod(), dci->getRelatedObject(),
            value, static_cast<INT64>(timestamp));

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
   const TCHAR *ds;
   switch (dci->getDataSource())
   {
      case DS_DEVICE_DRIVER:
         ds = _T("device");
         break;
      case DS_INTERNAL:
         ds = _T("internal");
         break;
      case DS_MODBUS:
         ds = _T("modbus");
         break;
      case DS_MQTT:
         ds = _T("mqtt");
         break;
      case DS_NATIVE_AGENT:
         ds = _T("agent");
         break;
      case DS_PUSH_AGENT:
         ds = _T("push");
         break;
      case DS_SCRIPT:
         ds = _T("script");
         break;
      case DS_SMCLP:
         ds = _T("smclp");
         break;
      case DS_SNMP_AGENT:
         ds = _T("snmp");
         break;
      case DS_SSH:
         ds = _T("ssh");
         break;
      case DS_WEB_SERVICE:
         ds = _T("websvc");
         break;
      case DS_WINPERF:
         ds = _T("wmi");
         break;
      default:
         ds = _T("unknown");
         break;
   }

   // Convert datasource to std::string
   char utf8_ds[64];
   wchar_to_utf8(ds, -1, utf8_ds, sizeof(utf8_ds));
   record.datasource = utf8_ds;

   // Delta calculation types
   const TCHAR *dct;
   switch (dci->getDeltaCalculationMethod())
   {
      case DCM_ORIGINAL_VALUE:
         dct = _T("original");
         break;
      case DCM_SIMPLE:
         dct = _T("simple");
         break;
      case DCM_AVERAGE_PER_SECOND:
         dct = _T("avg_sec");
         break;
      case DCM_AVERAGE_PER_MINUTE:
         dct = _T("avg_min");
         break;
      default:
         dct = _T("unknown");
         break;
   }

   // Convert delta type to std::string
   char utf8_dct[64];
   wchar_to_utf8(dct, -1, utf8_dct, sizeof(utf8_dct));
   record.deltatype = utf8_dct;

   // DCI (data collection item) data types
   const TCHAR *dt;
   bool isInteger, isUnsigned = false;
   switch (dci->getTransformedDataType())
   {
      case DCI_DT_INT:
         dt = _T("signed-integer32");
         isInteger = true;
         break;
      case DCI_DT_UINT:
         dt = _T("unsigned-integer32");
         isInteger = true;
         isUnsigned = true;
         break;
      case DCI_DT_INT64:
         dt = _T("signed-integer64");
         isInteger = true;
         break;
      case DCI_DT_UINT64:
         dt = _T("unsigned-integer64");
         isInteger = true;
         isUnsigned = true;
         break;
      case DCI_DT_FLOAT:
         dt = _T("float");
         isInteger = false;
         break;
      case DCI_DT_STRING:
         dt = _T("string");
         isInteger = false;
         break;
      case DCI_DT_NULL:
         dt = _T("null");
         isInteger = false;
         break;
      case DCI_DT_COUNTER32:
         dt = _T("counter32");
         isInteger = true;
         isUnsigned = true;
         break;
      case DCI_DT_COUNTER64:
         dt = _T("counter64");
         isInteger = true;
         isUnsigned = true;
         break;
      default:
         dt = _T("unknown");
         isInteger = false;
         break;
   }

   // Convert data type to std::string
   char utf8_dt[64];
   wchar_to_utf8(dt, -1, utf8_dt, sizeof(utf8_dt));
   record.datatype = utf8_dt;

   // Set data class
   record.dataclass = "item";

   // Get custom tags from host
   if (ParseCustomAttributesForTags(static_cast<NetObj&>(*dci->getOwner()), record.tags))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric not sent: ignore flag set on owner object"));
      return true;
   }

   // Get custom tags from related object (if any)
   shared_ptr<NetObj> relatedObject = FindObjectById(dci->getRelatedObject());
   if (relatedObject != nullptr)
   {
      if (ParseCustomAttributesForTags(static_cast<NetObj&>(*relatedObject), record.tags))
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric not sent: ignore flag set on related object %s"), relatedObject->getName());
         return true;
      }
      
      // Set related object type
      char utf8_rot[128];
      wchar_to_utf8(relatedObject->getObjectClassName(), -1, utf8_rot, sizeof(utf8_rot));
      record.relatedobjecttype = utf8_rot;
   }
   else
   {
      record.relatedobjecttype = "none";
   }

   // Set metric name
   StringBuffer name;
   if ((dci->getDataSource() == DS_SNMP_AGENT) ||
       ((dci->getDataSource() == DS_INTERNAL) && !_tcsnicmp(dci->getName(), _T("Dummy"), 5)))
   {
      name = NormalizeString(dci->getDescription());
   }
   else
   {
      name = NormalizeString(dci->getName());
   }

   // Set host 
   StringBuffer host(dci->getOwner()->getName());
   host.replace(_T(" "), _T("_"));
   host.replace(_T(","), _T("_"));
   host.replace(_T(":"), _T("_"));
   FindAndReplaceAll(&host, _T("__"), _T("_"));
   host.toLowercase();

   // Validate value
   TCHAR correctedValue[64];
   correctedValue[0] = 0;
   if (m_validateValues && (dci->getTransformedDataType() != DCI_DT_STRING))
   {
      TCHAR *eptr;

      if (isInteger)
      {
         if (isUnsigned && m_enableUnsignedType)
         {
            errno = 0;
            uint64_t u = _tcstoull(value, &eptr, 0);
            if ((*eptr != 0) || (errno == ERANGE))
            {
               if (!m_correctValues)
               {
                  nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric not sent: value %s:%s:%s=\"%s\" did not pass unsigned integer validation (%s)"),
                     ds, host.cstr(), name.cstr(), value, (*eptr != 0) ? _T("parse error") : _T("value is out of range"));
                  return true;
               }
               IntegerToString(u, correctedValue);
            }
         }
         else
         {
            errno = 0;
            int64_t i = _tcstoll(value, &eptr, 0);
            if ((*eptr != 0) || (errno == ERANGE))
            {
               if (!m_correctValues)
               {
                  nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric not sent: value %s:%s:%s=\"%s\" did not pass signed integer validation (%s)"),
                     ds, host.cstr(), name.cstr(), value, (*eptr != 0) ? _T("parse error") : _T("value is out of range"));
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
               nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric not sent: value %s:%s:%s=\"%s\" did not pass number validation (%s)"),
                  ds, host.cstr(), name.cstr(), value, (*eptr != 0) ? _T("parse error") : _T("value is out of range"));
               return true;
            }
            _sntprintf(correctedValue, 64, _T("%f"), d);
         }
      }

      if (correctedValue[0] != 0)
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Value %s:%s:%s=\"%s\" corrected to \"%s\""), ds, host.cstr(), name.cstr(), value, correctedValue);
   }

   // Set instance
   StringBuffer instance = NormalizeString(dci->getInstanceName());
   if (instance.isEmpty())
   {
      instance = _T("none");
   }
   else
   {
      // Remove instance from the metric name
      name.replace(instance, _T(""));
      FindAndReplaceAll(&name, _T("__"), _T("_"));
   }

   // Convert all string fields to UTF-8
   char utf8_name[256], utf8_host[256], utf8_instance[256], utf8_value[1024];
   
   wchar_to_utf8(name, -1, utf8_name, sizeof(utf8_name));
   wchar_to_utf8(host, -1, utf8_host, sizeof(utf8_host));
   wchar_to_utf8(instance, -1, utf8_instance, sizeof(utf8_instance));
   
   const TCHAR *finalValue = (correctedValue[0] != 0) ? correctedValue : value;
   wchar_to_utf8(finalValue, -1, utf8_value, sizeof(utf8_value));
   
   // Set the fields in the record
   record.name = utf8_name;
   record.host = utf8_host;
   record.instance = utf8_instance;
   record.value = utf8_value;
   
   // Queue the record for processing
   int senderIndex = dci->getId() % m_senders.size();
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Queuing metric record to sender #%d: %s.%s"), 
       senderIndex, host.cstr(), name.cstr());
   
   m_senders.get(senderIndex)->enqueue(record);
   return true;
}

/**
 * Save table DCI's
 */
bool ClickHouseStorageDriver::saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value)
{
   return true;
}

/**
 * Get driver's internal metrics
 */
DataCollectionError ClickHouseStorageDriver::getInternalMetric(const TCHAR *metric, TCHAR *value)
{
   DataCollectionError rc = DCE_SUCCESS;
   if (!_tcsicmp(metric, _T("senders")))
   {
      ret_int(value, m_senders.size());
   }
   else if (!_tcsicmp(metric, _T("overflowQueues")))
   {
      uint32_t count = 0;
      for(int i = 0; i < m_senders.size(); i++)
         if (m_senders.get(i)->isFull())
            count++;
      ret_uint(value, count);
   }
   else if (!_tcsicmp(metric, _T("messageDrops")))
   {
      uint64_t count = 0;
      for(int i = 0; i < m_senders.size(); i++)
         count += m_senders.get(i)->getMessageDrops();
      ret_uint64(value, count);
   }
   else if (!_tcsicmp(metric, _T("queueSize.bytes")))
   {
      uint64_t size = 0;
      for(int i = 0; i < m_senders.size(); i++)
         size += m_senders.get(i)->getQueueSizeInBytes();
      ret_uint64(value, size);
   }
   else if (!_tcsicmp(metric, _T("queueSize.messages")))
   {
      uint32_t size = 0;
      for(int i = 0; i < m_senders.size(); i++)
         size += m_senders.get(i)->getQueueSizeInMessages();
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