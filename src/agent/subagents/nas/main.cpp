/*
 ** NAS Smart water meter retrofit decoder for LoraWAN subagent
 ** Copyright (C) 2009 - 2017 Raden Solutions
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

#include "nas.h"

/**
 * Sensor data map
 */
HashMap<uuid, SensorData> g_sensorMap(true);

/**
 * Find sensor data in local cache
 */
 SensorData *FindSensor(uuid guid)
{
   SensorData *data;
   MutexLock(s_sensorMapMutex);
   data = g_sensorMap.get(guid);
   MutexUnlock(s_sensorMapMutex);

   return data;
}

/**
 * Add sensor data to local cache
 */
void AddSensor(SensorData *data)
{
   MutexLock(s_sensorMapMutex);
   g_sensorMap.set(data->guid, data);
   MutexUnlock(s_sensorMapMutex);
}

/**
 * Startup handler
 */
static BOOL SubagentInit(Config *config)
{
   s_sensorMapMutex = MutexCreate();

   NXMBDispatcher *dispatcher = NXMBDispatcher::getInstance();
   dispatcher->addCallHandler(_T("NOTIFY_DECODERS"), Decode);

   return TRUE;
}

/**
 * Shutdown handler
 */
static void SubagentShutdown()
{
   NXMBDispatcher *dispatcher = NXMBDispatcher::getInstance();
   dispatcher->removeCallHandler(_T("NOTIFY_DECODERS"));
   g_sensorMap.clear();
   MutexDestroy(s_sensorMapMutex);
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("WaterMeter.Usage(*)"), H_Sensor, _T("U"), DCI_DT_UINT64, _T("Usage") },
	{ _T("WaterMeter.Battery(*)"), H_Sensor, _T("B"), DCI_DT_FLOAT, _T("Battery voltage") },
   { _T("WaterMeter.RSSI(*)"), H_Sensor, _T("R"), DCI_DT_INT, _T("RSSI") },
	{ _T("WaterMeter.Temperature(*)"), H_Status, _T("T"), DCI_DT_UINT, _T("Temperature") },
   { _T("WaterMeter.Watch.Mode(*)"), H_Mode, _T("W"), DCI_DT_UINT, _T("Watch mode") },
   { _T("WaterMeter.Watch.Status(*)"), H_Status, _T("W"), DCI_DT_UINT, _T("Watch status") },
   { _T("WaterMeter.Leak.Mode(*)"), H_Mode, _T("L"), DCI_DT_UINT, _T("Leak mode") },
   { _T("WaterMeter.Leak.Status(*)"), H_Status, _T("L"), DCI_DT_UINT, _T("Leak status") },
   { _T("WaterMeter.ReverseFlow.Mode(*)"), H_Mode, _T("F"), DCI_DT_UINT, _T("Reverse flow mode") },
   { _T("WaterMeter.ReverseFlow.Status(*)"), H_Status, _T("R"), DCI_DT_UINT, _T("Reverse flow status") },
   { _T("WaterMeter.Tamper.Mode(*)"), H_Mode, _T("A"), DCI_DT_UINT, _T("Tamper mode") },
   { _T("WaterMeter.Tamper.Status(*)"), H_Status, _T("A"), DCI_DT_UINT, _T("Tamper status") },
   { _T("WaterMeter.Sensor.Mode(*)"), H_Mode, _T("S"), DCI_DT_UINT, _T("Sensor mode") },
   { _T("WaterMeter.Reporting.Mode(*)"), H_Mode, _T("R"), DCI_DT_UINT, _T("Reporting mode") },
   { _T("WaterMeter.TemperatureDetection.Mode(*)"), H_Mode, _T("T"), DCI_DT_UINT, _T("Temperature detection mode") },
   { _T("WaterMeter.TemperatureDetection.Status(*)"), H_Status, _T("D"), DCI_DT_UINT, _T("Temperature detection status") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("NAS"), NETXMS_VERSION_STRING,
	SubagentInit,
	SubagentShutdown,
	NULL, // command handler
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,		// lists
	0, NULL,		// tables
	0, NULL,    // actions
	0, NULL		// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(NAS)
{
	*ppInfo = &m_info;
	return TRUE;
}
