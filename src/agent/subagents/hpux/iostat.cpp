/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
** Copyright (C) 2010 Victor Kirhenshtein
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
*/

#include "hpux.h"
#include <sys/pstat.h>

#define MAX_DEVICES		256
#define SAMPLE_COUNT    60

#define PSTTIME_TO_MS(x) (((QWORD)x.pst_sec * 1000) + ((QWORD)x.pst_usec / 1000))


//
// Disk information structure
//

struct DISK_INFO
{
	int devMajor;
	int devMinor;
	char hwPath[PS_MAX_HW_ELEMS];
	int queueLen[SAMPLE_COUNT];
	QWORD transfers[SAMPLE_COUNT];
	QWORD reads[SAMPLE_COUNT];
	QWORD writes[SAMPLE_COUNT];
	QWORD bytesRead[SAMPLE_COUNT];
	QWORD bytesWritten[SAMPLE_COUNT];
	QWORD waitTime[SAMPLE_COUNT];
	struct pst_diskinfo last;
};


//
// Static data
//

static DISK_INFO s_total;
static DISK_INFO s_devices[MAX_DEVICES];
static int s_currSlot = 0;
static THREAD s_collectorThread = INVALID_THREAD_HANDLE;
static MUTEX s_dataLock = INVALID_MUTEX_HANDLE;
static CONDITION s_condShutdown = INVALID_CONDITION_HANDLE;


//
// Calculate total values
//

static void CalculateTotals()
{
	s_total.queueLen[s_currSlot] = 0;
	s_total.transfers[s_currSlot] = 0;
	s_total.reads[s_currSlot] = 0;
	s_total.writes[s_currSlot] = 0;
	s_total.bytesRead[s_currSlot] = 0;
	s_total.bytesWritten[s_currSlot] = 0;
	s_total.waitTime[s_currSlot] = 0;

	for(int i = 0; i < MAX_DEVICES; i++)
	{
		if ((s_devices[i].devMajor == 0) && (s_devices[i].devMinor == 0))
			break;

		s_total.queueLen[s_currSlot] += s_devices[i].queueLen[s_currSlot];
		s_total.transfers[s_currSlot] += s_devices[i].transfers[s_currSlot];
		s_total.reads[s_currSlot] += s_devices[i].reads[s_currSlot];
		s_total.writes[s_currSlot] += s_devices[i].writes[s_currSlot];
		s_total.bytesRead[s_currSlot] += s_devices[i].bytesRead[s_currSlot];
		s_total.bytesWritten[s_currSlot] += s_devices[i].bytesWritten[s_currSlot];
		s_total.waitTime[s_currSlot] += s_devices[i].waitTime[s_currSlot];
	}
}


//
// Process stats for single disk
//

static void ProcessDiskStats(struct pst_diskinfo *di)
{
	DISK_INFO *dev;
	int i;

	for(i = 0; i < MAX_DEVICES; i++)
	{
		if ((di->psd_dev.psd_major == s_devices[i].devMajor) &&
		    (di->psd_dev.psd_minor == s_devices[i].devMinor))
		{
			dev = &s_devices[i];
			break;
		}
		if ((s_devices[i].devMajor == 0) && (s_devices[i].devMinor == 0))
		{
			// End of device list, take new slot
			dev = &s_devices[i];
			dev->devMajor = di->psd_dev.psd_major;
			dev->devMinor = di->psd_dev.psd_minor;
			strcpy(dev->hwPath, di->psd_hw_path.psh_name);
			memcpy(&dev->last, di, sizeof(struct pst_diskinfo));
			AgentWriteDebugLog(4, "HPUX: device %d.%d hwpath %s added to I/O stat collection",
			                   dev->devMajor, dev->devMinor, dev->hwPath);
			return;  // No prev data for new device, ignore this sample
		}
	}

	if (i == MAX_DEVICES)
		return;	// New device and no more free slots

	dev->queueLen[s_currSlot] = di->psd_dkqlen_curr;
	dev->transfers[s_currSlot] = di->psd_dkxfer - dev->last.psd_dkxfer;
#if HAVE_DISKINFO_RWSTATS
	dev->reads[s_currSlot] = di->psd_dkread - dev->last.psd_dkread;
	dev->writes[s_currSlot] = di->psd_dkwrite - dev->last.psd_dkwrite;
	dev->bytesRead[s_currSlot] = di->psd_dkbyteread - dev->last.psd_dkbyteread;
	dev->bytesWritten[s_currSlot] = di->psd_dkbytewrite - dev->last.psd_dkbytewrite;
#else
	dev->reads[s_currSlot] = 0;
	dev->writes[s_currSlot] = 0;
	dev->bytesRead[s_currSlot] = 0;
	dev->bytesWritten[s_currSlot] = 0;
#endif
	dev->waitTime[s_currSlot] = PSTTIME_TO_MS(di->psd_dkwait) - PSTTIME_TO_MS(dev->last.psd_dkwait);

	memcpy(&dev->last, di, sizeof(struct pst_diskinfo));
}


//
// I/O stat collector
//

static THREAD_RESULT THREAD_CALL IOStatCollector(void *arg)
{
	struct pst_diskinfo di[64];
	int index, count;

	AgentWriteDebugLog(1, "HPUX: I/O stat collector thread started");

	while(!ConditionWait(s_condShutdown, 1000))
	{
		MutexLock(s_dataLock);
		for(index = 0, count = 64; count == 64; index += 64)
		{
			count = pstat_getdisk(di, sizeof(struct pst_diskinfo), 64, index);
			for(int i = 0; i < count; i++)
			{
				ProcessDiskStats(&di[i]);
			}
		}
		CalculateTotals();
		s_currSlot++;
		if (s_currSlot == SAMPLE_COUNT)
			s_currSlot = 0;
		MutexUnlock(s_dataLock);
	}

	AgentWriteDebugLog(1, "HPUX: I/O stat collector thread stopped");
	return THREAD_OK;
}


//
// Initialize I/O stat collector
//

void StartIOStatCollector()
{
	memset(&s_total, 0, sizeof(s_total));
	memset(s_devices, 0, sizeof(s_devices));
	s_dataLock = MutexCreate();
	s_condShutdown = ConditionCreate(TRUE);
	s_collectorThread = ThreadCreateEx(IOStatCollector, 0, NULL);
}


//
// Shutdown I/O stat collector
//

void ShutdownIOStatCollector()
{
	ConditionSet(s_condShutdown);
	ThreadJoin(s_collectorThread);
	MutexDestroy(s_dataLock);
	ConditionDestroy(s_condShutdown);
}


//
// Calculate average value for 32bit series
//

static double CalculateAverage32(int *series)
{
	double sum = 0;
	for(int i = 0; i < SAMPLE_COUNT; i++)
		sum += series[i];
	return sum / (double)SAMPLE_COUNT;
}


//
// Calculate average value for 64bit series
//

static QWORD CalculateAverage64(QWORD *series)
{
	QWORD sum = 0;
	for(int i = 0; i < SAMPLE_COUNT; i++)
		sum += series[i];
	return sum / SAMPLE_COUNT;
}


//
// Calculate average wait time
//

static DWORD CalculateAverageTime(QWORD *opcount, QWORD *times)
{
	QWORD totalOps = 0;
	QWORD totalTime = 0;

	for(int i = 0; i < SAMPLE_COUNT; i++)
	{
		totalOps += opcount[i];
		totalTime += times[i];
	}
	return (DWORD)(totalTime / totalOps);
}


//
// Get total I/O stat value
//

LONG H_IOStatsTotal(const char *cmd, const char *arg, char *value, AbstractCommSession *session)
{
	LONG rc = SYSINFO_RC_SUCCESS;

	MutexLock(s_dataLock);
	switch(CAST_FROM_POINTER(arg, int))
	{
		case IOSTAT_NUM_READS:
			ret_double(value, CalculateAverage64(s_total.reads));
			break;
		case IOSTAT_NUM_WRITES:
			ret_double(value, CalculateAverage64(s_total.writes));
			break;
		case IOSTAT_NUM_XFERS:
			ret_double(value, CalculateAverage64(s_total.transfers));
			break;
		case IOSTAT_QUEUE:
			ret_double(value, CalculateAverage32(s_total.queueLen));
			break;
		case IOSTAT_NUM_RBYTES:
			ret_uint64(value, CalculateAverage64(s_total.bytesRead));
			break;
		case IOSTAT_NUM_WBYTES:
			ret_uint64(value, CalculateAverage64(s_total.bytesWritten));
			break;
		case IOSTAT_WAIT_TIME:
			ret_uint(value, CalculateAverageTime(s_total.transfers, s_total.waitTime));
			break;
		default:
			rc = SYSINFO_RC_UNSUPPORTED;
			break;
	}
	MutexUnlock(s_dataLock);
	return rc;
}


//
// Get I/O stat for specific device
//

LONG H_IOStats(const char *cmd, const char *arg, char *value, AbstractCommSession *session)
{
	char device[MAX_PATH];
	struct stat devInfo;
	LONG rc = SYSINFO_RC_SUCCESS;
	int i;

	if (!AgentGetParameterArg(cmd, 1, device, MAX_PATH))
		return SYSINFO_RC_UNSUPPORTED;

	if (stat(device, &devInfo) != 0)
		return (errno == EIO) ? SYSINFO_RC_ERROR : SYSINFO_RC_UNSUPPORTED;
	if (!S_ISBLK(devInfo.st_mode))
		return SYSINFO_RC_UNSUPPORTED;
	int devMajor = major(devInfo.st_rdev);
	int devMinor = minor(devInfo.st_rdev);

	// find device
	for(i = 0; (i < MAX_DEVICES) && (makedev(s_devices[i].devMajor, s_devices[i].devMinor) != 0); i++)
		if ((s_devices[i].devMajor == devMajor) && (s_devices[i].devMinor == devMinor))
			break;
	if ((i == MAX_DEVICES) || ((s_devices[i].devMajor == 0) && (s_devices[i].devMinor == 0)))
		return SYSINFO_RC_UNSUPPORTED;	// unknown device

	MutexLock(s_dataLock);
	switch(CAST_FROM_POINTER(arg, int))
	{
		case IOSTAT_NUM_READS:
			ret_double(value, CalculateAverage64(s_devices[i].reads));
			break;
		case IOSTAT_NUM_WRITES:
			ret_double(value, CalculateAverage64(s_devices[i].writes));
			break;
		case IOSTAT_NUM_XFERS:
			ret_double(value, CalculateAverage64(s_devices[i].transfers));
			break;
		case IOSTAT_QUEUE:
			ret_double(value, CalculateAverage32(s_devices[i].queueLen));
			break;
		case IOSTAT_NUM_RBYTES:
			ret_uint64(value, CalculateAverage64(s_devices[i].bytesRead));
			break;
		case IOSTAT_NUM_WBYTES:
			ret_uint64(value, CalculateAverage64(s_devices[i].bytesWritten));
			break;
		case IOSTAT_WAIT_TIME:
			ret_uint(value, CalculateAverageTime(s_devices[i].transfers, s_devices[i].waitTime));
			break;
		default:
			rc = SYSINFO_RC_UNSUPPORTED;
			break;
	}
	MutexUnlock(s_dataLock);
	return rc;
}

