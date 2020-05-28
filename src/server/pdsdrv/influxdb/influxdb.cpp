/*
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for InfluxDB
 ** Copyright (C) 2019 Sebastian YEPES FERNANDEZ & Julien DERIVIERE
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
 ** File: influxdb.cpp
  */

#include <nms_core.h>
#include <pdsdrv.h>

#include <string>

// debug pdsdrv.influxdb 1-8
#define DEBUG_TAG _T("pdsdrv.influxdb")

/**
 * Driver class definition
 */
class InfluxDBStorageDriver : public PerfDataStorageDriver
{
private:
   const TCHAR *m_hostname;
   UINT16 m_port;
   SOCKET m_socket;
   std::string m_queuedMessages;
   UINT32 m_queuedMessageCount;
   UINT32 m_maxQueueSize;
   Mutex m_mutex;

   void queuePush(const std::string& data);

   static std::string normalizeString(std::string str);
   static std::string getString(const TCHAR *tstr);
   static void findAndReplaceAll(std::string& data, const std::string& toSearch, const std::string& replaceStr);
   static void toLowerCase(std::string& data);

public:
   InfluxDBStorageDriver();
   virtual ~InfluxDBStorageDriver();

   virtual const TCHAR *getName() override;
   virtual bool init(Config *config) override;
   virtual void shutdown() override;
   virtual bool saveDCItemValue(DCItem *dcObject, time_t timestamp, const TCHAR *value) override;
   virtual bool saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value) override;
};

/**
 * Driver name
 */
static const TCHAR *s_driverName = _T("InfluxDB");

/**
 * Constructor
 */
InfluxDBStorageDriver::InfluxDBStorageDriver()
{
   m_hostname = _T("localhost");
   m_port = 8189;
   m_socket = INVALID_SOCKET;
   m_queuedMessageCount = 0;
   m_queuedMessages = "";
   m_maxQueueSize = 100;
}

/**
 * Destructor
 */
InfluxDBStorageDriver::~InfluxDBStorageDriver()
{
}

/**
 * Normalize all the metric and tag names
 */
std::string InfluxDBStorageDriver::normalizeString(std::string str)
{
   toLowerCase(str);
   str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
   std::replace(str.begin(), str.end(), ':', '_');
   std::replace(str.begin(), str.end(), '.', '_');
   std::replace(str.begin(), str.end(), '-', '_');
   std::replace(str.begin(), str.end(), ',', '_');
   std::replace(str.begin(), str.end(), '#', '_');
   std::replace(str.begin(), str.end(), '\\', '/');
   size_t index = str.find('(');
   if (index != std::string::npos)
      str.resize(index);
   return str;
}

/**
 * Convert string to lower case
 */
inline void InfluxDBStorageDriver::toLowerCase(std::string& data)
{
   std::transform(data.begin(), data.end(), data.begin(), ::tolower);
}

/**
 * Get std::string from TCHAR (will return UTF-8 string in Unicode builds)
 */
std::string InfluxDBStorageDriver::getString(const TCHAR *tstr)
{
#ifdef UNICODE
#ifdef UNICODE_UCS4
   size_t len = ucs4_utf8len(tstr, -1);
#else
   size_t len = ucs2_utf8len(tstr, -1);
#endif
#if HAVE_ALLOCA
   char *buffer = static_cast<char*>(alloca(len + 1));
#else
   char *buffer = static_cast<char*>(MemAlloc(len + 1));
#endif
   WideCharToMultiByte(CP_UTF8, 0, tstr, -1, buffer, (int)len + 1, nullptr, nullptr);
#if HAVE_ALLOCA
   return std::string(buffer);
#else
   std::string result(buffer);
   MemFree(buffer);
   return result;
#endif
#else
   return std::string(tstr);
#endif
}

/**
 * Find and replace all occurrences of given sub-string
 */
void InfluxDBStorageDriver::findAndReplaceAll(std::string& data, const std::string& toSearch, const std::string& replaceStr)
{
   // Get the first occurrence
   size_t pos = data.find(toSearch);

   // Repeat till end is reached
   while (pos != std::string::npos)
   {
      // Replace this occurrence of Sub String
      data.replace(pos, toSearch.size(), replaceStr);
      // Get the next occurrence from the current position
      pos = data.find(toSearch, pos + replaceStr.size());
   }
}

/**
 * Metric Queuing and Packet sending
 */
void InfluxDBStorageDriver::queuePush(const std::string& data)
{
   m_mutex.lock();

   bool flushNow = data.empty();
   if (!flushNow)
   {
      // Check that packet is not longer than 64K
      if (data.length() + m_queuedMessages.length() < 65534)
      {
         m_queuedMessages += data + "\n";
         m_queuedMessageCount++;
      }
      else
      {
         flushNow = true;
      }
   }

   if ((m_queuedMessageCount >= m_maxQueueSize) || flushNow)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Queue size: %u / %u (sending)"), m_queuedMessageCount, m_maxQueueSize);

      if (m_queuedMessages.size() > 0)
      {
         if (SendEx(m_socket, m_queuedMessages.c_str(), m_queuedMessages.size(), 0, INVALID_MUTEX_HANDLE) <= 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 8, _T("socket error: %s"), _tcserror(errno));
            // Ignore; will be re-sent with the next message
         }
         else
         {
            // Data sent - empty queue
            m_queuedMessages.clear();
            m_queuedMessageCount = 0;
         }
      }
   }
   else
   {
      if (flushNow && !data.empty())
      {
         m_queuedMessages += data + "\n";
         m_queuedMessageCount++;
      }
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Queue size: %u / %u"), m_queuedMessageCount, m_maxQueueSize);
   }

   m_mutex.unlock();
}

/**
 * Get name
 */
const TCHAR *InfluxDBStorageDriver::getName()
{
   return s_driverName;
}

/**
 * Initialize driver
 */
bool InfluxDBStorageDriver::init(Config *config)
{
   m_hostname = config->getValue(_T("/InfluxDB/Hostname"), m_hostname);
   m_port = static_cast<UINT16>(config->getValueAsUInt(_T("/InfluxDB/Port"), m_port));
   m_maxQueueSize = config->getValueAsUInt(_T("/InfluxDB/MaxQueueSize"), m_maxQueueSize);

   InetAddress addr = InetAddress::resolveHostName(m_hostname);
   if (!addr.isValidUnicast())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("InfluxDB: invalid hostname %s"), m_hostname);
      return false;
   }

   m_socket = ConnectToHostUDP(addr, m_port);
   if (m_socket == INVALID_SOCKET)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("InfluxDB: cannot create UDP socket"));
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Using destination address %s:%u"), (const TCHAR *)addr.toString(), m_port);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Max queue size set to %u"), m_maxQueueSize);
   return true;
}

/**
 * Shutdown driver
 */
void InfluxDBStorageDriver::shutdown()
{
   queuePush("");
   closesocket(m_socket);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Shutdown completed"));
}

/**
 * Build and queue metric from item DCI's
 */
bool InfluxDBStorageDriver::saveDCItemValue(DCItem *dci, time_t timestamp, const TCHAR *value)
{
   nxlog_debug_tag(DEBUG_TAG, 8,
            _T("Raw metric: OwnerName:%s DataSource:%i Type:%i Name:%s Description: %s Instance:%s DataType:%i DeltaCalculationMethod:%i RelatedObject:%i Value:%s timestamp:") INT64_FMT,
            dci->getOwnerName(), dci->getDataSource(), dci->getType(), dci->getName().cstr(), dci->getDescription().cstr(),
            dci->getInstance().cstr(), dci->getDataType(), dci->getDeltaCalculationMethod(), dci->getRelatedObject(),
            value, static_cast<INT64>(timestamp));

   // Dont't try to send empty values
   if (*value == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric %s [%u] not sent: empty value"), dci->getName().cstr(), dci->getId());
      return true;
   }

   const char *ds = ""; // Data sources
   switch (dci->getDataSource())
   {
      case DS_DEVICE_DRIVER:
         ds = "device";
         break;
      case DS_INTERNAL:
         ds = "internal";
         break;
      case DS_MQTT:
         ds = "mqtt";
         break;
      case DS_NATIVE_AGENT:
         ds = "agent";
         break;
      case DS_PUSH_AGENT:
         ds = "push";
         break;
      case DS_SCRIPT:
         ds = "script";
         break;
      case DS_SMCLP:
         ds = "smclp";
         break;
      case DS_SNMP_AGENT:
         ds = "snmp";
         break;
      case DS_SSH:
         ds = "ssh";
         break;
      case DS_WEB_SERVICE:
         ds = "websvc";
         break;
      case DS_WINPERF:
         ds = "wmi";
         break;
      default:
         ds = "unknown";
         break;
   }

   const char *dc = ""; // Data collection object types
   switch (dci->getType())
   {
      case DCO_TYPE_GENERIC:
         dc = "generic";
         break;
      case DCO_TYPE_ITEM:
         dc = "item";
         break;
      case DCO_TYPE_TABLE:
         dc = "table";
         break;
      case DCO_TYPE_LIST:
         dc = "list";
         break;
      default:
         dc = "unknown";
         break;
   }

   const char *dct = ""; // Data Calculation types
   switch (dci->getDeltaCalculationMethod())
   {
      case DCM_ORIGINAL_VALUE:
         dct = "original";
         break;
      case DCM_SIMPLE:
         dct = "simple";
         break;
      case DCM_AVERAGE_PER_SECOND:
         dct = "avg_sec";
         break;
      case DCM_AVERAGE_PER_MINUTE:
         dct = "avg_min";
         break;
      default:
         dct = "unknown";
         break;
   }

   const char *dt = ""; // DCI (data collection item) data types
   bool isInteger;
   switch (dci->getDataType())
   {
      case DCI_DT_INT:
         dt = "signed-integer32";
         isInteger = true;
         break;
      case DCI_DT_UINT:
         dt = "unsigned-integer32";
         isInteger = true;
         break;
      case DCI_DT_INT64:
         dt = "signed-integer64";
         isInteger = true;
         break;
      case DCI_DT_UINT64:
         dt = "unsigned-integer64";
         isInteger = true;
         break;
      case DCI_DT_FLOAT:
         dt = "float";
         isInteger = false;
         break;
      case DCI_DT_STRING:
         dt = "string";
         isInteger = false;
         break;
      case DCI_DT_NULL:
         dt = "null";
         isInteger = false;
         break;
      case DCI_DT_COUNTER32:
         dt = "counter32";
         isInteger = true;
         break;
      case DCI_DT_COUNTER64:
         dt = "counter64";
         isInteger = true;
         break;
      default:
         dt = "unknown";
         isInteger = false;
         break;
   }

   // Metric name
   std::string name = "";

   // If it's a MIB or Dummy metric use the Description if not use the Name
   if ((dci->getDataSource() == DS_SNMP_AGENT) ||
       ((dci->getDataSource() == DS_INTERNAL) && !_tcsnicmp(dci->getName(), _T("Dummy"), 5)))
   {
      name = normalizeString(getString(dci->getDescription()));
   }
   else
   {
      name = normalizeString(getString(dci->getName()));
   }

   // Instance
   std::string instance = normalizeString(getString(dci->getInstance()));
   if (instance.empty())
   {
      instance = "none";
   }
   else
   {
      // Remove instance from the metric name
      std::string::size_type i = name.find(instance);
      if (i != std::string::npos)
      {
         name.erase(i, instance.length());
         findAndReplaceAll(name, "__", "_");
      }
   }

   // Host
   std::string host = getString(dci->getOwner()->getName());
   std::replace(host.begin(), host.end(), ' ', '_');
   std::replace(host.begin(), host.end(), ',', '_');
   std::replace(host.begin(), host.end(), ':', '_');
   findAndReplaceAll(host, "__", "_");
   toLowerCase(host);

   if (host.empty())
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric not sent: empty host"));
      return true;
   }

   // Get Host CA's
   bool ignoreMetric = false;
   std::string m_tags = "";
   StringMap *ca = dci->getOwner()->getCustomAttributes();
   if (ca != nullptr)
   {
      StringList *ca_key = ca->keys();
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Host: %hs - CMA: #%d"), host.c_str(), ca->size());

      for (int i = 0; i < ca_key->size(); i++)
      {
         std::string ca_key_name = getString(ca_key->get(i));
         std::string ca_value = getString(ca->get(ca_key->get(i)));
         ca_value = normalizeString(ca_value);

         if (!ca_value.empty() && !strnicmp("ignore_influxdb", ca_key_name.c_str(), 15))
         {
            ignoreMetric = true;
         }

         // Only include CA's with the prefix tag_
         if (!ca_value.empty() && !strnicmp("tag_", ca_key_name.c_str(), 4))
         {
            ca_key_name = normalizeString(ca_key_name.substr(4));
            nxlog_debug_tag(DEBUG_TAG, 7, _T("Host: %hs - CA: K:%hs = V:%hs"),
                  host.c_str(), ca_key_name.c_str(), ca_value.c_str());

            if (m_tags.empty())
            {
               m_tags = ca_key_name + '=' + ca_value;
            }
            else
            {
               m_tags = m_tags + ',' + ca_key_name + '=' + ca_value;
            }

         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("Host: %hs - CA: K:%hs (Ignored)"), host.c_str(), ca_key_name.c_str());
         }
      }
      delete ca_key;
   }
   delete ca;

   // Get RelatedObject (Interface) CA's
   const char *relatedObject_type = "none";

   shared_ptr<NetObj> relatedObject_iface = FindObjectById(dci->getRelatedObject(), OBJECT_INTERFACE);
   if (relatedObject_iface != nullptr)
   {
      relatedObject_type = relatedObject_iface->getObjectClassNameA();
      StringMap *ca = relatedObject_iface->getCustomAttributes();
      if (ca != nullptr)
      {
         StringList *ca_key = ca->keys();
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Host: %hs - RelatedObject: %s [%u] - RelatedObjectType: %s - CMA: #%d"),
                  host.c_str(), relatedObject_iface->getName(), relatedObject_iface->getId(), relatedObject_iface->getObjectClassName(), ca->size());

         for (int i = 0; i < ca_key->size(); i++)
         {
            std::string ca_key_name = getString(ca_key->get(i));
            std::string ca_value = getString(ca->get(ca_key->get(i)));
            ca_value = normalizeString(ca_value);

            if (!ca_value.empty() && !strnicmp("ignore_influxdb", ca_key_name.c_str(), 15))
            {
               ignoreMetric = true;
            }

            // Only include CA's with the prefix tag_
            if (!ca_value.empty() && !strnicmp("tag_", ca_key_name.c_str(), 4))
            {
               ca_key_name = normalizeString(ca_key_name.substr(4));
               nxlog_debug_tag(DEBUG_TAG, 7, _T("Host: %hs - RelatedObject %s [%u] - CA: K:%hs = V:%hs"),
                     host.c_str(), relatedObject_iface->getName(), relatedObject_iface->getId(), ca_key_name.c_str(), ca_value.c_str());

               if (m_tags.empty())
               {
                  m_tags = ca_key_name + '=' + ca_value;
               }
               else
               {
                  m_tags = m_tags + ',' + ca_key_name + '=' + ca_value;
               }

            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 7, _T("Host: %hs - RelatedObject %s [%u] - CA: K:%hs (Ignored)"), host.c_str(), relatedObject_iface->getName(), relatedObject_iface->getId(), ca_key_name.c_str());
            }
         }
         delete ca_key;
      }
      delete ca;
   }

   if (ignoreMetric)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Metric not sent: has ignore flag"));
      return true;
   }

   // Formatted timestamp
   char ts[32];
   snprintf(ts, 32, INT64_FMTA "000000000", static_cast<INT64>(timestamp));

   // Value
   std::string fvalue = getString(value);

   // String formating
   if (dci->getDataType() == DCI_DT_STRING)
      fvalue = '"' + fvalue + '"';

   // Integer formating
   if (isInteger)
      fvalue = fvalue + "i";

   // Build final metric structure
   std::string data = "";
   if (m_tags.empty())
   {
      data = name + ",host=" + host + ",instance=" + instance + ",datasource=" + ds + ",dataclass=" + dc + ",datatype="
               + dt + ",deltatype=" + dct + ",relatedobjecttype=" + relatedObject_type + " value=" + fvalue + " " + ts;
   }
   else
   {
      data = name + ",host=" + host + ",instance=" + instance + ",datasource=" + ds + ",dataclass=" + dc + ",datatype="
               + dt + ",deltatype=" + dct + ",relatedobjecttype=" + relatedObject_type + "," + m_tags + " value=" + fvalue + " " + ts;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Processing metric: %hs"), data.c_str());
   queuePush(data);

   return true;
}

/**
 * Save table DCI's
 */
bool InfluxDBStorageDriver::saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value)
{
   return true;
}

/**
 * Driver entry point
 */
DECLARE_PDSDRV_ENTRY_POINT(s_driverName, InfluxDBStorageDriver);

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
