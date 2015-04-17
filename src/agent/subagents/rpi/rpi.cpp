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
#include <bcm2835.h>

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
static bool m_disableDHT22 = false;
static TCHAR *m_inputPins = NULL;
static TCHAR *m_outputPins = NULL;
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("DisableDHT22"), CT_BOOLEAN, 0, 0, 1, 0, &m_disableDHT22 },
   { _T("InputPins"), CT_STRING_LIST, _T(','), 0, 0, 0, &m_inputPins },
   { _T("OutputPins"), CT_STRING_LIST, _T(','), 0, 0, 0, &m_outputPins },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Sensor reading
 */
static LONG H_Sensors(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG ret;

   if (m_disableDHT22)
      return SYSINFO_RC_UNSUPPORTED;

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
	TCHAR tmp[128];
	if (!AgentGetParameterArg(param, 1, tmp, 128))
   {
		return SYSINFO_RC_UNSUPPORTED;
   }
	StrStrip(tmp);
   long pin = _tcstol(tmp, NULL, 10);

   uint8_t level = bcm2835_gpio_lev((uint8_t)pin);
   ret_int(value, level == HIGH ? 1 : 0);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Set GPIO pin state
 */
static LONG H_SetPinState(const TCHAR *action, StringList *arguments, const TCHAR *data, AbstractCommSession *session)
{
   TCHAR *pinStr = arguments->get(0);
   TCHAR *stateStr = arguments->get(1);
   if (pinStr == NULL || stateStr == NULL) {
      return ERR_INTERNAL_ERROR;
   }
   long pin = _tcstol(pinStr, NULL, 10);
   long state = _tcstol(pinStr, NULL, 10);

   bcm2835_gpio_write(pin, state == 1 ? HIGH : LOW);

	return ERR_SUCCESS;
}

/**
 * Startup handler
 */
static BOOL SubagentInit(Config *config)
{
   if (!bcm2835_init()) 
   {    
      AgentWriteLog(NXLOG_ERROR, _T("RPI: call to bcm2835_init failed"));
      return FALSE;
   }

	// Parse configuration
	bool success = config->parseTemplate(_T("RPI"), m_cfgTemplate);
	if (success)
	{
		TCHAR *item, *end;

		if (m_inputPins != NULL)
		{
			for(item = m_inputPins; *item != 0; item = end + 1)
			{
				end = _tcschr(item, _T(','));
				if (end != NULL)
            {
					*end = 0;
            }
				StrStrip(item);
            int pin = (int)_tcstol(item, NULL, 10);
            AgentWriteDebugLog(1, _T("RPI: configuring pin %d as INPUT"), pin);
            bcm2835_gpio_fsel((uint8_t)pin, BCM2835_GPIO_FSEL_INPT);
			}
			free(m_inputPins);
		}
		if (m_outputPins != NULL)
		{
			for(item = m_outputPins; *item != 0; item = end + 1)
			{
				end = _tcschr(item, _T(','));
				if (end != NULL)
            {
					*end = 0;
            }
				StrStrip(item);
            int pin = (int)_tcstol(item, NULL, 10);
            AgentWriteDebugLog(1, _T("RPI: configuring pin %d as OUTPUT"), pin);
            bcm2835_gpio_fsel((uint8_t)pin, BCM2835_GPIO_FSEL_OUTP);
			}
			free(m_outputPins);
		}
   }

	BOOL ret = TRUE;
   if (!m_disableDHT22)
   {
      ret = StartSensorCollector();
   }
   return ret;
}

/**
 * Shutdown handler
 */
static void SubagentShutdown()
{
   if (!m_disableDHT22)
      StopSensorCollector();
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("GPIO.PinState(*)"), H_PinState, NULL, DCI_DT_INT, _T("Pin {instance} state") },
	{ _T("Sensors.Humidity"), H_Sensors, (TCHAR *)0, DCI_DT_INT, _T("Humidity") },
	{ _T("Sensors.Temperature"), H_Sensors, (TCHAR *)1, DCI_DT_INT, _T("Temperature") }
};

/**
 * Subagent's actions
 */
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
	{ _T("GPIO.SetPinState"), H_SetPinState, NULL, _T("Set GPIO pin ($1) state to high/low ($2 - 1/0)") }
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
	sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
	m_actions,
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
