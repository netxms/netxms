/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2025 Victor Kirhenshtein
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
#include <netxms-version.h>

/**
 * Device list
 */
static UPSInterface *m_deviceInfo[MAX_UPS_DEVICES];

/**
 * Universal handler
 */
static LONG H_UPSData(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	TCHAR deviceIdText[256];
	if (!AgentGetParameterArg(pszParam, 1, deviceIdText, 256))
		return SYSINFO_RC_UNSUPPORTED;

   TCHAR *eptr;
	int deviceId = _tcstol(deviceIdText, &eptr, 0);
	if ((*eptr != 0) || (deviceId < 0) || (deviceId >= MAX_UPS_DEVICES))
		return SYSINFO_RC_UNSUPPORTED;

	if (m_deviceInfo[deviceId] == nullptr)
		return SYSINFO_RC_UNSUPPORTED;

	if (!m_deviceInfo[deviceId]->isConnected())
		return SYSINFO_RC_ERROR;

	return m_deviceInfo[deviceId]->getParameter(CAST_FROM_POINTER(pArg, int), pValue);
}

/**
 * UPS connection status
 */
static LONG H_UPSConnStatus(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	TCHAR deviceIdText[256];
	if (!AgentGetParameterArg(pszParam, 1, deviceIdText, 256))
		return SYSINFO_RC_UNSUPPORTED;

   TCHAR *eptr;
	int deviceId = _tcstol(deviceIdText, &eptr, 0);
	if ((*eptr != 0) || (deviceId < 0) || (deviceId >= MAX_UPS_DEVICES))
		return SYSINFO_RC_UNSUPPORTED;

	if (m_deviceInfo[deviceId] == nullptr)
		return SYSINFO_RC_UNSUPPORTED;

	ret_boolean(pValue, m_deviceInfo[deviceId]->isConnected());
	return SYSINFO_RC_SUCCESS;
}

/**
 * List configured devices
 */
static LONG H_DeviceList(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
	TCHAR buffer[256];
	for(int i = 0; i < MAX_UPS_DEVICES; i++)
		if (m_deviceInfo[i] != nullptr)
		{
			_sntprintf(buffer, 256, _T("%d %s %s %s"), i,
					m_deviceInfo[i]->getDevice(), m_deviceInfo[i]->getType(),
					m_deviceInfo[i]->getName());
			value->add(buffer);
		}
	return SYSINFO_RC_SUCCESS;
}

/**
 * Add device from configuration file parameter
 * Parameter value should be <device_id>:<port>:<protocol>:<name>
 */
static bool AddDeviceFromConfig(const TCHAR *configString)
{
	const TCHAR *ptr;
	TCHAR *eptr;
	TCHAR port[MAX_PATH], name[MAX_DB_STRING] = _T("");
	int state, field, deviceIndex = 0, pos, protocol = 0;

	// Parse line
	TCHAR *currField = MemAllocString(_tcslen(configString) + 1);
	for(ptr = configString, state = 0, field = 0, pos = 0; (state != -1) && (state != 255); ptr++)
	{
		switch(state)
		{
			case 0:  // Normal text
				switch(*ptr)
				{
					case '\'':
						state = 1;
						break;
					case '"':
						state = 2;
						break;
					case ':':   // New field
					case 0:
						currField[pos] = 0;
						switch(field)
						{
							case 0:  // Device number
								deviceIndex = _tcstol(currField, &eptr, 0);
								if ((*eptr != 0) || (deviceIndex < 0) || (deviceIndex >= MAX_UPS_DEVICES))
									state = 255;  // Error
								break;
							case 1:  // Serial port
							   _tcslcpy(port, currField, MAX_PATH);
								break;
							case 2:  // Protocol
								if (!_tcsicmp(currField, _T("APC")))
								{
									protocol = UPS_PROTOCOL_APC;
								}
								else if (!_tcsicmp(currField, _T("BCMXCP")))
								{
									protocol = UPS_PROTOCOL_BCMXCP;
								}
								else if (!_tcsicmp(currField, _T("MEGATEC")))
								{
									protocol = UPS_PROTOCOL_MEGATEC;
								}
								else if (!_tcsicmp(currField, _T("METASYS")))
								{
									protocol = UPS_PROTOCOL_METASYS;
								}
								else if (!_tcsicmp(currField, _T("MICRODOWELL")))
								{
									protocol = UPS_PROTOCOL_MICRODOWELL;
								}
#ifdef _WIN32
								else if (!_tcsicmp(currField, _T("USB")))
								{
									protocol = UPS_PROTOCOL_USB;
								}
								else if (!_tcsicmp(currField, _T("MEC0003")))
								{
									protocol = UPS_PROTOCOL_MEC0003;
								}
#endif
								else
								{
									state = 255;  // Error
								}
								break;
							case 3:  // Name
								_tcslcpy(name, currField, MAX_DB_STRING);
								break;
							default:
								state = 255;  // Error
								break;
						}
						field++;
						pos = 0;
						if ((state != 255) && (*ptr == 0))
							state = -1;   // Finish
						break;
					default:
						currField[pos++] = *ptr;
						break;
				}
				break;
			case 1:  // Single quotes
				switch(*ptr)
				{
					case '\'':
						state = 0;
						break;
					case 0:  // Unexpected end of line
						state = 255;
						break;
					default:
						currField[pos++] = *ptr;
						break;
				}
				break;
			case 2:  // Double quotes
				switch(*ptr)
				{
					case '"':
						state = 0;
						break;
					case 0:  // Unexpected end of line
						state = 255;
						break;
					default:
						currField[pos++] = *ptr;
						break;
				}
				break;
			default:
				break;
		}
	}
	MemFree(currField);

	// Add new device if parsing was successful
	if ((state == -1) && (field >= 3))
	{
		if (m_deviceInfo[deviceIndex] != nullptr)
			delete m_deviceInfo[deviceIndex];
		switch(protocol)
		{
			case UPS_PROTOCOL_APC:
				m_deviceInfo[deviceIndex] = new APCInterface(port);
				break;
			case UPS_PROTOCOL_BCMXCP:
				m_deviceInfo[deviceIndex] = new BCMXCPInterface(port);
				break;
         case UPS_PROTOCOL_MEGATEC:
				m_deviceInfo[deviceIndex] = new MegatecInterface(port);
				break;
         case UPS_PROTOCOL_METASYS:
				m_deviceInfo[deviceIndex] = new MetaSysInterface(port);
				break;
			case UPS_PROTOCOL_MICRODOWELL:
				m_deviceInfo[deviceIndex] = new MicrodowellInterface(port);
				break;
#ifdef _WIN32
			case UPS_PROTOCOL_USB:
				m_deviceInfo[deviceIndex] = new USBInterface(port);
				break;
			case UPS_PROTOCOL_MEC0003:
				m_deviceInfo[deviceIndex] = new MEC0003Interface(port);
				break;
#endif
			default:
				break;
		}
		m_deviceInfo[deviceIndex]->setName(name);
		m_deviceInfo[deviceIndex]->setId(deviceIndex);
	}

	return ((state == -1) && (field >= 3));
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
	memset(m_deviceInfo, 0, sizeof(UPSInterface *) * MAX_UPS_DEVICES);

	// Enumerate MEC0003 devices on startup
#ifdef _WIN32
	MEC0003Interface::enumerateDevices();
#endif

	// Parse configuration
	ConfigEntry *devices = config->getEntry(_T("/UPS/Device"));
	if (devices != nullptr)
	{
		for(int i = 0; i < devices->getValueCount(); i++)
		{
			TCHAR *entry = Trim(MemCopyString(devices->getValue(i)));
			if (!AddDeviceFromConfig(entry))
			{
				nxlog_write_tag(NXLOG_WARNING, UPS_DEBUG_TAG,
						_T("Unable to add UPS device from configuration file. ")
						_T("Original configuration record: %s"), devices->getValue(i));
			}
			MemFree(entry);
		}
	}

	// Start communicating with configured devices
	for(int i = 0; i < MAX_UPS_DEVICES; i++)
	{
		if (m_deviceInfo[i] != nullptr)
			m_deviceInfo[i]->startCommunication();
	}

	return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
	for(int i = 0; i < MAX_UPS_DEVICES; i++)
      delete_and_null(m_deviceInfo[i]);
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
		nullptr,
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
	{ _T("UPS.DeviceList"), H_DeviceList, nullptr }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("UPS"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
   m_enums,
   0, nullptr,	// tables
   0, nullptr,	// actions
   0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(UPS)
{
   *ppInfo = &s_info;
   return true;
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
