/*
 ** Raspberry Pi subagent
 ** Copyright (C) 2013 Victor Kirhenshtein
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

#include <nms_common.h>
#include <nms_agent.h>

/**
 * 
 */
BOOL StartSensorCollector();
void StopSensorCollector();

/**
 * Sensor data
 */
extern float g_sensorData[];
extern time_t g_sensorUpdateTime;

/**
 * Configuration file template
 */
static TCHAR *m_enabledPins = NULL;
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ _T("EnabledPins"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_enabledPins },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Sensor reading
 */
static LONG H_Sensors(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG ret;
	if (time(NULL) - g_sensorUpdateTime <= 60)
	{
		ret_int(value, (int)g_sensorData[(int)arg]);
		ret = SYSINFO_RC_SUCCESS;
	}
	else
	{
		ret = SYSINFO_RC_ERROR;
	}

	return ret;
}

/**
 * GPIO pin state
 */
static LONG H_PinState(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG ret = SYSINFO_RC_ERROR;

	TCHAR tmp[128];
	if (!AgentGetParameterArg(param, 1, tmp, 128))
   {
		return SYSINFO_RC_UNSUPPORTED;
   }
	StrStrip(tmp);
   long pin = _tcstol(tmp, NULL, 10);

   uint8_t value = bcm2835_gpio_lev((uint8_t)pin);
   ret_int(value, value == HIGH ? 1 : 0);
   ret = SYSINFO_RC_SUCCESS;

	return ret;
}

/**
 * Startup handler
 */
static BOOL SubagentInit(Config *config)
{
	bool success;

	// Parse configuration
	success = config->parseTemplate(_T("RPI"), m_cfgTemplate);
	if (success)
	{
		TCHAR *item, *end;

		// Parse target list
		if (m_enabledPins != NULL)
		{
			for(item = m_enabledPins; *item != 0; item = end + 1)
			{
				end = _tcschr(item, _T('\n'));
				if (end != NULL)
            {
					*end = 0;
            }
				StrStrip(item);
            long pin = _tcstol(item, NULL, 10);
            AgentWriteDebugLog(1, _T("RPI: configuring pin %d as INPUT"), pin);
            bcm2835_gpio_fsel((uint8_t)pin, BCM2835_GPIO_FSEL_INPT);
			}
			free(m_enabledPins);
		}
   }

	return StartSensorCollector();
}

/**
 * Shutdown handler
 */
static void SubagentShutdown()
{
	StopSensorCollector();
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Sensors.Humidity"), H_Sensors, (TCHAR *)0, DCI_DT_INT, _T("Humidity") },
	{ _T("Sensors.Temperature"), H_Sensors, (TCHAR *)1, DCI_DT_INT, _T("Temperature") }
	{ _T("GPIO.PinState"), H_PinState, NULL, DCI_DT_INT, _T("Pin State") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("RPI"), NETXMS_VERSION_STRING,
	SubagentInit,
	SubagentShutdown,
	NULL, // command handler
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,		// lists
	0, NULL,		// tables
	0, NULL,		// actions
	0, NULL		// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(RPI)
{
	*ppInfo = &m_info;
	return TRUE;
}
