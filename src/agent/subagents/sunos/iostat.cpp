/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004-2011 Victor Kirhenshtein
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

#include "sunos_subagent.h"


//
// Constants
//

#define MAX_DEVICES		255
#define HISTORY_SIZE		60


//
// Device I/O stats structure
//

struct IO_STATS
{
   // device name
   char dev[KSTAT_STRLEN];

   // current values
   QWORD currBytesRead;
   QWORD currBytesWritten;
   DWORD currReadOps;
   DWORD currWriteOps;
   DWORD currQueue;

   // history - totals for queue, deltas for others
   QWORD histBytesRead[HISTORY_SIZE];
   QWORD histBytesWritten[HISTORY_SIZE];
   DWORD histReadOps[HISTORY_SIZE];
   DWORD histWriteOps[HISTORY_SIZE];
   DWORD histQueue[HISTORY_SIZE];
};

/**
 * Static data
 */
static IO_STATS s_data[MAX_DEVICES + 1];
static int s_currSlot = 0;	// current history slot

/**
 * Process stats for single device
 */
static void ProcessDeviceStats(const char *dev, kstat_io_t *kio)
{
   int i;

   // find device
   for(i = 1; i <= MAX_DEVICES; i++)
      if (!strcmp(dev, s_data[i].dev) || (s_data[i].dev[0] == 0))
         break;
   if (i > MAX_DEVICES)
      return;		// No more free slots

   if (s_data[i].dev[0] == 0)
   {
      // new device
      AgentWriteDebugLog(5, _T("SunOS: device %hs added to I/O stat collection"), dev);
      strcpy(s_data[i].dev, dev);
   }
   else
   {
      // existing device - update history
      s_data[i].histBytesRead[s_currSlot] = kio->nread - s_data[i].currBytesRead;
      s_data[i].histBytesWritten[s_currSlot] = kio->nwritten - s_data[i].currBytesWritten;
      s_data[i].histReadOps[s_currSlot] = kio->reads - s_data[i].currReadOps;
      s_data[i].histWriteOps[s_currSlot] = kio->writes - s_data[i].currWriteOps;
      s_data[i].histQueue[s_currSlot] = kio->wcnt + kio->rcnt;
   }

   // update current values
   s_data[i].currBytesRead = kio->nread;
   s_data[i].currBytesWritten = kio->nwritten;
   s_data[i].currReadOps = kio->reads;
   s_data[i].currWriteOps = kio->writes;
   s_data[i].currQueue = kio->wcnt + kio->rcnt;
}

/**
 * Calculate total values for all devices
 */
static void CalculateTotals()
{
   QWORD br = 0, bw = 0;
   DWORD r = 0, w = 0, q = 0;

   for(int i = 1; (i <= MAX_DEVICES) && (s_data[i].dev[0] != 0); i++)
   {
      br += s_data[i].histBytesRead[s_currSlot];
      bw += s_data[i].histBytesWritten[s_currSlot];
      r += s_data[i].histReadOps[s_currSlot];
      w += s_data[i].histWriteOps[s_currSlot];
      q += s_data[i].histQueue[s_currSlot];
   }

   s_data[0].histBytesRead[s_currSlot] = br;
   s_data[0].histBytesWritten[s_currSlot] = bw;
   s_data[0].histReadOps[s_currSlot] = r;
   s_data[0].histWriteOps[s_currSlot] = w;
   s_data[0].histQueue[s_currSlot] = q;
}

/**
 * I/O stat collector
 */
THREAD_RESULT THREAD_CALL IOStatCollector(void *arg)
{
   kstat_ctl_t *kc;
   kstat_t *kp;
   kstat_io_t kio;

   kstat_lock();
   kc = kstat_open();
   kstat_unlock();
   if (kc == NULL)
   {
      AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("SunOS::IOStatCollector: call to kstat_open failed (%s), I/O statistic will not be collected"), _tcserror(errno));
      return THREAD_OK;
   }

   memset(s_data, 0, sizeof(IO_STATS) * (MAX_DEVICES + 1));
   AgentWriteDebugLog(1, _T("SunOS: I/O stat collector thread started"));

   while(!g_bShutdown)
   {
      kstat_lock();
      kstat_chain_update(kc);
      for(kp = kc->kc_chain; kp != NULL; kp = kp->ks_next)
      {
         if (kp->ks_type == KSTAT_TYPE_IO)
         {
            kstat_read(kc, kp, &kio);
            ProcessDeviceStats(kp->ks_name, &kio);
         }
      }
      kstat_unlock();
      CalculateTotals();
      s_currSlot++;
      if (s_currSlot == HISTORY_SIZE)
         s_currSlot = 0;
      ThreadSleepMs(1000);
   }

   AgentWriteDebugLog(1, _T("SunOS: I/O stat collector thread stopped"));
   kstat_lock();
   kstat_close(kc);
   kstat_unlock();
   return THREAD_OK;
}

/**
 * Calculate average value for 32bit series
 */
static double CalculateAverage32(DWORD *series)
{
   double sum = 0;
   for(int i = 0; i < HISTORY_SIZE; i++)
      sum += series[i];
   return sum / (double)HISTORY_SIZE;
}

/**
 * Calculate min value for 32bit series
 */
static DWORD CalculateMin32(DWORD *series)
{
   DWORD val = series[0];
   for(int i = 1; i < HISTORY_SIZE; i++)
      if (series[i] < val)
         val = series[i];
   return val;
}

/**
 * Calculate max value for 32bit series
 */
static DWORD CalculateMax32(DWORD *series)
{
   DWORD val = series[0];
   for(int i = 1; i < HISTORY_SIZE; i++)
      if (series[i] > val)
         val = series[i];
   return val;
}

/**
 * Calculate average value for 64bit series
 */
static QWORD CalculateAverage64(QWORD *series)
{
   QWORD sum = 0;
   for(int i = 0; i < HISTORY_SIZE; i++)
      sum += series[i];
   return sum / HISTORY_SIZE;
}

/**
 * Calculate min value for 64bit series
 */
static QWORD CalculateMin64(QWORD *series)
{
   QWORD val = series[0];
   for(int i = 1; i < HISTORY_SIZE; i++)
      if (series[i] < val)
         val = series[i];
   return val;
}

/**
 * Calculate max value for 64bit series
 */
static QWORD CalculateMax64(QWORD *series)
{
   QWORD val = series[0];
   for(int i = 1; i < HISTORY_SIZE; i++)
      if (series[i] > val)
         val = series[i];
   return val;
}

/**
 * Get total I/O stat value
 */
LONG H_IOStatsTotal(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   switch(CAST_FROM_POINTER(arg, int))
   {
      case IOSTAT_NUM_READS:
         ret_double(value, CalculateAverage32(s_data[0].histReadOps));
         break;
      case IOSTAT_NUM_READS_MIN:
         ret_uint(value, CalculateMin32(s_data[0].histReadOps));
         break;
      case IOSTAT_NUM_READS_MAX:
         ret_uint(value, CalculateMax32(s_data[0].histReadOps));
         break;
      case IOSTAT_NUM_WRITES:
         ret_double(value, CalculateAverage32(s_data[0].histWriteOps));
         break;
      case IOSTAT_NUM_WRITES_MIN:
         ret_uint(value, CalculateMin32(s_data[0].histWriteOps));
         break;
      case IOSTAT_NUM_WRITES_MAX:
         ret_uint(value, CalculateMax32(s_data[0].histWriteOps));
         break;
      case IOSTAT_QUEUE:
         ret_double(value, CalculateAverage32(s_data[0].histQueue));
         break;
      case IOSTAT_QUEUE_MIN:
         ret_uint(value, CalculateMin32(s_data[0].histQueue));
         break;
      case IOSTAT_QUEUE_MAX:
         ret_uint(value, CalculateMax32(s_data[0].histQueue));
         break;
      case IOSTAT_NUM_RBYTES:
         ret_uint64(value, CalculateAverage64(s_data[0].histBytesRead));
         break;
      case IOSTAT_NUM_RBYTES_MIN:
         ret_uint64(value, CalculateMin64(s_data[0].histBytesRead));
         break;
      case IOSTAT_NUM_RBYTES_MAX:
         ret_uint64(value, CalculateMax64(s_data[0].histBytesRead));
         break;
      case IOSTAT_NUM_WBYTES:
         ret_uint64(value, CalculateAverage64(s_data[0].histBytesWritten));
         break;
      case IOSTAT_NUM_WBYTES_MIN:
         ret_uint64(value, CalculateMin64(s_data[0].histBytesWritten));
         break;
      case IOSTAT_NUM_WBYTES_MAX:
         ret_uint64(value, CalculateMax64(s_data[0].histBytesWritten));
         break;
      default:
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   return rc;
}

/**
 * Get I/O stat for specific device
 */
LONG H_IOStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char device[KSTAT_STRLEN];
   LONG rc = SYSINFO_RC_SUCCESS;
   int i;

   if (!AgentGetParameterArgA(cmd, 1, device, KSTAT_STRLEN))
      return SYSINFO_RC_UNSUPPORTED;

   // find device
   for(i = 1; (i <= MAX_DEVICES) && (s_data[i].dev[0] != 0); i++)
      if (!strcmp(device, s_data[i].dev))
         break;
   if ((i > MAX_DEVICES) || (s_data[i].dev[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;	// unknown device

   switch(CAST_FROM_POINTER(arg, int))
   {
      case IOSTAT_NUM_READS:
         ret_double(value, CalculateAverage32(s_data[i].histReadOps));
         break;
      case IOSTAT_NUM_READS_MIN:
         ret_uint(value, CalculateMin32(s_data[i].histReadOps));
         break;
      case IOSTAT_NUM_READS_MAX:
         ret_uint(value, CalculateMax32(s_data[i].histReadOps));
         break;
      case IOSTAT_NUM_WRITES:
         ret_double(value, CalculateAverage32(s_data[i].histWriteOps));
         break;
      case IOSTAT_NUM_WRITES_MIN:
         ret_uint(value, CalculateMin32(s_data[i].histWriteOps));
         break;
      case IOSTAT_NUM_WRITES_MAX:
         ret_uint(value, CalculateMax32(s_data[i].histWriteOps));
         break;
      case IOSTAT_QUEUE:
         ret_double(value, CalculateAverage32(s_data[i].histQueue));
         break;
      case IOSTAT_QUEUE_MIN:
         ret_uint(value, CalculateMin32(s_data[i].histQueue));
         break;
      case IOSTAT_QUEUE_MAX:
         ret_uint(value, CalculateMax32(s_data[i].histQueue));
         break;
      case IOSTAT_NUM_RBYTES:
         ret_uint64(value, CalculateAverage64(s_data[i].histBytesRead));
         break;
      case IOSTAT_NUM_RBYTES_MIN:
         ret_uint64(value, CalculateMin64(s_data[i].histBytesRead));
         break;
      case IOSTAT_NUM_RBYTES_MAX:
         ret_uint64(value, CalculateMax64(s_data[i].histBytesRead));
         break;
      case IOSTAT_NUM_WBYTES:
         ret_uint64(value, CalculateAverage64(s_data[i].histBytesWritten));
         break;
      case IOSTAT_NUM_WBYTES_MIN:
         ret_uint64(value, CalculateMin64(s_data[i].histBytesWritten));
         break;
      case IOSTAT_NUM_WBYTES_MAX:
         ret_uint64(value, CalculateMax64(s_data[i].histBytesWritten));
         break;
      default:
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   return rc;
}

