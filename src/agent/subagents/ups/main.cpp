/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2014 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "ups.h"

/**
 * Device list
 */
static UPSInterface *m_deviceInfo[MAX_UPS_DEVICES];

/**
 * Universal handler
 */
static LONG H_UPSData(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	LONG nDev;
	TCHAR *pErr, szArg[256];

	if (!AgentGetParameterArg(pszParam, 1, szArg, 256))
		return SYSINFO_RC_UNSUPPORTED;

	nDev = _tcstol(szArg, &pErr, 0);
	if ((*pErr != 0) || (nDev < 0) || (nDev >= MAX_UPS_DEVICES))
		return SYSINFO_RC_UNSUPPORTED;

	if (m_deviceInfo[nDev] == NULL)
		return SYSINFO_RC_UNSUPPORTED;

	if (!m_deviceInfo[nDev]->isConnected())
		return SYSINFO_RC_ERROR;

	return m_deviceInfo[nDev]->getParameter(CAST_FROM_POINTER(pArg, int), pValue);
}

/**
 * UPS connection status
 */
static LONG H_UPSConnStatus(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	LONG nDev;
	TCHAR *pErr, szArg[256];

	if (!AgentGetParameterArg(pszParam, 1, szArg, 256))
		return SYSINFO_RC_UNSUPPORTED;

	nDev = _tcstol(szArg, &pErr, 0);
	if ((*pErr != 0) || (nDev < 0) || (nDev >= MAX_UPS_DEVICES))
		return SYSINFO_RC_UNSUPPORTED;

	if (m_deviceInfo[nDev] == NULL)
		return SYSINFO_RC_UNSUPPORTED;

	ret_int(pValue, m_deviceInfo[nDev]->isConnected() ? 0 : 1);
	return SYSINFO_RC_SUCCESS;
}

/**
 * List configured devices
 */
static LONG H_DeviceList(const TCHAR *pszParam, const TCHAR *pArg, StringList *value)
{
	TCHAR szBuffer[256];
	int i;

	for(i = 0; i < MAX_UPS_DEVICES; i++)
		if (m_deviceInfo[i] != NULL)
		{
			_sntprintf(szBuffer, 256, _T("%d %s %s %s"), i,
					m_deviceInfo[i]->getDevice(), m_deviceInfo[i]->getType(),
					m_deviceInfo[i]->getName());
			value->add(szBuffer);
		}
	return SYSINFO_RC_SUCCESS;
}

/**
 * Add device from configuration file parameter
 * Parameter value should be <device_id>:<port>:<protocol>:<name>
 */
static BOOL AddDeviceFromConfig(const TCHAR *pszStr)
{
	const TCHAR *ptr;
	TCHAR *eptr, *pszCurrField;
	TCHAR szPort[MAX_PATH], szName[MAX_DB_STRING] = _T("");
	int nState, nField, nDev, nPos, nProto;

	// Parse line
	pszCurrField = (TCHAR *)malloc(sizeof(TCHAR) * (_tcslen(pszStr) + 1));
	for(ptr = pszStr, nState = 0, nField = 0, nPos = 0;
			(nState != -1) && (nState != 255); ptr++)
	{
		switch(nState)
		{
			case 0:  // Normal text
				switch(*ptr)
				{
					case '\'':
						nState = 1;
						break;
					case '"':
						nState = 2;
						break;
					case ':':   // New field
					case 0:
						pszCurrField[nPos] = 0;
						switch(nField)
						{
							case 0:  // Device number
								nDev = _tcstol(pszCurrField, &eptr, 0);
								if ((*eptr != 0) || (nDev < 0) || (nDev >= MAX_UPS_DEVICES))
									nState = 255;  // Error
								break;
							case 1:  // Serial port
								nx_strncpy(szPort, pszCurrField, MAX_PATH);
								break;
							case 2:  // Protocol
								if (!_tcsicmp(pszCurrField, _T("APC")))
								{
									nProto = UPS_PROTOCOL_APC;
								}
								else if (!_tcsicmp(pszCurrField, _T("BCMXCP")))
								{
									nProto = UPS_PROTOCOL_BCMXCP;
								}
								else if (!_tcsicmp(pszCurrField, _T("METASYS")))
								{
									nProto = UPS_PROTOCOL_METASYS;
								}
								else if (!_tcsicmp(pszCurrField, _T("MICRODOWELL")))
								{
									nProto = UPS_PROTOCOL_MICRODOWELL;
								}
#ifdef _WIN32
								else if (!_tcsicmp(pszCurrField, _T("USB")))
								{
									nProto = UPS_PROTOCOL_USB;
								}
#endif
								else
								{
									nState = 255;  // Error
								}
								break;
							case 3:  // Name
								nx_strncpy(szName, pszCurrField, MAX_DB_STRING);
								break;
							default:
								nState = 255;  // Error
								break;
						}
						nField++;
						nPos = 0;
						if ((nState != 255) && (*ptr == 0))
							nState = -1;   // Finish
						break;
					default:
						pszCurrField[nPos++] = *ptr;
						break;
				}
				break;
			case 1:  // Single quotes
				switch(*ptr)
				{
					case '\'':
						nState = 0;
						break;
					case 0:  // Unexpected end of line
						nState = 255;
						break;
					default:
						pszCurrField[nPos++] = *ptr;
						break;
				}
				break;
			case 2:  // Double quotes
				switch(*ptr)
				{
					case '"':
						nState = 0;
						break;
					case 0:  // Unexpected end of line
						nState = 255;
						break;
					default:
						pszCurrField[nPos++] = *ptr;
						break;
				}
				break;
			default:
				break;
		}
	}
	free(pszCurrField);

	// Add new device if parsing was successful
	if ((nState == -1) && (nField >= 3))
	{
		if (m_deviceInfo[nDev] != NULL)
			delete m_deviceInfo[nDev];
		switch(nProto)
		{
			case UPS_PROTOCOL_APC:
				m_deviceInfo[nDev] = new APCInterface(szPort);
				break;
			case UPS_PROTOCOL_BCMXCP:
				m_deviceInfo[nDev] = new BCMXCPInterface(szPort);
				break;
         case UPS_PROTOCOL_METASYS:
				m_deviceInfo[nDev] = new MetaSysInterface(szPort);
				break;
			case UPS_PROTOCOL_MICRODOWELL:
				m_deviceInfo[nDev] = new MicrodowellInterface(szPort);
				break;
#ifdef _WIN32
			case UPS_PROTOCOL_USB:
				m_deviceInfo[nDev] = new USBInterface(szPort);
				break;
#endif
			default:
				break;
		}
		m_deviceInfo[nDev]->setName(szName);
		m_deviceInfo[nDev]->setIndex(nDev);
	}

	return ((nState == -1) && (nField >= 3));
}

/**
 * Configuration file template
 */
static TCHAR *m_pszDeviceList = NULL;
static NX_CFG_TEMPLATE cfgTemplate[] =
{
	{ _T("Device"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszDeviceList },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
	int i;
	TCHAR *entry;
	ConfigEntry *devices; 

	memset(m_deviceInfo, 0, sizeof(UPSInterface *) * MAX_UPS_DEVICES);

	// Parse configuration
	devices = config->getEntry(_T("/UPS/Device"));
	if (devices != NULL)
	{
		for(i = 0; i < devices->getValueCount(); i++)
		{
			entry = _tcsdup(devices->getValue(i));
			StrStrip(entry);
			if (!AddDeviceFromConfig(entry))
			{
				AgentWriteLog(EVENTLOG_WARNING_TYPE,
						_T("Unable to add UPS device from configuration file. ")
						_T("Original configuration record: %s"), devices->getValue(i));
			}
			free(entry);
		}
	}

	// Start communicating with configured devices
	for(i = 0; i < MAX_UPS_DEVICES; i++)
	{
		if (m_deviceInfo[i] != NULL)
			m_deviceInfo[i]->startCommunication();
	}

	return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
	int i;

	// Close all devices
	for(i = 0; i < MAX_UPS_DEVICES; i++)
		if (m_deviceInfo[i] != NULL)
		{
			delete_and_null(m_deviceInfo[i]);
		}
}

/**
 * Provided parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("UPS.BatteryLevel(*)"),           H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_BATTERY_LEVEL, TCHAR *),
		DCI_DT_INT,      _T("UPS {instance} battery charge level")
	},

	{ _T("UPS.BatteryVoltage(*)"),         H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_BATTERY_VOLTAGE, TCHAR *),
		DCI_DT_FLOAT,    _T("UPS {instance} battery voltage")
	},

	{ _T("UPS.ConnectionStatus(*)"),       H_UPSConnStatus,
		NULL,
		DCI_DT_INT,      _T("UPS {instance} connection status")
	},

	{ _T("UPS.EstimatedRuntime(*)"),       H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_EST_RUNTIME, TCHAR *),
		DCI_DT_INT,      _T("UPS {instance} estimated on-battery runtime (minutes)")
	},

	{ _T("UPS.Firmware(*)"),               H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_FIRMWARE, TCHAR *),
		DCI_DT_STRING,   _T("UPS {instance} firmware version")
	},

	{ _T("UPS.InputVoltage(*)"),           H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_INPUT_VOLTAGE, TCHAR *),
		DCI_DT_FLOAT,    _T("UPS {instance} input line voltage")
	},

	{ _T("UPS.LineFrequency(*)"),          H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_LINE_FREQ, TCHAR *),
		DCI_DT_INT,      _T("UPS {instance} input line frequency")
	},

	{ _T("UPS.Load(*)"),                   H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_LOAD, TCHAR *),
		DCI_DT_INT,      _T("UPS {instance} load")
	},

	{ _T("UPS.MfgDate(*)"),                H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_MFG_DATE, TCHAR *),
		DCI_DT_STRING,   _T("UPS {instance} manufacturing date")
	},

	{ _T("UPS.Model(*)"),                  H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_MODEL, TCHAR *),
		DCI_DT_STRING,   _T("UPS {instance} model")
	},

	{ _T("UPS.NominalBatteryVoltage(*)"),  H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_NOMINAL_BATT_VOLTAGE, TCHAR *),
		DCI_DT_FLOAT,    _T("UPS {instance} nominal battery voltage")
	},

	{ _T("UPS.OnlineStatus(*)"),           H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_ONLINE_STATUS, TCHAR *),
		DCI_DT_INT,      _T("UPS {instance} online status")
	},

	{ _T("UPS.OutputVoltage(*)"),          H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_OUTPUT_VOLTAGE, TCHAR *),
		DCI_DT_FLOAT,    _T("UPS {instance} output voltage")
	},

	{ _T("UPS.SerialNumber(*)"),           H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_SERIAL, TCHAR *),
		DCI_DT_STRING,   _T("UPS {instance} serial number")
	},

	{ _T("UPS.Temperature(*)"),            H_UPSData,
		CAST_TO_POINTER(UPS_PARAM_TEMP, TCHAR *),
		DCI_DT_INT,      _T("UPS {instance} temperature")
	}
};

/**
 * Provided lists
 */
static NETXMS_SUBAGENT_LIST m_enums[] =
{
	{ _T("UPS.DeviceList"), H_DeviceList, NULL }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("UPS"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
	m_enums,
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(UPS)
{
	*ppInfo = &m_info;
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
