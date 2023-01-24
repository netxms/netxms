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
 * Executes smartctl command for given device
 *
 * @returns smartctl output as json_t object or nullptr on failure
 */
json_t* RunSmartCtl(const TCHAR *device)
{
   TCHAR cmd[1024];
#ifdef _WIN32
   _sntprintf(cmd, 1024, _T("smartctl.exe --all %s -j 2>NUL"), device);
#else
   _sntprintf(cmd, 1024, _T("smartctl --all %s -j 2>/dev/null"), device);
#endif

   OutputCapturingProcessExecutor pe(cmd);
   if (!pe.execute())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to execute smartctl for device \"%s\""), device);
      return nullptr;
   }
   if (!pe.waitForCompletion(10000))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("smartctl for device \"%s\" timed out"), device);
      return nullptr;
   }

   json_error_t error;
   json_t* root = json_loads(pe.getOutput(), 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Error parsing smartctl JSON output for device \"%s\" (line %d: %hs)"), device, error.line, error.text);
      return nullptr;
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
 * Handler for PhysicalDisk.*
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
      root = RunSmartCtl(device);
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
