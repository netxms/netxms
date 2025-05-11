/*
 ** MQTT subagent
 ** Copyright (C) 2017-2025 Raden Solutions
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
 * Auto registration flag
 */
static bool s_enableAutoRegistration = true;

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
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
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
      value->set(1, b->getName());
      value->set(2, b->getHostname());
      value->set(3, b->getPort());
      value->set(4, CHECK_NULL_EX_A(b->getLogin()));
      value->set(5, b->isLocallyConfigured() ? 1 : 0);
      value->set(6, b->getTopicCount());
      value->set(7, b->isConnected() ? 1 : 0);
   }
   s_brokersLock.unlock();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Add brokers and parameters from config
 */
static void RegisterBrokers(StructArray<NETXMS_SUBAGENT_PARAM> *parameters, StructArray<NETXMS_SUBAGENT_LIST> *lists, Config *config)
{
   int initialParameterCount = parameters->size();
   int initialListCount = lists->size();
   unique_ptr<ObjectArray<ConfigEntry>> brokers = config->getSubEntries(_T("/MQTT/Brokers"), _T("*"));
   if (brokers != nullptr)
   {
      for(int i = 0; i < brokers->size(); i++)
      {
         MqttBroker *b = MqttBroker::createFromConfig(brokers->get(i), parameters, lists);
         if (b != nullptr)
         {
            s_brokers.add(b);
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot add broker %s definition from config"), brokers->get(i)->getName());
         }
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("%d MQTT parameters and %d MQTT lists added from configuration"), parameters->size() - initialParameterCount, lists->size() - initialListCount);
}

/**
 * Handler for MQTT.TopicValue parameter
 */
static LONG H_TopicValue(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char buffer[1024];
   if (!AgentGetParameterArgA(cmd, 1, buffer, 1024))
      return SYSINFO_RC_UNSUPPORTED;

   char *brokerName;
   char *topic = strchr(buffer, ':'); // Check if provided as broker:topic
   if (topic != nullptr)
   {
      *topic = 0;
      topic++;
      brokerName = buffer;
   }
   else
   {
      topic = buffer;
      brokerName = nullptr;
   }

   MqttBroker *broker = nullptr;
   s_brokersLock.lock();
   if (brokerName != nullptr)
   {
      for(int i = 0; i < s_brokers.size(); i++)
      {
         MqttBroker *b = s_brokers.get(i);
         if (!stricmp(b->getName(), brokerName))
         {
            broker = b;
            break;
         }
      }
   }
   else if (!s_brokers.isEmpty())
   {
      broker = s_brokers.get(0);
   }
   s_brokersLock.unlock();

   return (broker != nullptr) ? broker->getTopicData(topic, value, s_enableAutoRegistration) : SYSINFO_RC_NO_SUCH_INSTANCE;
}

/**
 * Initialize subagent
 */
static bool SubAgentInit(Config *config)
{
   mosquitto_lib_init();

   int major, minor, rev;
   mosquitto_lib_version(&major, &minor, &rev);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Using libmosquitto %d.%d.%d"), major, minor, rev);

   s_enableAutoRegistration = config->getValueAsBoolean(_T("/MQTT/EnableTopicAutoRegistration"), s_enableAutoRegistration);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Automatic registration of MQTT topics is %s"), s_enableAutoRegistration ? _T("enabled") : _T("disabled"));

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
   nxlog_debug_tag(DEBUG_TAG, 2, _T("MQTT subagent shutdown completed"));
}

/**
 * Provided tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("MQTT.Brokers"), H_BrokersTable, nullptr, _T("GUID"), _T("MQTT: configured brokers") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("MQTT"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
	0, nullptr,    // parameters
	0, nullptr,		// lists
	sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
	0, nullptr,		// actions
	0, nullptr		// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(MQTT)
{
   StructArray<NETXMS_SUBAGENT_PARAM> parameters;

   NETXMS_SUBAGENT_PARAM *p = parameters.addPlaceholder();
   memset(p, 0, sizeof(NETXMS_SUBAGENT_PARAM));
   _tcscpy(p->name, _T("MQTT.TopicData(*)"));
   _tcscpy(p->description, _T("Last known value for MQTT topic {instance}"));
   p->dataType = DCI_DT_STRING;
   p->handler = H_TopicValue;

   StructArray<NETXMS_SUBAGENT_LIST> lists;

   RegisterBrokers(&parameters, &lists, config);

   s_info.numParameters = parameters.size();
   s_info.parameters = MemCopyArray(parameters.getBuffer(), parameters.size());
   s_info.numLists = lists.size();
   s_info.lists = MemCopyArray(lists.getBuffer(), lists.size());

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
