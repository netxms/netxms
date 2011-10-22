/* 
** NetXMS subagent for AIX
** Copyright (C) 2006 Alex Kirhenshtein
** Copyright (C) 2011 Victor Kirhenshtein
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

#include "aix_subagent.h"

#define MAX_DEVICES		256
#define SAMPLE_COUNT    60


//
// Disk information structure
//

struct DISK_INFO
{
	char name[IDENTIFIER_LENGTH];
	int queueLen[SAMPLE_COUNT];
	QWORD transfers[SAMPLE_COUNT];
	QWORD reads[SAMPLE_COUNT];
	QWORD writes[SAMPLE_COUNT];
	QWORD bytesRead[SAMPLE_COUNT];
	QWORD bytesWritten[SAMPLE_COUNT];
	QWORD waitTime[SAMPLE_COUNT];
	perfstat_disk_t last;
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
		if (s_devices[i].name[0] == 0)
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

static void ProcessDiskStats(perfstat_disk_t *di)
{
	DISK_INFO *dev;
	int i;

	for(i = 0; i < MAX_DEVICES; i++)
	{
		if (!strcmp(di->name, s_devices[i].name))
		{
			dev = &s_devices[i];
			break;
		}
		if (s_devices[i].name[0] == 0)
		{
			// End of device list, take new slot
			dev = &s_devices[i];
			strcpy(dev->name, di->name);
			memcpy(&dev->last, di, sizeof(perfstat_disk_t));
			AgentWriteDebugLog(4, "AIX: device %s on adapter %s added to I/O stat collection", dev->name, di->adapter);
			return;  // No prev data for new device, ignore this sample
		}
	}

	if (i == MAX_DEVICES)
		return;	// New device and no more free slots

	dev->queueLen[s_currSlot] = di->qdepth;
	dev->transfers[s_currSlot] = di->xfers - dev->last.xfers;
	dev->reads[s_currSlot] = di->__rxfers - dev->last.__rxfers;
	dev->writes[s_currSlot] = (di->xfers - di->__rxfers) - (dev->last.xfers - dev->last.__rxfers);
	dev->bytesRead[s_currSlot] = (di->rblks - dev->last.rblks) * di->bsize;
	dev->bytesWritten[s_currSlot] = (di->wblks - dev->last.wblks) * di->bsize;
	dev->waitTime[s_currSlot] = (di->wq_time - dev->last.wq_time) / 1000;
	// NOTE: I'm not 100% sure that wq_time is measured in microseconds,
	// but it looks like that and I was unable to find any additional info

	memcpy(&dev->last, di, sizeof(perfstat_disk_t));
}


//
// I/O stat collector
//

static THREAD_RESULT THREAD_CALL IOStatCollector(void *arg)
{
	AgentWriteDebugLog(1, "AIX: I/O stat collector thread started");

	while(!ConditionWait(s_condShutdown, 1000))
	{
		MutexLock(s_dataLock, INFINITE);

		perfstat_id_t firstdisk;
		strcpy(firstdisk.name, FIRST_DISK);

		int diskCount = perfstat_disk(NULL, NULL, sizeof(perfstat_disk_t), 0);
		if (diskCount > 0)
		{
			perfstat_disk_t *data = (perfstat_disk_t *)malloc(sizeof(perfstat_disk_t) * diskCount);
			if (perfstat_disk(&firstdisk, data, sizeof(perfstat_disk_t), diskCount) == diskCount)
			{
				for(int i = 0; i < diskCount; i++)
					ProcessDiskStats(&data[i]);
			}
			free(data);
		}
		CalculateTotals();
		s_currSlot++;
		if (s_currSlot == SAMPLE_COUNT)
			s_currSlot = 0;
		MutexUnlock(s_dataLock);
	}

	AgentWriteDebugLog(1, "AIX: I/O stat collector thread stopped");
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

LONG H_IOStatsTotal(const char *cmd, const char *arg, char *value)
{
	LONG rc = SYSINFO_RC_SUCCESS;

	MutexLock(s_dataLock, INFINITE);
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

LONG H_IOStats(const char *cmd, const char *arg, char *value)
{
	char device[MAX_PATH];
	struct stat devInfo;
	LONG rc = SYSINFO_RC_SUCCESS;
	int i;

	if (!AgentGetParameterArg(cmd, 1, device, MAX_PATH))
		return SYSINFO_RC_UNSUPPORTED;

	// find device
	for(i = 0; (i < MAX_DEVICES) && (s_devices[i].name[0] != 0); i++)
		if (!strcmp(device, s_devices[i].name))
			break;
	if ((i == MAX_DEVICES) || (s_devices[i].name[0] == 0))
		return SYSINFO_RC_UNSUPPORTED;	// unknown device

	MutexLock(s_dataLock, INFINITE);
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

