/*
 ** MQTT subagent
 ** Copyright (C) 2017-2020 Raden Solutions
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
#include "mqtt_subagent.h"
#include <netxms-version.h>

/**
 * Registered brokers
 */
static ObjectArray<MqttBroker> s_brokers(8, 8, Ownership::True);
static Mutex s_brokersLock;

/**
 * Handler for MQTT.Brokers table
 */
static LONG H_BrokersTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("GUID"), DCI_DT_STRING, _T("GUID"), true);
   value->addColumn(_T("HOSTNAME"), DCI_DT_STRING, _T("Hostname"));
   value->addColumn(_T("PORT"), DCI_DT_UINT, _T("Port"));
   value->addColumn(_T("LOGIN"), DCI_DT_STRING, _T("Login"));
   value->addColumn(_T("IS_LOCAL"), DCI_DT_STRING, _T("Is local"));
   value->addColumn(_T("TOPICS"), DCI_DT_UINT, _T("Topics"));
   value->addColumn(_T("IS_CONNECTED"), DCI_DT_UINT, _T("Is connected"));

   s_brokersLock.lock();
   for(int i = 0; i < s_brokers.size(); i++)
   {
      value->addRow();
      MqttBroker *b = s_brokers.get(i);
      value->set(0, b->getGuid().toString());
      value->set(1, b->getHostname());
      value->set(2, b->getPort());
      value->set(3, CHECK_NULL_EX_A(b->getLogin()));
      value->set(4, b->isLocallyConfigured() ? 1 : 0);
      value->set(5, b->getTopicCount());
      value->set(6, b->isConnected() ? 1 : 0);
   }
   s_brokersLock.unlock();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Add brokers and parameters from config
 */
static void RegisterBrokers(StructArray<NETXMS_SUBAGENT_PARAM> *parameters, Config *config)
{
   ObjectArray<ConfigEntry> *brokers = config->getSubEntries(_T("/MQTT/Brokers"), _T("*"));
   if (brokers != NULL)
   {
      for(int i = 0; i < brokers->size(); i++)
      {
         MqttBroker *b = MqttBroker::createFromConfig(brokers->get(i), parameters);
         if (b != NULL)
         {
            s_brokers.add(b);
         }
         else
         {
            AgentWriteLog(NXLOG_WARNING, _T("MQTT: cannot add broker %s definition from config"), brokers->get(i)->getName());
         }
      }
      delete brokers;
   }
   nxlog_debug(3, _T("MQTT: %d parameters added from configuration"), parameters->size());
}

/**
 * Initialize subagent
 */
static bool SubAgentInit(Config *config)
{
   mosquitto_lib_init();

   int major, minor, rev;
   mosquitto_lib_version(&major, &minor, &rev);
   nxlog_debug(2, _T("MQTT: using libmosquitto %d.%d.%d"), major, minor, rev);

   // Start network loops
   for(int i = 0; i < s_brokers.size(); i++)
      s_brokers.get(i)->startNetworkLoop();

   return true;
}

/**
 * Shutdown subagent
 */
static void SubAgentShutdown()
{
   // Stop network loops
   for(int i = 0; i < s_brokers.size(); i++)
      s_brokers.get(i)->stopNetworkLoop();

   mosquitto_lib_cleanup();
   nxlog_debug(2, _T("MQTT subagent shutdown completed"));
}

/**
 * Command handler
 */
static bool CommandHandler(UINT32 command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   switch(command)
   {
      case CMD_CONFIGURE_MQTT_BROKER:
         return true;
      case CMD_REMOVE_MQTT_BROKER:
         return true;
      case CMD_ADD_MQTT_TOPIC:
         return true;
      case CMD_REMOVE_MQTT_TOPIC:
         return true;
      default:
         return false;
   }
}

/**
 * Provided tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("MQTT.Brokers"), H_BrokersTable, NULL, _T("GUID"), _T("MQTT: configured brokers") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("MQTT"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, CommandHandler, NULL,
	0, NULL,    // parameters
	0, NULL,		// lists
	sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
	0, NULL,		// actions
	0, NULL		// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(MQTT)
{
   StructArray<NETXMS_SUBAGENT_PARAM> parameters;

   RegisterBrokers(&parameters, config);

   m_info.numParameters = parameters.size();
   m_info.parameters = MemCopyArray(parameters.getBuffer(), parameters.size());

	*ppInfo = &m_info;
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
