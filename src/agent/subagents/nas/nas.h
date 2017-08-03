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

#include <nms_agent.h>
#include <nms_util.h>
#include <nxmbapi.h>

/**
 * Decoder ID
 */
#define NAS 0

/**
 * Sensor data field defines
 */
#define SENSOR_COUNTER32            0
#define SENSOR_COUNTER64            1
#define SENSOR_BATTERY              2
#define SENSOR_TEMP                 3
#define SENSOR_RSSI                 4
#define SENSOR_WATCH_MODE           5
#define SENSOR_WATCH_STATUS         6
#define SENSOR_LEAK_MODE            7
#define SENSOR_LEAK_STATUS          8
#define SENSOR_REV_FLOW_MODE        9
#define SENSOR_REV_FLOW_STATUS      10
#define SENSOR_TAMPER_MODE          11
#define SENSOR_TAMPER_STATUS        12
#define SENSOR_MODE                 13
#define SENSOR_REPORTING_MODE       14
#define SENSOR_TEMP_DETECT_MODE     15
#define SENSOR_TEMP_DETECT_STATUS   16

/**
 * Sensor data struct
 */
struct SensorData
{
   uuid guid;
   UINT64 counter;
   double batt;
   INT32 rssi;
   INT32 temp;
   UINT32 watchMode;
   UINT32 watchStatus;
   UINT32 leakMode;
   UINT32 leakStatus;
   UINT32 revFlowMode;
   UINT32 revFlowStatus;
   UINT32 tamperMode;
   UINT32 tamperStatus;
   UINT32 sensorMode;
   UINT32 reportingMode;
   UINT32 tempDetectMode;
   UINT32 tempDetectStatus;

   ~SensorData() {}
};

/**
 * Sensor data map
 */
extern HashMap<uuid, SensorData> g_sensorMap;

/**
 * Sensor map mutex
 */
static MUTEX s_sensorMapMutex = INVALID_MUTEX_HANDLE;

/**
 * Sensor data handlers
 */
LONG H_Sensor(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_Mode(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_Status(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Sensor map functions
 */
SensorData *FindSensor(uuid guid);
void AddSensor(SensorData *data);

/**
 * NXMBDispatcher handler
 */
bool Decode(const TCHAR *name, const void *data, void *result);
