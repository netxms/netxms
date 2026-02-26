/*
** NetXMS subagent for Darwin (macOS)
** Copyright (C) 2012-2026 Raden Solutions
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

#include <nms_common.h>
#include <nms_agent.h>
#include "darwin.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/storage/IOMedia.h>
#include <CoreFoundation/CoreFoundation.h>

#define MAX_DEVICES    256
#define HISTORY_SIZE   60

/**
 * Get 64-bit integer from CFDictionary
 */
static int64_t GetCFDictionaryInt64(CFDictionaryRef dict, CFStringRef key)
{
   CFNumberRef num = static_cast<CFNumberRef>(CFDictionaryGetValue(dict, key));
   if (num == nullptr)
      return 0;
   int64_t value = 0;
   CFNumberGetValue(num, kCFNumberSInt64Type, &value);
   return value;
}

/**
 * Device I/O stats structure
 */
struct IO_STATS
{
   char dev[64];

   // current cumulative values
   uint64_t currBytesRead;
   uint64_t currBytesWritten;
   uint64_t currReadOps;
   uint64_t currWriteOps;
   uint64_t currReadTimeNs;
   uint64_t currWriteTimeNs;

   // history (per-second deltas)
   uint64_t histBytesRead[HISTORY_SIZE];
   uint64_t histBytesWritten[HISTORY_SIZE];
   uint32_t histReadOps[HISTORY_SIZE];
   uint32_t histWriteOps[HISTORY_SIZE];
   uint32_t histIoTime[HISTORY_SIZE];
};

/**
 * Static data
 */
static IO_STATS s_data[MAX_DEVICES + 1];
static int s_currSlot = 0;
static THREAD s_collectorThread = INVALID_THREAD_HANDLE;
static Mutex s_dataLock(MutexType::FAST);
static bool s_shutdown = false;

/**
 * Process stats for a single device
 */
static void ProcessDeviceStats(const char *name, uint64_t bytesRead, uint64_t bytesWritten,
                               uint64_t readOps, uint64_t writeOps, uint64_t readTimeNs, uint64_t writeTimeNs)
{
   int i;
   for (i = 1; i <= MAX_DEVICES; i++)
   {
      if (!strcmp(name, s_data[i].dev) || (s_data[i].dev[0] == 0))
         break;
   }
   if (i > MAX_DEVICES)
      return;

   if (s_data[i].dev[0] == 0)
   {
      // New device
      strlcpy(s_data[i].dev, name, sizeof(s_data[i].dev));
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Device %hs added to I/O stat collection"), name);
   }
   else
   {
      // Existing device - update history
      s_data[i].histBytesRead[s_currSlot] = bytesRead - s_data[i].currBytesRead;
      s_data[i].histBytesWritten[s_currSlot] = bytesWritten - s_data[i].currBytesWritten;
      s_data[i].histReadOps[s_currSlot] = static_cast<uint32_t>(readOps - s_data[i].currReadOps);
      s_data[i].histWriteOps[s_currSlot] = static_cast<uint32_t>(writeOps - s_data[i].currWriteOps);
      // macOS has no single busy_time counter; sum read+write time and cap at 1 second
      uint64_t readDeltaMs = (readTimeNs - s_data[i].currReadTimeNs) / 1000000;
      uint64_t writeDeltaMs = (writeTimeNs - s_data[i].currWriteTimeNs) / 1000000;
      uint64_t totalMs = readDeltaMs + writeDeltaMs;
      s_data[i].histIoTime[s_currSlot] = static_cast<uint32_t>((totalMs <= 1000) ? totalMs : 1000);
   }

   s_data[i].currBytesRead = bytesRead;
   s_data[i].currBytesWritten = bytesWritten;
   s_data[i].currReadOps = readOps;
   s_data[i].currWriteOps = writeOps;
   s_data[i].currReadTimeNs = readTimeNs;
   s_data[i].currWriteTimeNs = writeTimeNs;
}

/**
 * Calculate total values for throughput metrics (sum across all devices)
 */
static void CalculateTotals()
{
   uint64_t br = 0, bw = 0;
   uint32_t r = 0, w = 0;

   for (int i = 1; (i <= MAX_DEVICES) && (s_data[i].dev[0] != 0); i++)
   {
      br += s_data[i].histBytesRead[s_currSlot];
      bw += s_data[i].histBytesWritten[s_currSlot];
      r += s_data[i].histReadOps[s_currSlot];
      w += s_data[i].histWriteOps[s_currSlot];
   }

   s_data[0].histBytesRead[s_currSlot] = br;
   s_data[0].histBytesWritten[s_currSlot] = bw;
   s_data[0].histReadOps[s_currSlot] = r;
   s_data[0].histWriteOps[s_currSlot] = w;
}

/**
 * Collect disk statistics from IOKit
 */
static void CollectDiskStats()
{
   CFMutableDictionaryRef match = IOServiceMatching(kIOBlockStorageDriverClass);
   if (match == nullptr)
      return;

   io_iterator_t iter;
   if (IOServiceGetMatchingServices(kIOMasterPortDefault, match, &iter) != KERN_SUCCESS)
      return;

   io_object_t driver;
   while ((driver = IOIteratorNext(iter)) != 0)
   {
      // Get Statistics dictionary from the driver
      CFDictionaryRef properties = nullptr;
      if (IORegistryEntryCreateCFProperties(driver, (CFMutableDictionaryRef *)&properties, kCFAllocatorDefault, 0) != KERN_SUCCESS || properties == nullptr)
      {
         IOObjectRelease(driver);
         continue;
      }

      CFDictionaryRef stats = static_cast<CFDictionaryRef>(CFDictionaryGetValue(properties, CFSTR(kIOBlockStorageDriverStatisticsKey)));
      if (stats == nullptr)
      {
         CFRelease(properties);
         IOObjectRelease(driver);
         continue;
      }

      // Find the child IOMedia to get BSD Name
      char devName[64] = "";
      io_iterator_t childIter;
      if (IORegistryEntryGetChildIterator(driver, kIOServicePlane, &childIter) == KERN_SUCCESS)
      {
         io_object_t child;
         while ((child = IOIteratorNext(childIter)) != 0)
         {
            CFStringRef bsdName = static_cast<CFStringRef>(IORegistryEntryCreateCFProperty(child, CFSTR(kIOBSDNameKey), kCFAllocatorDefault, 0));
            if (bsdName != nullptr)
            {
               CFStringGetCString(bsdName, devName, sizeof(devName), kCFStringEncodingASCII);
               CFRelease(bsdName);
               IOObjectRelease(child);
               break;
            }
            IOObjectRelease(child);
         }
         IOObjectRelease(childIter);
      }

      if (devName[0] != 0)
      {
         ProcessDeviceStats(devName,
            static_cast<uint64_t>(GetCFDictionaryInt64(stats, CFSTR(kIOBlockStorageDriverStatisticsBytesReadKey))),
            static_cast<uint64_t>(GetCFDictionaryInt64(stats, CFSTR(kIOBlockStorageDriverStatisticsBytesWrittenKey))),
            static_cast<uint64_t>(GetCFDictionaryInt64(stats, CFSTR(kIOBlockStorageDriverStatisticsReadsKey))),
            static_cast<uint64_t>(GetCFDictionaryInt64(stats, CFSTR(kIOBlockStorageDriverStatisticsWritesKey))),
            static_cast<uint64_t>(GetCFDictionaryInt64(stats, CFSTR(kIOBlockStorageDriverStatisticsTotalReadTimeKey))),
            static_cast<uint64_t>(GetCFDictionaryInt64(stats, CFSTR(kIOBlockStorageDriverStatisticsTotalWriteTimeKey))));
      }

      CFRelease(properties);
      IOObjectRelease(driver);
   }
   IOObjectRelease(iter);
}

/**
 * I/O stat collector thread
 */
static void IOStatCollector()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("I/O stat collector thread started"));

   memset(s_data, 0, sizeof(s_data));

   while (!s_shutdown)
   {
      s_dataLock.lock();
      CollectDiskStats();
      CalculateTotals();
      s_currSlot++;
      if (s_currSlot == HISTORY_SIZE)
         s_currSlot = 0;
      s_dataLock.unlock();
      ThreadSleepMs(1000);
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("I/O stat collector thread stopped"));
}

/**
 * Start I/O stat collector
 */
void StartIOStatCollector()
{
   s_shutdown = false;
   s_collectorThread = ThreadCreateEx(IOStatCollector);
}

/**
 * Shutdown I/O stat collector
 */
void ShutdownIOStatCollector()
{
   s_shutdown = true;
   ThreadJoin(s_collectorThread);
}

/**
 * Calculate average value for 32-bit series
 */
static double CalculateAverage32(uint32_t *series)
{
   double sum = 0;
   for (int i = 0; i < HISTORY_SIZE; i++)
      sum += series[i];
   return sum / static_cast<double>(HISTORY_SIZE);
}

/**
 * Calculate average value for 64-bit series
 */
static uint64_t CalculateAverage64(uint64_t *series)
{
   uint64_t sum = 0;
   for (int i = 0; i < HISTORY_SIZE; i++)
      sum += series[i];
   return sum / HISTORY_SIZE;
}

/**
 * Handler for System.IO.* total parameters
 */
LONG H_IOStatsTotal(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_dataLock.lock();
   switch (CAST_FROM_POINTER(arg, int))
   {
      case IOSTAT_NUM_READS:
         ret_double(value, CalculateAverage32(s_data[0].histReadOps));
         break;
      case IOSTAT_NUM_WRITES:
         ret_double(value, CalculateAverage32(s_data[0].histWriteOps));
         break;
      case IOSTAT_NUM_RBYTES:
         ret_uint64(value, CalculateAverage64(s_data[0].histBytesRead));
         break;
      case IOSTAT_NUM_WBYTES:
         ret_uint64(value, CalculateAverage64(s_data[0].histBytesWritten));
         break;
      case IOSTAT_IO_TIME:
      {
         // Report maximum disk utilization across all devices
         double maxValue = 0;
         for (int i = 1; (i <= MAX_DEVICES) && (s_data[i].dev[0] != 0); i++)
         {
            double v = CalculateAverage32(s_data[i].histIoTime) / 10;
            if (v > maxValue)
               maxValue = v;
         }
         ret_double(value, maxValue);
         break;
      }
      default:
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   s_dataLock.unlock();
   return rc;
}

/**
 * Handler for System.IO.* per-device parameters
 */
LONG H_IOStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char device[64];
   if (!AgentGetParameterArgA(cmd, 1, device, sizeof(device)))
      return SYSINFO_RC_UNSUPPORTED;

   int i;
   for (i = 1; (i <= MAX_DEVICES) && (s_data[i].dev[0] != 0); i++)
      if (!strcmp(device, s_data[i].dev))
         break;
   if ((i > MAX_DEVICES) || (s_data[i].dev[0] == 0))
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   LONG rc = SYSINFO_RC_SUCCESS;

   s_dataLock.lock();
   switch (CAST_FROM_POINTER(arg, int))
   {
      case IOSTAT_NUM_READS:
         ret_double(value, CalculateAverage32(s_data[i].histReadOps));
         break;
      case IOSTAT_NUM_WRITES:
         ret_double(value, CalculateAverage32(s_data[i].histWriteOps));
         break;
      case IOSTAT_NUM_RBYTES:
         ret_uint64(value, CalculateAverage64(s_data[i].histBytesRead));
         break;
      case IOSTAT_NUM_WBYTES:
         ret_uint64(value, CalculateAverage64(s_data[i].histBytesWritten));
         break;
      case IOSTAT_IO_TIME:
         ret_double(value, CalculateAverage32(s_data[i].histIoTime) / 10);
         break;
      default:
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   s_dataLock.unlock();
   return rc;
}

/**
 * Handler for System.IO.Devices list
 */
LONG H_IODeviceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   s_dataLock.lock();
   for (int i = 1; (i <= MAX_DEVICES) && (s_data[i].dev[0] != 0); i++)
   {
#ifdef UNICODE
      value->addMBString(s_data[i].dev);
#else
      value->add(s_data[i].dev);
#endif
   }
   s_dataLock.unlock();
   return SYSINFO_RC_SUCCESS;
}
