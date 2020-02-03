/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2006-2013 Victor Kirhenshtein
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
#include <regex.h>

/**
 * Constants
 */
#define MAX_DEVICE_COUNT        64
#define STATUS_FIELD_LEN        64

/**
 * DRBD device structure
 */
typedef struct
{
	int id;
	int protocol;
	char connState[STATUS_FIELD_LEN];
	char localDeviceState[STATUS_FIELD_LEN];
	char remoteDeviceState[STATUS_FIELD_LEN];
	char localDataState[STATUS_FIELD_LEN];
	char remoteDataState[STATUS_FIELD_LEN];
} DRBD_DEVICE;

/**
 * Static data
 */
static DRBD_DEVICE s_devices[MAX_DEVICE_COUNT];
static MUTEX s_deviceAccess = INVALID_MUTEX_HANDLE;
static MUTEX s_versionAccess = INVALID_MUTEX_HANDLE;
static char s_drbdVersion[32] = "0.0.0";
static int s_apiVersion = 0;
static char s_protocolVersion[32] = "0";
static CONDITION s_stopCondition = INVALID_CONDITION_HANDLE;
static THREAD s_collectorThread = INVALID_THREAD_HANDLE;

/**
 * Read and parse /proc/drbd file
 */
static bool ParseDrbdStatus()
{
	char line[1024];
	FILE *fp;
	regex_t pregDevice, pregVersion;
	regmatch_t pmatch[9];
	DRBD_DEVICE device;
	bool rc = false;

	if (regcomp(&pregVersion, "version: (.*) \\(api\\:([0-9]+)\\/proto\\:([0-9\\-]+)\\)", REG_EXTENDED) != 0)
		return false;
	if (regcomp(&pregDevice, "^[[:space:]]*([0-9]+)\\: cs\\:(.*) (st|ro)\\:(.*)\\/(.*) ds\\:(.*)\\/([^[:space:]]*) ([^[:space:]]*) .*", REG_EXTENDED) != 0)
	{
		regfree(&pregVersion);
		return false;
	}

	fp = fopen("/proc/drbd", "r");
	if (fp != NULL)
	{
		MutexLock(s_deviceAccess);
		for(int i = 0; i < MAX_DEVICE_COUNT; i++)
			s_devices[i].id = -1;

		while(!feof(fp))
		{
			if (fgets(line, 1024, fp) == NULL)
				break;
			if (regexec(&pregDevice, line, 9, pmatch, 0) == 0)
			{
				for(int i = 1; i < 9; i++)
					line[pmatch[i].rm_eo] = 0;

				memset(&device, 0, sizeof(DRBD_DEVICE));
				device.id = strtol(&line[pmatch[1].rm_so], NULL, 10);
				device.protocol = line[pmatch[8].rm_so];
				strlcpy(device.connState, &line[pmatch[2].rm_so], STATUS_FIELD_LEN);
				strlcpy(device.localDeviceState, &line[pmatch[4].rm_so], STATUS_FIELD_LEN);
				strlcpy(device.remoteDeviceState, &line[pmatch[5].rm_so], STATUS_FIELD_LEN);
				strlcpy(device.localDataState, &line[pmatch[6].rm_so], STATUS_FIELD_LEN);
				strlcpy(device.remoteDataState, &line[pmatch[7].rm_so], STATUS_FIELD_LEN);

				if ((device.id >= 0) && (device.id < MAX_DEVICE_COUNT))
				{
					memcpy(&s_devices[device.id], &device, sizeof(DRBD_DEVICE));
				}
			}
			else if (regexec(&pregVersion, line, 9, pmatch, 0) == 0)
			{
				for(int i = 1; i < 4; i++)
					line[pmatch[i].rm_eo] = 0;

				MutexLock(s_versionAccess);
				strlcpy(s_drbdVersion, &line[pmatch[1].rm_so], 32);
				s_apiVersion = strtol(&line[pmatch[2].rm_so], NULL, 10);
				strlcpy(s_protocolVersion, &line[pmatch[3].rm_so], 32);
				MutexUnlock(s_versionAccess);
			}
		}
		MutexUnlock(s_deviceAccess);
		fclose(fp);
		rc = true;
	}
	else
	{
		MutexLock(s_deviceAccess);
		for(int i = 0; i < MAX_DEVICE_COUNT; i++)
			s_devices[i].id = -1;
		MutexUnlock(s_deviceAccess);
	}

	regfree(&pregVersion);
	regfree(&pregDevice);
	return rc;
}

/**
 * DRBD stat collector thread
 */
static THREAD_RESULT THREAD_CALL CollectorThread(void *arg)
{
	if (!ParseDrbdStatus())
	{
		AgentWriteDebugLog(1, _T("Unable to parse /proc/drbd, DRBD data collector will not start"));
		return THREAD_OK;
	}

	while(!ConditionWait(s_stopCondition, 15000))
		ParseDrbdStatus();
	return THREAD_OK;
}

/**
 * Initialize DRBD stat collector
 */
void InitDrbdCollector()
{
	s_deviceAccess = MutexCreate();
	s_versionAccess = MutexCreate();
	s_stopCondition = ConditionCreate(TRUE);
	s_collectorThread = ThreadCreateEx(CollectorThread, 0, NULL);
}

/**
 * Stop DRBD stat collector
 */
void StopDrbdCollector()
{
	ConditionSet(s_stopCondition);
	ThreadJoin(s_collectorThread);
	ConditionDestroy(s_stopCondition);
	MutexDestroy(s_deviceAccess);
	MutexDestroy(s_versionAccess);
}

/**
 * Get list of configured DRBD devices
 */
LONG H_DRBDDeviceList(const TCHAR *pszCmd, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
{
	TCHAR szBuffer[1024];

	MutexLock(s_deviceAccess);
	for(int i = 0; i < MAX_DEVICE_COUNT; i++)
	{
		if (s_devices[i].id != -1)
		{
                        _sntprintf(szBuffer, 1024, _T("/dev/drbd%d %hs %hs/%hs %hs/%hs %c"),
			           i, s_devices[i].connState, s_devices[i].localDeviceState,
			           s_devices[i].remoteDeviceState, s_devices[i].localDataState,
			           s_devices[i].remoteDataState, (TCHAR)s_devices[i].protocol);
			pValue->add(szBuffer);
		}
	}
	MutexUnlock(s_deviceAccess);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Get DRBD version
 */
LONG H_DRBDVersion(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	switch(*pArg)
	{
		case 'v':	// DRBD version
			ret_mbstring(pValue, s_drbdVersion);
			break;
		case 'a':	// API version
			ret_int(pValue, s_apiVersion);
			break;
		case 'p':	// Protocol version
			ret_mbstring(pValue, s_protocolVersion);
			break;
		default:
			nRet = SYSINFO_RC_UNSUPPORTED;
			break;
	}
	return nRet;
}

/**
 * Get information for specific DRBD device
 */
LONG H_DRBDDeviceInfo(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	TCHAR szDev[256], *eptr;

	if (!AgentGetParameterArg(pszCmd, 1, szDev, 256))
		return SYSINFO_RC_UNSUPPORTED;

        int nDev = _tcstol(szDev, &eptr, 0);
        if ((nDev < 0) || (nDev >= MAX_DEVICE_COUNT) || (*eptr != 0))
		return SYSINFO_RC_UNSUPPORTED;

        LONG nRet = SYSINFO_RC_ERROR;
	MutexLock(s_deviceAccess);
	if (s_devices[nDev].id != -1)
	{
		nRet = SYSINFO_RC_SUCCESS;
		switch(*pArg)
		{
			case 'c':	// Connection state as text
				ret_mbstring(pValue, s_devices[nDev].connState);
				break;
			case 's':	// State as text
				ret_mbstring(pValue, s_devices[nDev].localDeviceState);
				break;
			case 'S':	// Peer state as text
				ret_mbstring(pValue, s_devices[nDev].remoteDeviceState);
				break;
			case 'd':	// Data state as text
				ret_mbstring(pValue, s_devices[nDev].localDataState);
				break;
			case 'D':	// Peer data state as text
				ret_mbstring(pValue, s_devices[nDev].remoteDataState);
				break;
			case 'p':	// Protocol
				pValue[0] = s_devices[nDev].protocol;
				pValue[1] = 0;
				break;
			default:
				nRet = SYSINFO_RC_UNSUPPORTED;
				break;
		}
	}
	MutexUnlock(s_deviceAccess);

	return nRet;
}
