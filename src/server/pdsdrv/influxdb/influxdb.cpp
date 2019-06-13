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

#include <iostream>
#include <mutex>
#include <regex>
#include <string>

// debug pdsdrv.influxdb 1-5
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
   std::string queuedMessages;
   unsigned int nbQueuedMessages;
   std::mutex mtx;
   void queuePush(std::string data, unsigned int maxSize);
   void logMessage(std::string message, int level);
   std::string normalizeString(std::string str);
   std::string getString(const wchar_t *data);
   void findAndReplaceAll(std::string &data, std::string toSearch, std::string replaceStr);
   void toLowerCase(std::string &data);

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
   nbQueuedMessages = 0;
   queuedMessages = "";
}

/**
 * Destructor
 */
InfluxDBStorageDriver::~InfluxDBStorageDriver()
{
}

inline void InfluxDBStorageDriver::logMessage(std::string message, int level = 1) {
  std::wstring msg(message.begin(), message.end());
  nxlog_debug_tag(DEBUG_TAG, level, msg.c_str());
}

/**
 * Normalize all the metric and tag names
 */
std::string InfluxDBStorageDriver::normalizeString(std::string str) {
  toLowerCase(str);
  str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
  std::replace(str.begin(), str.end(), ':', '_');
  std::replace(str.begin(), str.end(), '.', '_');
  std::replace(str.begin(), str.end(), '-', '_');
  std::replace(str.begin(), str.end(), ',', '_');
  std::replace(str.begin(), str.end(), '#', '_');
  std::regex remove{"\\(.*"};
  str = std::regex_replace(str, remove, "");

  return str;
}

inline void InfluxDBStorageDriver::toLowerCase(std::string &data) {
  std::transform(data.begin(), data.end(), data.begin(), ::tolower);
}

std::string InfluxDBStorageDriver::getString(const wchar_t *data) {
  std::wstring tempWStr(data);
  std::string tmpStr(tempWStr.begin(), tempWStr.end());
  return tmpStr;
}

void InfluxDBStorageDriver::findAndReplaceAll(std::string &data, std::string toSearch, std::string replaceStr) {
  // Get the first occurrence
  size_t pos = data.find(toSearch);

  // Repeat till end is reached
  while (pos != std::string::npos) {
    // Replace this occurrence of Sub String
    data.replace(pos, toSearch.size(), replaceStr);
    // Get the next occurrence from the current position
    pos = data.find(toSearch, pos + replaceStr.size());
  }
}

/**
 * Metric Queuing and Packet sending
 */
void InfluxDBStorageDriver::queuePush(std::string data, unsigned int maxSize = 100) {

  bool flushNow = data.empty();
  if (!flushNow) {
    queuedMessages += data + "\n";
    nbQueuedMessages++;
    logMessage("Queue size: " + std::to_string(nbQueuedMessages) + " / " + std::to_string(maxSize), 3);
  }

  if ((nbQueuedMessages >= maxSize) || flushNow) {
    logMessage("Queue size: " + std::to_string(nbQueuedMessages) + " / " + std::to_string(maxSize) + " (Sending)", 2);

    if (queuedMessages.size() > 0) {
      if (send(m_socket, (char *)queuedMessages.c_str(), queuedMessages.size(), 0) <= 0) {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("socket error: %s"), _tcserror(errno));
        // Ignore; will be re-sent with the next message
      } else {
        // Data sent - empty queue
        queuedMessages = "";
        nbQueuedMessages = 0;
      }
    }
  }
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
   logMessage("Initializing", 1);

   m_hostname = config->getValue(_T("/InfluxDB/Hostname"), _T("localhost"));
   m_port = static_cast<UINT16>(config->getValueAsUInt(_T("/InfluxDB/Port"), 8189));

   InetAddress addr = InetAddress::resolveHostName(m_hostname);
   if (!addr.isValidUnicast())
   {
      nxlog_write_generic(NXLOG_ERROR, _T("InfluxDB: invalid hostname %s"), m_hostname);
      return false;
   }

   m_socket = ConnectToHostUDP(addr, m_port);
   if (m_socket == INVALID_SOCKET)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("InfluxDB: cannot create UDP socket"));
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Using destination address %s:%u"), (const TCHAR *)addr.toString(), m_port);
   return true;
}

void InfluxDBStorageDriver::shutdown()
{
   logMessage("Shutdown", 1);
   this->queuePush("");
   closesocket(m_socket);
}

/**
 * Build and queue metric from item DCI's
 */
bool InfluxDBStorageDriver::saveDCItemValue(DCItem *dci, time_t timestamp, const TCHAR *value) {
  if (dci == NULL) {
    logMessage("Metric Not Sent: Empty DCI", 1);
    return true;
  }

  nxlog_debug_tag(DEBUG_TAG, 5, _T("Raw metric: OwnerName:%s SystemTag:%s DataSource:%i Type:%i Name:%s Description: %s Instance:%s DataType:%i Value:%s timestamp:%zu"), dci->getOwner()->getName(), dci->getSystemTag(), dci->getDataSource(), dci->getType(), dci->getName(), dci->getDescription(), dci->getInstance(), dci->getDataType(), value, timestamp);

  std::string m_ds = ""; // Data sources
  switch (dci->getDataSource()) {
  case DS_INTERNAL:
    m_ds = "internal";
    break;
  case DS_NATIVE_AGENT:
    m_ds = "agent";
    break;
  case DS_SNMP_AGENT:
    m_ds = "snmp";
    break;
  case DS_CHECKPOINT_AGENT:
    m_ds = "check_point_snmp";
    break;
  case DS_PUSH_AGENT:
    m_ds = "push";
    break;
  case DS_WINPERF:
    m_ds = "wmi";
    break;
  case DS_SMCLP:
    m_ds = "smclp";
    break;
  case DS_SCRIPT:
    m_ds = "script";
    break;
  case DS_SSH:
    m_ds = "ssh";
    break;
  case DS_MQTT:
    m_ds = "mqtt";
    break;
  case DS_DEVICE_DRIVER:
    m_ds = "device";
    break;
  default:
    m_ds = "unknown";
    break;
  }

  std::string m_dc = ""; // Data collection object types
  switch (dci->getType()) {
  case DCO_TYPE_GENERIC:
    m_dc = "generic";
    break;
  case DCO_TYPE_ITEM:
    m_dc = "item";
    break;
  case DCO_TYPE_TABLE:
    m_dc = "table";
    break;
  case DCO_TYPE_LIST:
    m_dc = "list";
    break;
  default:
    m_dc = "unknown";
    break;
  }

  std::string m_dt = ""; // DCI (data collection item) data types
  switch (dci->getDataType()) {
  case DCI_DT_INT:
    m_dt = "signed-integer32";
    break;
  case DCI_DT_UINT:
    m_dt = "unsigned-integer32";
    break;
  case DCI_DT_INT64:
    m_dt = "signed-integer64";
    break;
  case DCI_DT_UINT64:
    m_dt = "unsigned-integer64";
    break;
  case DCI_DT_FLOAT:
    m_dt = "float";
    break;
  case DCI_DT_STRING:
    m_dt = "string";
    break;
  case DCI_DT_NULL:
    m_dt = "null";
    break;
  case DCI_DT_COUNTER32:
    m_dt = "counter32";
    break;
  case DCI_DT_COUNTER64:
    m_dt = "counter64";
    break;
  default:
    m_dt = "unknown";
    break;
  }

  // Metric name
  std::string m_name = "";

  // If it's a MIB or Dummy metric use the Description if not use the Name
  if (std::regex_match(getString(dci->getName()), std::regex{"^\\.1.*"}) || std::regex_match(getString(dci->getName()), std::regex{"^Dummy.*"})) {
    m_name = normalizeString(getString(dci->getDescription()));
  } else {
    m_name = normalizeString(getString(dci->getName()));
  }

  // Instance
  std::string m_inst = normalizeString(getString(dci->getInstance()));
  if (m_inst.empty()) {
    m_inst = "none";
  } else {
    // Remove instance from the metric name
    std::string::size_type i = m_name.find(m_inst);
    if (i != std::string::npos) {
      m_name.erase(i, m_inst.length());
      findAndReplaceAll(m_name, "__", "_");
    }
  }

  // Host
  std::string m_host = getString(dci->getOwner()->getName());
  std::replace(m_host.begin(), m_host.end(), ' ', '_');
  std::replace(m_host.begin(), m_host.end(), ':', '_');
  findAndReplaceAll(m_host, "__", "_");
  toLowerCase(m_host);

  if (m_host.empty()) {
    logMessage("Metric Not Sent: Empty host", 1);
    return true;
  }

  // Get Host CA's
  std::string m_tags = "";
  StringMap *ca = dci->getOwner()->getCustomAttributes();
  if (ca != NULL) {
    StringList *ca_key = ca->keys();
    logMessage("Host: " + m_host + " - CMA: #" + std::to_string(ca->size()), 5);

    for (int i = 0; i < ca_key->size(); i++) {
      std::string ca_key_name = getString(ca_key->get(i));
      std::string ca_value = getString(ca->get(ca_key->get(i)));
      ca_value = normalizeString(ca_value);

      // Only include CA's with the prefix tag_
      if (std::regex_match(ca_key_name, std::regex{"^tag_.*", std::regex_constants::icase}) && !ca_value.empty()) {
        std::regex remove{"^tag_", std::regex_constants::icase};
        ca_key_name = std::regex_replace(ca_key_name, remove, "");
        ca_key_name = normalizeString(ca_key_name);
        logMessage("Host: " + m_host + " - CA: K:" + ca_key_name + " = V:" + ca_value, 5);

        if (m_tags.empty()) {
          m_tags = ca_key_name + '=' + '"' + ca_value + '"';
        } else {
          m_tags = m_tags + ',' + ca_key_name + '=' + '"' + ca_value + '"';
        }

      } else {
        logMessage("Host: " + m_host + " - CA: K:" + ca_key_name + " (Ignored)", 5);
      }
    }
    delete ca_key;
  }
  delete ca;


  // TS
  std::stringstream cls;
  cls << (long)timestamp;
  std::string m_ts = cls.str();

  // Value
  std::string m_val = getString(value);

  // Dont't try to send empty values
  if (m_val.empty()) {
    logMessage("Metric Not Sent: Empty value", 1);
    return true;
  }

  // String formating
  if (m_dt == "string") {
    m_val = '"' + m_val + '"';
  }

  // Integer formating
  if (m_dt.find("integer") != std::string::npos || m_dt.find("counter") != std::string::npos) {
    m_val = m_val + "i";
  }

  // Build final metric structure
  std::string m_data = "";
  if (m_tags.empty()) {
    m_data = m_name + ",host=" + m_host + ",instance=" + m_inst + ",datasource=" + m_ds + ",dataclass=" + m_dc + ",datatype=" + m_dt + " value=" + m_val + " " + m_ts + "000000000";
  } else {
    m_data = m_name + ",host=" + m_host + ",instance=" + m_inst + ",datasource=" + m_ds + ",dataclass=" + m_dc + ",datatype=" + m_dt + "," + m_tags + " value=" + m_val + " " + m_ts + "000000000";
  }

  logMessage("Processing metric: " + m_data, 3);
  this->mtx.lock();
  this->queuePush(m_data);
  this->mtx.unlock();

  return true;
}

/**
 * Save table DCI's
 */
bool InfluxDBStorageDriver::saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value) {
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
