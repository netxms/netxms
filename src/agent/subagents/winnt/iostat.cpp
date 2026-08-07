/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: iostat.cpp
**
**/

#include "winnt_subagent.h"

#define MAX_PHYSICAL_DISK_COUNT  1024

/**
 * Get physical disk numbers
 */
static int GetPhysicalDiskNumbers(UINT32 *disks)
{
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum"), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
      return 0;

   int count = 0;
   UINT32 index = 0;
   TCHAR name[256];
   DWORD size = 256;
   while (RegEnumValue(hKey, index++, name, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
   {
      if (!_tcscmp(name, _T("0")))
      {
         disks[count++] = 0;
      }
      else
      {
         TCHAR *eptr;
         UINT32 v = _tcstoul(name, &eptr, 10);
         if ((v > 0) && (*eptr == 0))
         {
            disks[count++] = v;
         }
      }
      size = 256;
   }
   RegCloseKey(hKey);
   return count;
}

/**
 * Get performance data for given pysical disk
 */
static BOOL GetDiskPerformanceData(UINT32 number, DISK_PERFORMANCE *perfdata)
{
   TCHAR path[128];
   _sntprintf(path, 128, _T("\\\\.\\PhysicalDrive%u"), number);
   HANDLE device = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if (device == INVALID_HANDLE_VALUE)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetDiskPerformanceData: cannot open device %s (%s)"), path, GetSystemErrorText(GetLastError(), buffer, 1024));
      return FALSE;
   }

   DWORD bytes;
   BOOL success = DeviceIoControl(device, IOCTL_DISK_PERFORMANCE, NULL, 0, perfdata, sizeof(DISK_PERFORMANCE), &bytes, NULL);
   if (!success)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetDiskInfo: GetDiskPerformanceData on device %s failed (%s)"), path, GetSystemErrorText(GetLastError(), buffer, 1024));
   }
   CloseHandle(device);
   return success;
}

/**
 * Number of samples per minute
 */
#define SAMPLES_PER_MINUTE 60

/**
 * Calculate delta between two readings of monotonic counter. Returns 0 if counter was reset or went backwards.
 */
static inline uint64_t CounterDelta(int64_t currentValue, int64_t lastValue)
{
   return (currentValue > lastValue) ? static_cast<uint64_t>(currentValue - lastValue) : 0;
}

/**
 * Performance data for single device
 */
struct DiskPerfStats
{
   uint32_t device;
   int collectionFailures;
   int nextSample;
   uint64_t bytesRead[SAMPLES_PER_MINUTE];
   uint64_t bytesWritten[SAMPLES_PER_MINUTE];
   uint32_t readCount[SAMPLES_PER_MINUTE];
   uint32_t writeCount[SAMPLES_PER_MINUTE];
   uint32_t diskTime[SAMPLES_PER_MINUTE];
   uint32_t readTime[SAMPLES_PER_MINUTE];
   uint32_t writeTime[SAMPLES_PER_MINUTE];
   uint32_t queueDepth[SAMPLES_PER_MINUTE];
   DISK_PERFORMANCE lastRawData;

   DiskPerfStats(UINT32 dev)
   {
      device = dev;
      memset(bytesRead, 0, sizeof(bytesRead));
      memset(bytesWritten, 0, sizeof(bytesWritten));
      memset(readCount, 0, sizeof(readCount));
      memset(writeCount, 0, sizeof(writeCount));
      memset(diskTime, 0, sizeof(diskTime));
      memset(readTime, 0, sizeof(readTime));
      memset(writeTime, 0, sizeof(writeTime));
      memset(queueDepth, 0, sizeof(queueDepth));
      collectionFailures = 1; // Force first update to only save data
      nextSample = 0;
   }

   void update(const DISK_PERFORMANCE &currentRawData)
   {
      if (collectionFailures == 0)
      {
         bytesRead[nextSample] = CounterDelta(currentRawData.BytesRead.QuadPart, lastRawData.BytesRead.QuadPart);
         bytesWritten[nextSample] = CounterDelta(currentRawData.BytesWritten.QuadPart, lastRawData.BytesWritten.QuadPart);
         readCount[nextSample] = static_cast<uint32_t>(CounterDelta(currentRawData.ReadCount, lastRawData.ReadCount));
         writeCount[nextSample] = static_cast<uint32_t>(CounterDelta(currentRawData.WriteCount, lastRawData.WriteCount));
         queueDepth[nextSample] = currentRawData.QueueDepth;
         // Time in DISK_PERFORMANCE expressed in 100-nanosecond intervals
         // Use IdleTime to compute disk busy time (ReadTime + WriteTime can overlap on devices with concurrent I/O)
         uint64_t idleDelta = CounterDelta(currentRawData.IdleTime.QuadPart, lastRawData.IdleTime.QuadPart) / 10000;
         diskTime[nextSample] = (idleDelta < 1000) ? static_cast<uint32_t>(1000 - idleDelta) : 0;
         readTime[nextSample] = static_cast<uint32_t>(CounterDelta(currentRawData.ReadTime.QuadPart, lastRawData.ReadTime.QuadPart) / 10000);
         writeTime[nextSample] = static_cast<uint32_t>(CounterDelta(currentRawData.WriteTime.QuadPart, lastRawData.WriteTime.QuadPart) / 10000);
         nextSample++;
         if (nextSample == SAMPLES_PER_MINUTE)
            nextSample = 0;
      }
      else
      {
         collectionFailures = 0;
      }
      memcpy(&lastRawData, &currentRawData, sizeof(DISK_PERFORMANCE));
   }
};

/**
 * Collected disk performance data
 */
static HashMap<UINT32, DiskPerfStats> s_diskPerfStats(Ownership::True);
static win_mutex_t s_diskPerfStatsLock;

/**
 * Add devices for I/O stats collection
 */
static void AddDevicesForIOStatCollection()
{
   UINT32 devices[MAX_PHYSICAL_DISK_COUNT];
   int count = GetPhysicalDiskNumbers(devices);
   if (count == 0)
      return;

   LockMutex(&s_diskPerfStatsLock, INFINITE);
   for (int i = 0; i < count; i++)
   {
      if (!s_diskPerfStats.contains(devices[i]))
      {
         DiskPerfStats *ps = new DiskPerfStats(devices[i]);
         s_diskPerfStats.set(ps->device, ps);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Physical disk %u added to I/O stat collector"), ps->device);
      }
   }
   UnlockMutex(&s_diskPerfStatsLock);
}

/**
 * I/O stat collector thread
 */
static void IOStatColector()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("I/O stat collector thread started"));
   AddDevicesForIOStatCollection();

   int spinCount = 0;
   while (!AgentSleepAndCheckForShutdown(1000))
   {
      // Update list of devices once per minute
      if (spinCount++ == 60)
      {
         spinCount = 0;
         AddDevicesForIOStatCollection();
      }

      LockMutex(&s_diskPerfStatsLock, INFINITE);
      auto it = s_diskPerfStats.begin();
      while (it.hasNext())
      {
         DiskPerfStats *ps = it.next();
         DISK_PERFORMANCE rawData;
         if (GetDiskPerformanceData(ps->device, &rawData))
         {
            ps->update(rawData);
         }
         else
         {
            ps->collectionFailures++;
            if (ps->collectionFailures > 600)
            {
               // Performance data cannot be collected for more than 10 minutes
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Physical disk %u removed from I/O stat collector"), ps->device);
               it.remove();
            }
         }
      }
      UnlockMutex(&s_diskPerfStatsLock);
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("I/O stat collector thread stopped"));
}

/**
 * I/O stat collector thread handle
 */
static THREAD s_ioStatCollectorThread = INVALID_THREAD_HANDLE;

/**
 * Start I/O stat collector
 */
void StartIOStatCollector()
{
   InitializeMutex(&s_diskPerfStatsLock, 1000);
   s_ioStatCollectorThread = ThreadCreateEx(IOStatColector);
}

/**
 * Stop I/O stat collector
 */
void StopIOStatCollector()
{
   ThreadJoin(s_ioStatCollectorThread);
   DestroyMutex(&s_diskPerfStatsLock);
}

/**
 * Get average value from array
 */
template <typename T> double GetAverageValue(T *data)
{
   double total = 0;
   for (int i = 0; i < SAMPLES_PER_MINUTE; i++)
      total += data[i];
   return total / SAMPLES_PER_MINUTE;
}

/**
 * Get metric value for single device. Caller must hold s_diskPerfStatsLock.
 */
static bool GetDeviceMetricValue(DiskPerfStats *ps, IOInfoType metric, double *value)
{
   switch (metric)
   {
      case IOSTAT_DISK_QUEUE:
         *value = GetAverageValue(ps->queueDepth);
         break;
      case IOSTAT_IO_READ_TIME:
         *value = GetAverageValue(ps->readTime) / 10; // milliseconds to %
         break;
      case IOSTAT_IO_TIME:
         *value = GetAverageValue(ps->diskTime) / 10; // milliseconds to %
         break;
      case IOSTAT_IO_WRITE_TIME:
         *value = GetAverageValue(ps->writeTime) / 10; // milliseconds to %
         break;
      case IOSTAT_NUM_READS:
         *value = GetAverageValue(ps->readCount);
         break;
      case IOSTAT_NUM_SREADS:
         *value = GetAverageValue(ps->bytesRead);
         break;
      case IOSTAT_NUM_SWRITES:
         *value = GetAverageValue(ps->bytesWritten);
         break;
      case IOSTAT_NUM_WRITES:
         *value = GetAverageValue(ps->writeCount);
         break;
      default:
         return false;
   }
   return true;
}

/**
 * Format metric value according to metric's data type
 */
static inline void RetMetricValue(TCHAR *buffer, IOInfoType metric, double value)
{
   if ((metric == IOSTAT_NUM_SREADS) || (metric == IOSTAT_NUM_SWRITES))
      ret_uint64(buffer, static_cast<uint64_t>(value));
   else
      ret_double(buffer, value);
}

/**
 * Handler for System.IO.* parameters (with instance)
 */
LONG H_IoStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR deviceName[64];
   if (!AgentGetParameterArg(param, 1, deviceName, 64))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR *eptr;
   UINT32 device = _tcstoul(deviceName, &eptr, 0);
   if (*eptr != 0)
      return SYSINFO_RC_UNSUPPORTED;

   IOInfoType metric = CAST_FROM_POINTER(arg, IOInfoType);

   double metricValue = 0;
   LockMutex(&s_diskPerfStatsLock, INFINITE);
   DiskPerfStats *ps = s_diskPerfStats.get(device);
   LONG rc;
   if (ps != nullptr)
   {
      rc = GetDeviceMetricValue(ps, metric, &metricValue) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   }
   UnlockMutex(&s_diskPerfStatsLock);

   if (rc == SYSINFO_RC_SUCCESS)
      RetMetricValue(value, metric, metricValue);
   return rc;
}

/**
 * Handler for System.IO.* parameters (totals)
 */
LONG H_IoStatsTotal(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   IOInfoType metric = CAST_FROM_POINTER(arg, IOInfoType);

   // Summing percentages is not meaningful, so for time-based metrics report maximum across all disks
   bool useMaximum = (metric == IOSTAT_IO_TIME) || (metric == IOSTAT_IO_READ_TIME) || (metric == IOSTAT_IO_WRITE_TIME);

   double total = 0;
   bool success = true;
   LockMutex(&s_diskPerfStatsLock, INFINITE);
   for (DiskPerfStats *ps : s_diskPerfStats)
   {
      double v;
      if (!GetDeviceMetricValue(ps, metric, &v))
      {
         success = false;
         break;
      }
      if (useMaximum)
      {
         if (v > total)
            total = v;
      }
      else
      {
         total += v;
      }
   }
   UnlockMutex(&s_diskPerfStatsLock);

   if (!success)
      return SYSINFO_RC_UNSUPPORTED;

   RetMetricValue(value, metric, total);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.IO.Devices list
 */
LONG H_IoDeviceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LockMutex(&s_diskPerfStatsLock, INFINITE);
   for (DiskPerfStats *ps : s_diskPerfStats)
      value->add(ps->device);
   UnlockMutex(&s_diskPerfStatsLock);
   return SYSINFO_RC_SUCCESS;
}
