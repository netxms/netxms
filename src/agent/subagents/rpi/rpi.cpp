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
bool StartSensorCollector();
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
static LONG H_SetPinState(const TCHAR *action, const StringList *arguments, const TCHAR *data, AbstractCommSession *session)
{
   const TCHAR *pinStr = arguments->get(0);
   const TCHAR *stateStr = arguments->get(1);
   if (pinStr == NULL || stateStr == NULL) 
   {
      return ERR_INTERNAL_ERROR;
   }
   uint8_t pin = (uint8_t)_tcstol(pinStr, NULL, 10);
   uint8_t state = (uint8_t)_tcstol(stateStr, NULL, 10);

   bcm2835_gpio_write(pin, state == 1 ? HIGH : LOW);

   return ERR_SUCCESS;
}

/**
 *  CPU Temperature 
 */
static LONG H_CpuTemperature(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   FILE *cpuTemp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
   UINT32 rc = SYSINFO_RC_ERROR;
   if (cpuTemp == NULL)
      return rc;

   char buffer[10];
   if (fgets(buffer, 10, cpuTemp) != NULL)
   {
      float result;
      if(sscanf(buffer, "%f", &result))
      {
         result = result / 1000.0;
         ret_double(value, result, 1);
         rc = SYSINFO_RC_SUCCESS;
      }
   }
   fclose(cpuTemp);
   return rc;
}

/**
 * Parse coma separated lines from configuration
 * and FREE input.
 * NULL safe.
 */
static void ConfigureGPIO(TCHAR *str, uint8_t mode)
{
   if (str != NULL)
   {
      TCHAR *item, *end;
      for(item = str; *item != 0; item = end + 1)
      {
         end = _tcschr(item, _T(','));
         if (end != NULL)
         {
            *end = 0;
         }
         StrStrip(item);
         uint8_t pin = (uint8_t)_tcstol(item, NULL, 10);
         AgentWriteDebugLog(1, _T("RPI: configuring gpio%u as %s"), pin,
               mode == BCM2835_GPIO_FSEL_INPT ? _T("INPUT") : _T("OUTPUT"));
         bcm2835_gpio_fsel(pin, mode);
      }
      free(str);
   }
}

/**
 * Startup handler
 */
static bool SubagentInit(Config *config)
{
   if (bcm2835_init() != 1)
   {    
      AgentWriteLog(NXLOG_ERROR, _T("RPI: call to bcm2835_init failed"));
      return false;
   }

	// Parse configuration
	bool success = config->parseTemplate(_T("RPI"), m_cfgTemplate);
	if (success)
	{
      ConfigureGPIO(m_inputPins, BCM2835_GPIO_FSEL_INPT);
      ConfigureGPIO(m_outputPins, BCM2835_GPIO_FSEL_OUTP);
   }

	bool ret = true;
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
	{ _T("Sensors.Temperature"), H_Sensors, (TCHAR *)1, DCI_DT_INT, _T("Temperature") },
	{ _T("System.CPU.Temperature"), H_CpuTemperature, _T("T"), DCI_DT_FLOAT, _T("CPU: temperature") }
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
	NULL, // notification handler
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
	return true;
}
