/*
** NetXMS DS18x20 sensor subagent
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
** File: ds18x20.cpp
**
**/

#include "ds18x20.h"

/**
 * Static data
 */
static StringMap s_sensorNames;

/**
 * Hanlder for Sensor.Temperature(*)
 */
static LONG H_SensorTemperature(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	TCHAR sensor[64];
	if (!AgentGetParameterArg(param, 1, sensor, 64))
		return SYSINFO_RC_UNSUPPORTED;

	const TCHAR *deviceName = s_sensorNames.get(sensor);
	if (deviceName == NULL)
		deviceName = sensor;

	TCHAR path[MAX_PATH];
	_sntprintf(path, MAX_PATH, _T("/sys/bus/w1/devices/%s/w1_slave"), deviceName);
	FILE *f = _tfopen(path, _T("r"));
	if (f == NULL)
		return SYSINFO_RC_UNSUPPORTED;

	LONG rc = SYSINFO_RC_ERROR;
	char buffer[64];
	if (fgets(buffer, 64, f) != NULL)
	{
		if (fgets(buffer, 64, f) != NULL)
		{
			char *t = strchr(buffer, 't');
			if ((t != NULL) && (*(t + 1) == '='))
			{
				_sntprintf(value, MAX_RESULT_LENGTH, _T("%0.3f"), (double)strtol(t + 2, NULL, 10) / 1000.0);
				rc = SYSINFO_RC_SUCCESS;
			}
		}
	}
	fclose(f);

	return rc;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
}

/**
 * Add sensor name mapping
 * Should be in form alias:device
 */
static bool AddSensorNameMapping(TCHAR *str)
{
	TCHAR *ptr = _tcschr(str, _T(':'));
	if (ptr == NULL)
		return false;

	*ptr = 0;
	ptr++;
	StrStrip(str);
	StrStrip(ptr);
	s_sensorNames.set(str, ptr);
	return true;
}

/**
 * Configuration file template
 */
static TCHAR *s_sensorList = NULL;
static NX_CFG_TEMPLATE s_cfgTemplate[] =
{
	{ _T("Sensor"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &s_sensorList },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
	// Parse configuration
	bool success = config->parseTemplate(_T("DS18X20"), s_cfgTemplate);
	if (success)
	{
		// Parse sensor list
		if (s_sensorList != NULL)
		{
			TCHAR *pItem, *pEnd;

			for(pItem = pEnd = s_sensorList; pEnd != NULL; pItem = pEnd + 1)
			{
				pEnd = _tcschr(pItem, _T('\n'));
				if (pEnd != NULL)
					*pEnd = 0;
				StrStrip(pItem);
				if (!AddSensorNameMapping(pItem))
				{
					AgentWriteLog(EVENTLOG_WARNING_TYPE,
							_T("Unable to add sensor from configuration file. ")
							_T("Original configuration record: %s"), pItem);
				}
			}
			free(s_sensorList);
		}
	}
	else
	{
		safe_free(s_sensorList);
	}

	return success;
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Sensor.Temperature(*)"), H_SensorTemperature, NULL, DCI_DT_FLOAT, _T("Temperature reported by sensor {instance}") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("DS18X20"), NETXMS_VERSION_STRING,
	SubagentInit, SubagentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL, // lists
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(DS18X20)
{
	*ppInfo = &s_info;
	return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
