/*
 ** MQTT subagent
 ** Copyright (C) 2017 Raden Solutions
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

/**
 * Registered brokers
 */
static ObjectArray<MqttBroker> s_brokers(8, 8, true);

/**
 * Add brokers and parameters from config
 */
static void RegisterBrokers(StructArray<NETXMS_SUBAGENT_PARAM> *parameters, Config *config)
{
   ObjectArray<ConfigEntry> *brokers = config->getSubEntries(_T("/MQTT/Brokers"), _T("*"));
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

   nxlog_debug(3, _T("MQTT: %d parameters added from configuration"), parameters->size());
}

/**
 * Initialize subagent
 */
static BOOL SubAgentInit(Config *config)
{
   mosquitto_lib_init();

   int major, minor, rev;
   mosquitto_lib_version(&major, &minor, &rev);
   nxlog_debug(2, _T("MQTT: using libmosquitto %d.%d.%d"), major, minor, rev);

   // Start network loops
   for(int i = 0; i < s_brokers.size(); i++)
      s_brokers.get(i)->startNetworkLoop();

   return TRUE;
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
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("MQTT"), NETXMS_BUILD_TAG,
	SubAgentInit, SubAgentShutdown, NULL,
	0, NULL,    // parameters
	0, NULL,		// lists
	0, NULL,		// tables
	0, NULL,		// actions
	0, NULL		// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(MQTT)
{
   StructArray<NETXMS_SUBAGENT_PARAM> *parameters = new StructArray<NETXMS_SUBAGENT_PARAM>();

   RegisterBrokers(parameters, config);

   m_info.numParameters = parameters->size();
   m_info.parameters = (NETXMS_SUBAGENT_PARAM *)nx_memdup(parameters->getBuffer(),
                     parameters->size() * sizeof(NETXMS_SUBAGENT_PARAM));
   delete parameters;

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
