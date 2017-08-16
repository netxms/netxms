/*
 ** LoraWAN subagent
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

#include "lorawan.h"

/**
 * MQTT connection settings
 */
static MqttClient *s_mqtt;

/**
 * LoraWAN server link
 */
static LoraWanServerLink *s_link = NULL;

/**
 * LoraWAN device map
 */
HashMap<uuid, LoraDeviceData> g_deviceMap(true);

/**
 * Device map mutex
 */
MUTEX g_deviceMapMutex = INVALID_MUTEX_HANDLE;

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("LoraWAN.RSSI(*)"), H_Communication, _T("R"), DCI_DT_INT, _T("RSSI") },
   { _T("LoraWAN.SNR(*)"), H_Communication, _T("S"), DCI_DT_INT, _T("SNR") },
   { _T("LoraWAN.Frequency(*)"), H_Communication, _T("F"), DCI_DT_UINT, _T("Frequency") },
   { _T("LoraWAN.MessageCount(*)"), H_Communication, _T("M"), DCI_DT_UINT, _T("Message count") },
   { _T("LoraWAN.DataRate(*)"), H_Communication, _T("D"), DCI_DT_STRING, _T("Data rate") },
   { _T("LoraWAN.LastContact(*)"), H_Communication, _T("C"), DCI_DT_STRING, _T("Last contact") },
   { _T("LoraWAN.DevAddr(*)"), H_Communication, _T("A"), DCI_DT_STRING, _T("DevAddr") }
};

/**
 * Add LoraDeviceData object to DB and local hashmap
 */
UINT32 AddDevice(LoraDeviceData *device)
{
   UINT32 rcc = device->saveToDB(true);

   if (rcc == ERR_SUCCESS)
   {
      MutexLock(g_deviceMapMutex);
      g_deviceMap.set(device->getGuid(), device);
      MutexUnlock(g_deviceMapMutex);
   }

   return rcc;
}

/**
 * Remove LoraDeviceData object from DB and local hashmap
 */
UINT32 RemoveDevice(LoraDeviceData *device)
{
   UINT32 rcc = device->deleteFromDB();

   if (rcc == ERR_SUCCESS)
   {
      MutexLock(g_deviceMapMutex);
      g_deviceMap.remove(device->getGuid());
      MutexUnlock(g_deviceMapMutex);
   }

   return rcc;
}

/**
 * Find device in local map
 */
LoraDeviceData *FindDevice(uuid guid)
{
   LoraDeviceData *data;

   MutexLock(g_deviceMapMutex);
   data = g_deviceMap.get(guid);
   MutexUnlock(g_deviceMapMutex);

   return data;
}

/**
 * Load LoraWAN devices from DB
 */
static void LoadDevices()
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT guid,devAddr,devEui,decoder,last_contact FROM device_decoder_map"));

   if (hResult != NULL)
   {
      UINT32 nRows = DBGetNumRows(hResult);
      MutexLock(g_deviceMapMutex);
      for(int i = 0; i < nRows; i++)
      {
         LoraDeviceData *data = new LoraDeviceData(hResult, i);
         g_deviceMap.set(data->getGuid(), data);
      }
      MutexUnlock(g_deviceMapMutex);
      DBFreeResult(hResult);
   }
   else
      nxlog_debug(4, _T("LoraWAN Subagent: Unable to load device map table"));

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Process commands regarding LoraWAN devices
 */
static BOOL ProcessCommands(UINT32 command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   switch(command)
   {
      case CMD_REGISTER_LORAWAN_SENSOR:
         response->setField(VID_RCC, s_link->registerDevice(request));
         return TRUE;
         break;
      case CMD_UNREGISTER_LORAWAN_SENSOR:
         response->setField(VID_RCC, s_link->deleteDevice(request->getFieldAsGUID(VID_GUID)));
         return TRUE;
         break;
      default:
         return FALSE;
   }
}

/**
 * Startup handler
 */
static BOOL SubagentInit(Config *config)
{
   g_deviceMapMutex = MutexCreate();

   LoadDevices();

   s_mqtt = new MqttClient(config->getEntry(_T("/LORAWAN")));
   s_mqtt->setMessageHandler(MqttMessageHandler);
   s_mqtt->startNetworkLoop();
   s_link = new LoraWanServerLink(config->getEntry(_T("/LORAWAN")));
   s_link->connect();

   return TRUE;
}

/**
 * Shutdown handler
 */
static void SubagentShutdown()
{
   s_mqtt->stopNetworkLoop();

   MutexDestroy(g_deviceMapMutex);
   delete(s_mqtt);
   delete(s_link);
}

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("LORAWAN"), NETXMS_VERSION_STRING,
	SubagentInit,
	SubagentShutdown,
	ProcessCommands, // command handler
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
DECLARE_SUBAGENT_ENTRY_POINT(LORAWAN)
{
	*ppInfo = &m_info;
	return TRUE;
}
