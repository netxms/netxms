/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2023 Raden Solutions
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
** File: hddinfo.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("smartctl")

/**
 * Cache entry
 */
struct CacheEntry
{
   json_t *data;
   time_t timestamp;

   CacheEntry(json_t *_data)
   {
      data = _data;
      timestamp = time(nullptr);
   }

   ~CacheEntry()
   {
      json_decref(data);
   }
};

/**
 * Cache
 */
static StringObjectMap<CacheEntry> s_cache(Ownership::True);
static Mutex s_cacheLock(MutexType::FAST);

/**
 * Log messages from smartctl output
 */
static void LogSmartCtlMessages(json_t *root, const TCHAR *prefix)
{
   json_t *messages = json_object_get_by_path_a(root, "smartctl/messages");
   if (json_is_array(messages))
   {
      size_t i;
      json_t *m;
      json_array_foreach(messages, i, m)
      {
         TCHAR *severity = json_object_get_string_t(m, "severity", _T("unknown"));
         TCHAR *text = json_object_get_string_t(m, "string", _T("no text"));
         nxlog_debug_tag(DEBUG_TAG, 4, _T("%s: smartctl %s message: %s"), prefix, severity, text);
         MemFree(severity);
         MemFree(text);
      }
   }
}

/**
 * Validates device parameter to prevent command injection
 *
 * @param device Device name to validate
 * @returns true if device name is valid, false otherwise
 */
static bool ValidateDeviceName(const TCHAR *device)
{
   if (device == nullptr || *device == 0)
      return false;

   // Check length
   size_t len = _tcslen(device);
   if (len > 128)  // Reasonable maximum device name length
      return false;

   // Unix device validation; smartctl on Windows uses UNIX naming convention for devices
   // Allow: letters, digits, slash, underscore, hyphen, dot
   for (size_t i = 0; i < len; i++)
   {
      TCHAR c = device[i];
      if (!(_istalnum(c) || c == _T('/') || c == _T('_') || c == _T('-') || c == _T('.')))
         return false;
   }

   // Must start with slash (absolute path) or letter
   if (!(device[0] == _T('/') || _istalpha(device[0])))
      return false;

   // Prevent command injection patterns
   if (_tcsstr(device, _T("..")) != nullptr ||  // Directory traversal
       _tcsstr(device, _T(";")) != nullptr ||   // Command separator
       _tcsstr(device, _T("&")) != nullptr ||   // Command separator
       _tcsstr(device, _T("|")) != nullptr ||   // Pipe
       _tcsstr(device, _T("`")) != nullptr ||   // Command substitution
       _tcsstr(device, _T("$(")) != nullptr ||  // Command substitution
       _tcsstr(device, _T("${")) != nullptr)    // Variable expansion
      return false;

   return true;
}

/**
 * Executes smartctl command for given device
 *
 * @returns smartctl output as json_t object or nullptr on failure
 */
static json_t *RunSmartCtl(const TCHAR *device, bool detailed)
{
   // Validate device name to prevent command injection
   if (!ValidateDeviceName(device))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("RunSmartCtl: Invalid device name \"%s\""), device);
      return nullptr;
   }

   TCHAR cmd[MAX_PATH];
#ifdef _WIN32
   TCHAR binDir[MAX_PATH];
   GetNetXMSDirectory(nxDirBin, binDir);
   _sntprintf(cmd, MAX_PATH, _T("%s\\smartctl.exe %s -j %s 2>NUL"), binDir, detailed ? _T("--all") : _T("-i"), device);
#else
   _sntprintf(cmd, MAX_PATH, _T("smartctl %s -j %s 2>/dev/null"), detailed ? _T("--all") : _T("-i"), device);
#endif

   TCHAR debugPrefix[1024];
   _sntprintf(debugPrefix, 1024, _T("RunSmartCtl(\"%s\")"), device);

   OutputCapturingProcessExecutor pe(cmd);
   if (!pe.execute())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("%s: Failed to execute smartctl"), debugPrefix);
      return nullptr;
   }
   if (!pe.waitForCompletion(10000))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("%s: smartctl timed out"), debugPrefix);
      return nullptr;
   }

   json_error_t error;
   json_t* root = json_loads(pe.getOutput(), 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("%s: Error parsing smartctl JSON output (line %d: %hs)"), debugPrefix, error.line, error.text);
      return nullptr;
   }

   LogSmartCtlMessages(root, debugPrefix);

   if (json_object_get(root, "device") == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("%s: Device section missing in smartctl output"), debugPrefix);
      json_decref(root);
      root = nullptr;
   }

   return root;
}

/**
 * Get value from json_t
 */
static void GetValueFromJson(json_t *json, TCHAR *value)
{
   switch(json_typeof(json))
   {
      case JSON_STRING:
         ret_utf8string(value, json_string_value(json));
         break;
      case JSON_INTEGER:
         IntegerToString(static_cast<INT64>(json_integer_value(json)), value);
         break;
      case JSON_REAL:
         _sntprintf(value, MAX_RESULT_LENGTH, _T("%f"), json_real_value(json));
         break;
      case JSON_TRUE:
         _tcscpy(value, _T("true"));
         break;
      case JSON_FALSE:
         _tcscpy(value, _T("false"));
         break;
      default:
         *value = 0;
         break;
   }
}

/**
 * Handler for PhysicalDisk.* metrics
 */
LONG H_PhysicalDiskInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char customPathBuffer[2048];
   const char *jsonPath;
   switch(arg[0])
   {
      case 'A':
         AgentGetParameterArgA(param, 2, customPathBuffer, 2048);
         jsonPath = customPathBuffer;
         break;
      case 'C':
         jsonPath = "user_capacity/bytes";
         break;
      case 'D':
         jsonPath = "device/type";
         break;
      case 'F':
         jsonPath = "firmware_version";
         break;
      case 'M':
         jsonPath = "model_name";
         break;
      case 'N':
         jsonPath = "serial_number";
         break;
      case 'O':
         jsonPath = "power_on_time/hours";
         break;
      case 'P':
         jsonPath = "power_cycle_count";
         break;
      case 'T':
         jsonPath = "temperature/current";
         break;
      case 'S':
         jsonPath = "smart_status/passed";
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   TCHAR device[128];
   AgentGetParameterArg(param, 1, device, 128);

   json_t *root;
   s_cacheLock.lock();
   CacheEntry *cacheEntry = s_cache.get(device);
   if ((cacheEntry == nullptr) || (time(nullptr) - cacheEntry->timestamp > 60))
   {
      root = RunSmartCtl(device, true);
      if (cacheEntry != nullptr)
      {
         json_decref(cacheEntry->data);
         cacheEntry->data = root;
         cacheEntry->timestamp = time(nullptr);
      }
      else
      {
         cacheEntry = new CacheEntry(root);
         s_cache.set(device, cacheEntry);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("H_PhysicalDiskInfo: added cache entry for device \"%s\""), device);
      }
   }
   else
   {
      root = cacheEntry->data;
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_PhysicalDiskInfo: using cached data for device \"%s\""), device);
   }
   if (root == nullptr)
   {
      s_cacheLock.unlock();
      return SYSINFO_RC_ERROR;
   }

   LONG rc;
   json_t *element = json_object_get_by_path_a(root, jsonPath);
   if ((element == nullptr) && (arg[0] == 'A'))
   {
      // check for attribute name in ata_smart_attributes
      json_t *attributes = json_object_get_by_path_a(root, "ata_smart_attributes/table");
      if (json_is_array(attributes))
      {
         size_t i;
         json_t *attr;
         json_array_foreach(attributes, i, attr)
         {
            const char *name = json_object_get_string_utf8(attr, "name", nullptr);
            if ((name != nullptr) && (strcmp(name, jsonPath) == 0))
            {
               element = json_object_get_by_path_a(attr, "value");
               break;
            }
         }
      }

      // check for attribute name in nvme_smart_health_information_log
      if (element == nullptr)
      {
         json_t *nvmeLog = json_object_get_by_path_a(root, "nvme_smart_health_information_log");
         if (json_is_object(nvmeLog))
         {
            element = json_object_get(nvmeLog, jsonPath);
         }
      }
   }
   if ((element != nullptr) && !json_is_null(element))
   {
      GetValueFromJson(element, value);
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      rc = SYSINFO_RC_UNSUPPORTED;
   }

   s_cacheLock.unlock();

   return rc;
}

/**
 * Run smartctl scan
 */
static bool RunSmartCtlScan(StringList *output)
{
#ifdef _WIN32
   TCHAR binDir[MAX_PATH];
   GetNetXMSDirectory(nxDirBin, binDir);
   StringBuffer cmd(binDir);
   cmd.append(_T("\\smartctl.exe --scan -j 2>NUL"));
#else
   static const TCHAR *cmd = _T("smartctl --scan -j 2>/dev/null");
#endif

   OutputCapturingProcessExecutor pe(cmd);
   if (!pe.execute())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("RunSmartCtlScan: Failed to execute smartctl"));
      return false;
   }
   if (!pe.waitForCompletion(10000))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("RunSmartCtlScan: smartctl timed out"));
      return false;
   }

   json_error_t error;
   json_t* root = json_loads(pe.getOutput(), 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("RunSmartCtlScan: Error parsing smartctl JSON output (line %d: %hs)"), error.line, error.text);
      return false;
   }

   LogSmartCtlMessages(root, _T("RunSmartCtlScan"));

   bool success;
   json_t *devices = json_object_get(root, "devices");
   if (json_is_array(devices))
   {
      success = true;

      size_t i;
      json_t *d;
      json_array_foreach(devices, i, d)
      {
         TCHAR *name = json_object_get_string_t(d, "name", nullptr);
         if (name != nullptr)
            output->addPreallocated(name);
      }
   }
   else
   {
      success = false;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("RunSmartCtlScan: device section missing in smartctl output"));
   }

   json_decref(root);
   return success;
}

/**
 * Handler for PhysicalDisk.Devices list
 */
LONG H_PhysicalDiskList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   return RunSmartCtlScan(value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

static inline void SetCellValueFromJson(Table *table, int column, json_t *root, const char *path)
{
   json_t *element = json_object_get_by_path_a(root, path);
   if (element == nullptr)
      return;

   TCHAR buffer[1024];
   GetValueFromJson(element, buffer);
   table->set(column, buffer);
}

/**
 * Handler for PhysicalDisk.Devices table
 */
LONG H_PhysicalDiskTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   StringList devices;
   if (!RunSmartCtlScan(&devices))
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
   value->addColumn(_T("PROTOCOL"), DCI_DT_STRING, _T("Protocol"));
   value->addColumn(_T("MODEL_FAMILY"), DCI_DT_STRING, _T("Family"));
   value->addColumn(_T("MODEL_NAME"), DCI_DT_STRING, _T("Model"));
   value->addColumn(_T("SERIAL_NUMBER"), DCI_DT_STRING, _T("Serial Number"));
   value->addColumn(_T("FIRMWARE_VERSION"), DCI_DT_STRING, _T("Firmware Version"));
   value->addColumn(_T("CAPACITY"), DCI_DT_UINT64, _T("Capacity"));

   for(int i = 0; i < devices.size(); i++)
   {
      const TCHAR *name = devices.get(i);

      value->addRow();
      value->set(0, name);

      json_t *info = RunSmartCtl(name, false);
      if (info == nullptr)
         continue;

      SetCellValueFromJson(value, 1, info, "device/type");
      SetCellValueFromJson(value, 2, info, "device/protocol");
      SetCellValueFromJson(value, 3, info, "model_family");
      SetCellValueFromJson(value, 4, info, "model_name");
      SetCellValueFromJson(value, 5, info, "serial_number");
      SetCellValueFromJson(value, 6, info, "firmware_version");
      SetCellValueFromJson(value, 7, info, "user_capacity/bytes");

      json_decref(info);
   }

   return SYSINFO_RC_SUCCESS;
}
