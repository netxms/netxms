/* $Id$ */
/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004 - 2008 NetXMS Team
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


//
// One sample structure
//

struct IOSTAT_SAMPLE
{
	DWORD ioRequests;
	DWORD stats[5];
};


//
// Device structure
//

struct IOSTAT_DEVICE
{
	char name[64];
	IOSTAT_SAMPLE samples[SAMPLES_PER_MINUTE];
};


//
// Static data
//

static CONDITION m_condStop;
static THREAD m_collectorThread;
static const char *m_statFile = "/proc/diskstats";
static IOSTAT_DEVICE *m_devices = NULL;
static int m_deviceCount = 0;
static int m_currSample = 0;
static MUTEX m_dataAccess;


//
// Parse single stat line
//

static void ParseIoStat(char *line)
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
	for(dev = 0; dev < m_deviceCount; dev++)
		if (!strcmp(devName, m_devices[dev].name))
			break;
	if (dev == m_deviceCount)
	{
		// Add new device
		m_deviceCount++;
		m_devices = (IOSTAT_DEVICE *)realloc(m_devices, m_deviceCount * sizeof(IOSTAT_DEVICE));
		strcpy(m_devices[dev].name, devName);
		memset(m_devices[dev].samples, 0, sizeof(IOSTAT_SAMPLE) * SAMPLES_PER_MINUTE);
	}

	// Parse counters
	IOSTAT_SAMPLE *s = &m_devices[dev].samples[m_currSample];
	sscanf(p, "%d %*d %d %*d %d %*d %d %*d %d %d %*d", &s->stats[IOSTAT_NUM_READS], &s->stats[IOSTAT_NUM_SREADS],
	       &s->stats[IOSTAT_NUM_WRITES], &s->stats[IOSTAT_NUM_SWRITES], &s->ioRequests, &s->stats[IOSTAT_IO_TIME]);
}


//
// Collect I/O stats
//

static void Collect()
{
	MutexLock(m_dataAccess, INFINITE);

	FILE *f = fopen(m_statFile, "r");
	if (f != NULL)
	{
		char line[1024];

		while(1)
		{
			if (fgets(line, 1024, f) == NULL)
				break;

			ParseIoStat(line);
		}
		fclose(f);
	}

	m_currSample++;
	if (m_currSample == SAMPLES_PER_MINUTE)
		m_currSample = 0;

	MutexUnlock(m_dataAccess);
}


//
// Collection thread
//

static THREAD_RESULT THREAD_CALL IoCollectionThread(void *arg)
{
	// Get first sample for each device and fill all samples
	// with same data
	Collect();
	MutexLock(m_dataAccess, INFINITE);
	for(int i = 0; i < m_deviceCount; i++)
	{
		for(int j = 1; j < SAMPLES_PER_MINUTE; j++)
			memcpy(&m_devices[i].samples[j], &m_devices[i].samples[0], sizeof(IOSTAT_SAMPLE));
	}
	MutexUnlock(m_dataAccess);

	while(1)
	{
		if (ConditionWait(m_condStop, 60000 / SAMPLES_PER_MINUTE))
			break;	// Stop condition set
		Collect();
	}
	return THREAD_OK;
}


//
// Start collector
//

void StartIoStatCollector()
{
	m_condStop = ConditionCreate(TRUE);
	m_dataAccess = MutexCreate();
	m_collectorThread = ThreadCreateEx(IoCollectionThread, 0, NULL);
}


//
// Stop collector
//

void ShutdownIoStatCollector()
{
	ConditionSet(m_condStop);
	ThreadJoin(m_collectorThread);
	ConditionDestroy(m_condStop);
	MutexDestroy(m_dataAccess);
}


//
// Get samples for given device
//

static IOSTAT_SAMPLE *GetSamples(const char *param)
{
	char devName[64];

	if (!NxGetParameterArg(param, 1, devName, 64))
		return NULL;

	for(int i = 0; i < m_deviceCount; i++)
		if (!strcmp(devName, m_devices[i].name))
			return m_devices[i].samples;

	return NULL;
}


//
// Get difference between oldest and newest samples
//

static DWORD GetSampleDelta(IOSTAT_SAMPLE *samples, int metric)
{
	// m_currSample points to next sample, so it's oldest one
	int last = m_currSample - 1;
	if (last < 0)
		last = SAMPLES_PER_MINUTE - 1;
	return samples[last].stats[metric] - samples[m_currSample].stats[metric];
}


//
// Handlers for agent parameters
//

LONG H_IoStats(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_UNSUPPORTED;

	MutexLock(m_dataAccess, INFINITE);

	IOSTAT_SAMPLE *s = GetSamples(pszParam);
	if (s != NULL)
	{
		int metric = CAST_FROM_POINTER(pArg, int);
		DWORD delta = GetSampleDelta(s, metric);
		switch(metric)
		{
			case IOSTAT_NUM_SREADS:
			case IOSTAT_NUM_SWRITES:
				delta *= 512;	// Convert sectors to bytes
				ret_uint(pValue, delta / SAMPLES_PER_MINUTE);
				break;
			case IOSTAT_IO_TIME:   // Milliseconds spent on I/O
				ret_double(pValue, (double)delta / 600);	// = / 60000 * 100
				break;
			default:
				ret_double(pValue, (double)delta / SAMPLES_PER_MINUTE);
				break;
		}
		nRet = SYSINFO_RC_SUCCESS;
	}

	MutexUnlock(m_dataAccess);	

	return nRet;
}

LONG H_DiskQueue(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_UNSUPPORTED;

	MutexLock(m_dataAccess, INFINITE);

	IOSTAT_SAMPLE *s = GetSamples(pszParam);
	if (s != NULL)
	{
		DWORD sum = 0;
		for(int i = 0; i < SAMPLES_PER_MINUTE; i++)
			sum += s[i].ioRequests;
		ret_double(pValue, (double)sum / SAMPLES_PER_MINUTE);
		nRet = SYSINFO_RC_SUCCESS;
	}

	MutexUnlock(m_dataAccess);	

	return nRet;
}

LONG H_DiskQueueTotal(const char *pszParam, const char *pArg, char *pValue)
{
	MutexLock(m_dataAccess, INFINITE);

	DWORD sum = 0;
	for(int i = 0; i < m_deviceCount; i++)
	{
		for(int j = 0; j < SAMPLES_PER_MINUTE; j++)
			sum += m_devices[i].samples[j].ioRequests;
	}
	MutexUnlock(m_dataAccess);	

	ret_double(pValue, (double)sum / SAMPLES_PER_MINUTE);
	return SYSINFO_RC_SUCCESS;
}

