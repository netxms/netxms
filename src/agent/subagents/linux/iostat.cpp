/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2021 Raden Solutions
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
**/

#include <linux_subagent.h>

#define SAMPLES_PER_MINUTE		60

/**
 * One sample structure
 */
struct IOSTAT_SAMPLE
{
	unsigned long stats[9];
};

/**
 * Device structure
 */
struct IOSTAT_DEVICE
{
   char name[64];
   char sysfsName[64];
   bool isRealDevice;
   bool isFirstRead;
   unsigned long lastReads;
   unsigned long lastWrites;
   unsigned long lastReadWaitTime;
   unsigned long lastWriteWaitTime;
   IOSTAT_SAMPLE samples[SAMPLES_PER_MINUTE];
};

/**
 * Static data
 */
static Condition s_stopCondition(true);
static THREAD s_collectorThread = INVALID_THREAD_HANDLE;
static const char *s_statFile = "/proc/diskstats";
static IOSTAT_DEVICE *s_devices = nullptr;
static int s_deviceCount = 0;
static int s_currSample = 0;
static Mutex s_dataAccessLock(MutexType::FAST);
static bool s_isSysFsAvailable = false;

/**
 * Check if given disk is a device, not partition
 */
static bool IsRealDevice(const char *name)
{
   if (!s_isSysFsAvailable)
      return false;  // Unable to check

   // Check using /sys/block
   char path[MAX_PATH];
   snprintf(path, MAX_PATH, "/sys/block/%s", name);
   return access(path, F_OK) == 0;
}

/**
 * Read and parse /sys/block/<dev>/stat
 */
static void ReadSysFsStat(IOSTAT_DEVICE *d)
{
   char fname[MAX_PATH];
   snprintf(fname, MAX_PATH, "/sys/block/%s/stat", d->sysfsName);
   FILE *f = fopen(fname, "r");
   if (f != nullptr)
   {
      char line[1024];
      if (fgets(line, 1024, f) != nullptr)
      {
         unsigned long reads, writes, readWaitTime, writeWaitTime;
         int count = sscanf(line, "%ld %*d %*d %ld %ld %*d %*d %ld", &reads, &readWaitTime, &writes, &writeWaitTime);
         if (count == 4)
         {
            IOSTAT_SAMPLE *s = &d->samples[s_currSample];
            if (d->isFirstRead)
            {
               s->stats[IOSTAT_READ_WAIT_TIME] = 0;
               s->stats[IOSTAT_WRITE_WAIT_TIME] = 0;
               s->stats[IOSTAT_WAIT_TIME] = 0;
               d->isFirstRead = false;
            }
            else
            {
               unsigned long diffReads = reads - d->lastReads;
               unsigned long diffReadWaitTime = readWaitTime - d->lastReadWaitTime;
               s->stats[IOSTAT_READ_WAIT_TIME] = (diffReads > 0) ? (diffReadWaitTime / diffReads) : 0;

               unsigned long diffWrites = writes - d->lastWrites;
               unsigned long diffWriteWaitTime = writeWaitTime - d->lastWriteWaitTime;
               s->stats[IOSTAT_WRITE_WAIT_TIME] = (diffWrites > 0) ? (diffWriteWaitTime / diffWrites) : 0;

               unsigned long diffOps = diffReads + diffWrites;
               s->stats[IOSTAT_WAIT_TIME] = (diffOps > 0) ? ((diffReadWaitTime + diffWriteWaitTime) / diffOps) : 0;
            }
            d->lastReads = reads;
            d->lastWrites = writes;
            d->lastReadWaitTime = readWaitTime;
            d->lastWriteWaitTime = writeWaitTime;
         }
      }
      fclose(f);
   }
}

/**
 * Parse single stat line
 */
static IOSTAT_DEVICE *ParseIoStat(char *line)
{
   char *p = line;

   // Find device name
   while(isspace(*p) || isdigit(*p))
      p++;

   // Read device name
   char devName[64];
   int i;
   for(i = 0; !isspace(*p) && (i < 63); i++, p++)
      devName[i] = *p;
   devName[i] = 0;

   // Find device entry
   int dev;
   for(dev = 0; dev < s_deviceCount; dev++)
      if (!strcmp(devName, s_devices[dev].name))
         break;
   if (dev == s_deviceCount)
   {
      // Add new device
      s_deviceCount++;
      s_devices = MemReallocArray(s_devices, s_deviceCount);
      strcpy(s_devices[dev].name, devName);
      strcpy(s_devices[dev].sysfsName, devName);

	   // Replace / by ! in device name for sysfs
	   for(int i = 0; s_devices[dev].sysfsName[i] != 0; i++)
	      if (s_devices[dev].sysfsName[i] == '/')
	         s_devices[dev].sysfsName[i] = '!';

	   s_devices[dev].isRealDevice = IsRealDevice(s_devices[dev].sysfsName);
	   s_devices[dev].isFirstRead = true;
	   memset(s_devices[dev].samples, 0, sizeof(IOSTAT_SAMPLE) * SAMPLES_PER_MINUTE);
	   nxlog_debug_tag(DEBUG_TAG, 2, _T("ParseIoStat(): new device added (name=%hs isRealDevice=%d)"), devName, s_devices[dev].isRealDevice);
	}

	// Parse counters
   IOSTAT_DEVICE *d = &s_devices[dev];
   IOSTAT_SAMPLE *s = &d->samples[s_currSample];
   sscanf(p, "%ld %*d %ld %*d %ld %*d %ld %*d %ld %ld %*d", &s->stats[IOSTAT_NUM_READS], &s->stats[IOSTAT_NUM_SREADS],
            &s->stats[IOSTAT_NUM_WRITES], &s->stats[IOSTAT_NUM_SWRITES], &s->stats[IOSTAT_DISK_QUEUE], &s->stats[IOSTAT_IO_TIME]);
   return d;
}

/**
 * Collect I/O stats
 */
static void Collect()
{
   s_dataAccessLock.lock();

   FILE *f = fopen(s_statFile, "r");
   if (f != nullptr)
   {
      char line[1024];
		while(true)
		{
			if (fgets(line, 1024, f) == nullptr)
				break;

			IOSTAT_DEVICE *d = ParseIoStat(line);
			if (s_isSysFsAvailable && d->isRealDevice)
			{
			   ReadSysFsStat(d);
			}
		}
		fclose(f);
	}

	s_currSample++;
	if (s_currSample == SAMPLES_PER_MINUTE)
		s_currSample = 0;

	s_dataAccessLock.unlock();
}

/**
 * Collection thread
 */
static void IoCollectionThread()
{
	// Get first sample for each device and fill all samples
	// with same data
	Collect();
	s_dataAccessLock.lock();
	for(int i = 0; i < s_deviceCount; i++)
	{
		for(int j = 1; j < SAMPLES_PER_MINUTE; j++)
			memcpy(&s_devices[i].samples[j], &s_devices[i].samples[0], sizeof(IOSTAT_SAMPLE));
	}
	s_dataAccessLock.unlock();

	while(true)
	{
		if (s_stopCondition.wait(60000 / SAMPLES_PER_MINUTE))
			break;	// Stop condition set
		Collect();
	}
}

/**
 * Start collector
 */
void StartIoStatCollector()
{
	// Check if /sys/block is available
	struct stat st;	
	if (stat("/sys/block", &st) == 0)
	{
		if (S_ISDIR(st.st_mode))
		{
			s_isSysFsAvailable = true;
			nxlog_debug_tag(DEBUG_TAG, 2, _T("Using /sys/block to distinguish devices from partitions"));
		}
	}	

	s_collectorThread = ThreadCreateEx(IoCollectionThread);
}

/**
 * Stop collector
 */
void ShutdownIoStatCollector()
{
	s_stopCondition.set();
	ThreadJoin(s_collectorThread);
}

/**
 * Get samples for given device
 */
static IOSTAT_SAMPLE *GetSamples(const TCHAR *param)
{
	char buffer[64];
	if (!AgentGetParameterArgA(param, 1, buffer, 64))
		return nullptr;

	char *devName = !strncmp(buffer, "/dev/", 5) ? &buffer[5] : buffer;

	for(int i = 0; i < s_deviceCount; i++)
		if (!strcmp(devName, s_devices[i].name))
			return s_devices[i].samples;

	return nullptr;
}

/**
 * Get difference between oldest and newest samples
 */
static unsigned long GetSampleDelta(IOSTAT_SAMPLE *samples, int metric)
{
	// m_currSample points to next sample, so it's oldest one
	int last = s_currSample - 1;
	if (last < 0)
		last = SAMPLES_PER_MINUTE - 1;
	return samples[last].stats[metric] - samples[s_currSample].stats[metric];
}

/**
 * Get total for all samples
 */
static unsigned long GetSampleTotal(IOSTAT_SAMPLE *samples, int metric)
{
   unsigned long sum = 0;
   for(int i = 0; i < SAMPLES_PER_MINUTE; i++)
      sum += samples[i].stats[metric];
   return sum;
}

/**
 * Handler for I/O stat parameters (per device) for cumulative counters
 */
LONG H_IoStatsCumulative(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_UNSUPPORTED;

	s_dataAccessLock.lock();

	IOSTAT_SAMPLE *s = GetSamples(param);
	if (s != nullptr)
	{
		int metric = CAST_FROM_POINTER(arg, int);
		unsigned long delta = GetSampleDelta(s, metric);
		switch(metric)
		{
			case IOSTAT_NUM_SREADS:
			case IOSTAT_NUM_SWRITES:
				delta *= 512;	// Convert sectors to bytes
				ret_uint(value, static_cast<uint32_t>(delta / SAMPLES_PER_MINUTE));
				break;
			case IOSTAT_IO_TIME:   // Milliseconds spent on I/O
				ret_double(value, (double)delta / 600);	// = / 60000 * 100
				break;
			default:
				ret_double(value, (double)delta / SAMPLES_PER_MINUTE);
				break;
		}
		nRet = SYSINFO_RC_SUCCESS;
	}

	s_dataAccessLock.unlock();
	return nRet;
}

/**
 * Handler for I/O stat parameters (per device) for non-cumulative counters
 */
LONG H_IoStatsNonCumulative(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int nRet = SYSINFO_RC_UNSUPPORTED;

   s_dataAccessLock.lock();

   IOSTAT_SAMPLE *s = GetSamples(param);
   if (s != nullptr)
   {
      int metric = CAST_FROM_POINTER(arg, int);
      unsigned long delta = GetSampleTotal(s, metric);
      switch(metric)
      {
         case IOSTAT_READ_WAIT_TIME:
         case IOSTAT_WRITE_WAIT_TIME:
         case IOSTAT_WAIT_TIME:
            ret_uint(value, static_cast<uint32_t>(delta / SAMPLES_PER_MINUTE));
            break;
         case IOSTAT_DISK_QUEUE:
            ret_double(value, static_cast<double>(delta) / SAMPLES_PER_MINUTE);
            break;
      }
      nRet = SYSINFO_RC_SUCCESS;
   }

   s_dataAccessLock.unlock();
   return nRet;
}

/**
 * Handler for I/O stat parameters (total) that needs conversion from number of sectors to bytes
 */
LONG H_IoStatsTotalSectors(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int metric = CAST_FROM_POINTER(arg, int);

   s_dataAccessLock.lock();

   uint64_t sum = 0;
   for(int i = 0; i < s_deviceCount; i++)
   {
      if (!s_devices[i].isRealDevice)
         continue;

      unsigned long delta = GetSampleDelta(s_devices[i].samples, metric);
      delta *= 512;	// Convert sectors to bytes
      sum += static_cast<uint64_t>(delta / 60);
   }

   s_dataAccessLock.unlock();

   ret_uint64(value, sum);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for I/O stat parameters (total) with floating point value
 */
LONG H_IoStatsTotalFloat(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int metric = CAST_FROM_POINTER(arg, int);

   s_dataAccessLock.lock();

   double sum = 0;
   for(int i = 0; i < s_deviceCount; i++)
   {
      if (s_devices[i].isRealDevice)
         sum += GetSampleDelta(s_devices[i].samples, metric) / 60;
   }

   s_dataAccessLock.unlock();

   ret_double(value, sum);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for I/O stat parameters (total) expressed in percentage of time
 */
LONG H_IoStatsTotalTimePct(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int metric = CAST_FROM_POINTER(arg, int);

   s_dataAccessLock.lock();

   double sum = 0;
   for(int i = 0; i < s_deviceCount; i++)
   {
      if (s_devices[i].isRealDevice)
         sum += static_cast<double>(GetSampleDelta(s_devices[i].samples, metric)) / 600; // = / 60000 * 100
   }

   s_dataAccessLock.unlock();

   ret_double(value, sum);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for non-cumulative I/O stat parameters (total) with floating point value
 */
LONG H_IoStatsTotalNonCumulativeFloat(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int metric = CAST_FROM_POINTER(arg, int);

   s_dataAccessLock.lock();

   unsigned long sum = 0;
   for(int i = 0; i < s_deviceCount; i++)
   {
      if (s_devices[i].isRealDevice)
      {
         for(int j = 0; j < SAMPLES_PER_MINUTE; j++)
            sum += s_devices[i].samples[j].stats[metric];
      }
   }
   s_dataAccessLock.unlock();

   ret_double(value, static_cast<double>(sum) / SAMPLES_PER_MINUTE);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for non-cumulative I/O stat parameters (total) with integer value
 */
LONG H_IoStatsTotalNonCumulativeInteger(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int metric = CAST_FROM_POINTER(arg, int);

   s_dataAccessLock.lock();

   unsigned long sum = 0;
   for(int i = 0; i < s_deviceCount; i++)
   {
      if (s_devices[i].isRealDevice)
      {
         for(int j = 0; j < SAMPLES_PER_MINUTE; j++)
            sum += s_devices[i].samples[j].stats[metric];
      }
   }
   s_dataAccessLock.unlock();

   ret_uint64(value, static_cast<uint64_t>(sum) / SAMPLES_PER_MINUTE);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.IO.Devices list
 */
LONG H_IoDevices(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_deviceCount; i++)
   {
      if (s_devices[i].isRealDevice)
      {
         value->addMBString(s_devices[i].name);
      }
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.IO.LogicalDevices list
 */
LONG H_IoLogicalDevices(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_deviceCount; i++)
   {
      if (!s_devices[i].isRealDevice) 
      {
         value->addMBString(s_devices[i].name);
      }
   }
   return SYSINFO_RC_SUCCESS;
}
