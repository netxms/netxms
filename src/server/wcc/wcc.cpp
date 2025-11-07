/*
** NetXMS - Network Management System
** Copyright (C) 2023-2024 Raden Solutions
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
** File: wcc.cpp
**
**/

#include "wcc.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("WCC", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Unique client counters
 */
static HashSet<MacAddress> s_uniqueClients;
static Mutex s_mutex(MutexType::FAST);

/**
 * Topology poll hook
 */
static void OnTopologyPoll(Node *node, ClientSession *session, uint32_t rqId, PollerInfo *poller)
{
   if (!node->isWirelessAccessPoint() && !node->isWirelessController())
      return;

   nxlog_debug_tag(WCC_DEBUG_TAG, 5, _T("Processing wireless %s \"%s\" [%u]"), node->isWirelessController() ? _T("controller") : _T("acecss point"), node->getName(), node->getId());
   ObjectArray<WirelessStationInfo> *stations = node->getWirelessStations();
   if (stations == nullptr)
   {
      nxlog_debug_tag(WCC_DEBUG_TAG, 5, _T("Cannot get list of wireless stations from \"%s\" [%u]"), node->getName(), node->getId());
      return;
   }

   s_mutex.lock();
   for(int i = 0; i < stations->size(); i++)
   {
      s_uniqueClients.put(MacAddress(stations->get(i)->macAddr, MAC_ADDR_LENGTH));
   }
   int count = s_uniqueClients.size();
   s_mutex.unlock();

   delete stations;
   nxlog_debug_tag(WCC_DEBUG_TAG, 5, _T("%d unique clients after processing \"%s\" [%u]"), count, node->getName(), node->getId());
}

/**
 * Get internal metric
 */
static DataCollectionError GetInternalMetric(DataCollectionTarget *object, const TCHAR *metric, TCHAR *value, size_t size)
{
   if (!_tcsicmp(metric, _T("WCC.UniqueClients")))
   {
      s_mutex.lock();
      ret_int(value, s_uniqueClients.size());
      s_mutex.unlock();
      return DCE_SUCCESS;
   }
   return DCE_NOT_SUPPORTED;
}

/**
 * Save collected data
 */
static void SaveData()
{
   time_t timestamp = time(nullptr);

   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirData, path);
   _tcslcat(path, FS_PATH_SEPARATOR _T("wcc.data"), MAX_PATH);

   FILE *f = _tfopen(path, _T("wb"));
   if (f == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, WCC_DEBUG_TAG, _T("Cannot open data file \"%s\" (%s)"), path, _tcserror(errno));
      return;
   }

   fwrite(&timestamp, sizeof(time_t), 1, f);

   s_mutex.lock();

   int32_t size = s_uniqueClients.size();
   fwrite(&size, sizeof(int32_t), 1, f);

   s_uniqueClients.forEach(
      [] (const MacAddress *a, void *f) -> EnumerationCallbackResult
      {
         fwrite(a->value(), MAC_ADDR_LENGTH, 1, static_cast<FILE*>(f));
         return _CONTINUE;
      }, f);

   s_mutex.unlock();

   fclose(f);

   nxlog_debug_tag(WCC_DEBUG_TAG, 5, _T("%d wireless station addresses saved to \"%s\""), size, path);
}

/**
 * Reset collected data at midnight
 */
static void ResetData()
{
   s_mutex.lock();
   s_uniqueClients.clear();
   s_mutex.unlock();

   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirData, path);
   _tcslcat(path, FS_PATH_SEPARATOR _T("wcc.data"), MAX_PATH);
   _tremove(path);

   ThreadPoolScheduleRelative(g_mainThreadPool, GetSleepTime(0, 0, 0) * 1000, ResetData);
}

/**
 * Timer for periodically saving collected data
 */
static void SaveDataTimer()
{
   SaveData();
   ThreadPoolScheduleRelative(g_mainThreadPool, 3600000, SaveDataTimer);
}

/**
 * Server start handler
 */
static void OnServerStart()
{
   ThreadPoolScheduleRelative(g_mainThreadPool, GetSleepTime(0, 0, 0) * 1000, ResetData);
   ThreadPoolScheduleRelative(g_mainThreadPool, 3600000, SaveDataTimer);
}

/**
 * Shutdown handler
 */
static void OnShutdown()
{
   SaveData();
}

/**
 * Initialize module
 */
static bool Initialize(Config *config)
{
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirData, path);
   _tcslcat(path, FS_PATH_SEPARATOR _T("wcc.data"), MAX_PATH);

   FILE *f = _tfopen(path, _T("rb"));
   if (f == nullptr)
   {
      nxlog_debug_tag(WCC_DEBUG_TAG, 3, _T("Cannot load collected wireless station data from file \"%s\" (%s)"), path, _tcserror(errno));
      return true;
   }

   time_t timestamp;
   if (fread(&timestamp, sizeof(time_t), 1, f) == 1)
   {
      time_t now = time(nullptr);
      struct tm *currTime = localtime(&now);
      struct tm *savedTime = localtime(&timestamp);
      if ((currTime->tm_year == savedTime->tm_year) && (currTime->tm_yday == savedTime->tm_yday))
      {
         int32_t count;
         if (fread(&count, sizeof(int32_t), 1, f) == 1)
         {
            s_mutex.lock();
            for(int i = 0; i < count; i++)
            {
               BYTE mac[MAC_ADDR_LENGTH];
               if (fread(mac, 6, 1, f) < 1)
                  break;
               s_uniqueClients.put(MacAddress(mac, MAC_ADDR_LENGTH));
            }
            s_mutex.unlock();
            nxlog_debug_tag(WCC_DEBUG_TAG, 3, _T("%d wireless station addresses loaded"), count);
         }
         else
         {
            nxlog_debug_tag(WCC_DEBUG_TAG, 3, _T("Premature end of file while reading wireless station data"));
         }
      }
      else
      {
         nxlog_debug_tag(WCC_DEBUG_TAG, 3, _T("Collected wireless station data discarded as it is from different day"));
      }
   }
   else
   {
      nxlog_debug_tag(WCC_DEBUG_TAG, 3, _T("Premature end of file while reading wireless station data"));
   }

   fclose(f);
   return true;
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"WCC");
   module->pfInitialize = Initialize;
   module->pfServerStarted = OnServerStart;
   module->pfShutdown = OnShutdown;
   module->pfTopologyPollHook = OnTopologyPoll;
   module->pfGetInternalMetric = GetInternalMetric;
   return true;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
