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
 * Log callback
 */
static void LogCallback(struct mosquitto *handle, void *userData, int level, const char *message)
{
   nxlog_debug((level == MOSQ_LOG_DEBUG) ? 7 : 3, _T("MQTT: %hs"), message);
}

/**
 * Topic parameter handler
 */
static LONG H_TopicData(const TCHAR *name, const TCHAR *arg, TCHAR *result, AbstractCommSession *session)
{
   Topic *topic = (Topic *)arg;
   return topic->retrieveData(result, MAX_RESULT_LENGTH) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Broker constructor
 */
MqttBroker::MqttBroker() : m_topics(16, 16, true)
{
   m_hostname = NULL;
   m_port = 0;
   m_login = NULL;
   m_password = NULL;
   m_loopThread = INVALID_THREAD_HANDLE;

   char clientId[128];
   strcpy(clientId, "nxagentd/");
   char *guid = uuid::generate().toString().getUTF8String();
   strcat(clientId, guid);
   free(guid);
   m_handle = mosquitto_new(clientId, true, this);
}

/**
 * Broker destructor
 */
MqttBroker::~MqttBroker()
{
   if (m_loopThread != INVALID_THREAD_HANDLE)
      ThreadJoin(m_loopThread);
   if (m_handle != NULL)
      mosquitto_destroy(m_handle);
   free(m_hostname);
   free(m_login);
   free(m_password);
}

/**
 * Create broker object from configuration
 */
MqttBroker *MqttBroker::createFromConfig(const ConfigEntry *config, StructArray<NETXMS_SUBAGENT_PARAM> *parameters)
{
   MqttBroker *broker = new MqttBroker();
   if (broker->m_handle == NULL)
   {
      nxlog_debug(3, _T("MQTT: cannot create client instance"));
      delete broker;
      return NULL;
   }

   mosquitto_log_callback_set(broker->m_handle, LogCallback);
   mosquitto_message_callback_set(broker->m_handle, MqttBroker::messageCallback);

#ifdef UNICODE
   broker->m_hostname = UTF8StringFromWideString(config->getSubEntryValue(L"Hostname", 0, L"127.0.0.1"));
#else
   broker->m_hostname = strdup(config->getSubEntryValue("Hostname", 0, "127.0.0.1"));
#endif
   broker->m_port = (UINT16)config->getSubEntryValueAsUInt(_T("Port"), 0, 1883);
   broker->m_login = _tcsdup_ex(config->getSubEntryValue(_T("Login")));
   broker->m_password = _tcsdup_ex(config->getSubEntryValue(_T("Password")));

   const ConfigEntry *metricRoot = config->findEntry(_T("Metrics"));
   if (metricRoot != NULL)
   {
      ObjectArray<ConfigEntry> *metrics = metricRoot->getSubEntries(_T("*"));
      for(int i = 0; i < metrics->size(); i++)
      {
         ConfigEntry *e = metrics->get(i);
         Topic *t = new Topic(e->getValue());
         broker->m_topics.add(t);

         NETXMS_SUBAGENT_PARAM p;
         memset(&p, 0, sizeof(NETXMS_SUBAGENT_PARAM));
         nx_strncpy(p.name, e->getName(), MAX_PARAM_NAME);
         p.arg = (const TCHAR *)t;
         p.dataType = DCI_DT_STRING;
         p.handler = H_TopicData;
         _sntprintf(p.description, MAX_DB_STRING, _T("MQTT topic %hs"), t->getPattern());
         parameters->add(&p);
      }
      delete metrics;
   }

   const ConfigEntry *eventRoot = config->findEntry(_T("Events"));
   if (eventRoot != NULL)
   {
      ObjectArray<ConfigEntry> *events = eventRoot->getSubEntries(_T("*"));
      for(int i = 0; i < events->size(); i++)
      {
         ConfigEntry *e = events->get(i);
         Topic *t = new Topic(e->getValue(), e->getName());
         broker->m_topics.add(t);
      }
      delete events;
   }

   return broker;
}

/**
 * Broker network loop
 */
void MqttBroker::networkLoop()
{
   while(mosquitto_connect(m_handle, m_hostname, m_port, 600) != MOSQ_ERR_SUCCESS)
   {
      nxlog_debug(4, _T("MQTT: unable to connect to broker at %hs:%d, will retry in 60 seconds"), m_hostname, (int)m_port);
      if (AgentSleepAndCheckForShutdown(60000))
         return;  // Agent shutdown
   }

   nxlog_debug(3, _T("MQTT: connected to broker %hs:%d"), m_hostname, (int)m_port);

   for(int i = 0; i < m_topics.size(); i++)
   {
      Topic *t = m_topics.get(i);
      if (mosquitto_subscribe(m_handle, NULL, t->getPattern(), 0) == MOSQ_ERR_SUCCESS)
         nxlog_debug(4, _T("MQTT: subscribed to topic %hs on broker %hs:%d"), t->getPattern(), m_hostname, (int)m_port);
      else
         AgentWriteDebugLog(NXLOG_WARNING, _T("MQTT: cannot subscribe to topic %hs on broker %hs:%d"), t->getPattern(), m_hostname, (int)m_port);
   }

   mosquitto_loop_forever(m_handle, -1, 1);
   nxlog_debug(3, _T("MQTT: network loop stopped for broker %hs:%d"), m_hostname, (int)m_port);
}

/**
 * Broker network loop starter
 */
THREAD_RESULT THREAD_CALL MqttBroker::networkLoopStarter(void *arg)
{
   ((MqttBroker *)arg)->networkLoop();
   return THREAD_OK;
}

/**
 * Start broker network loop
 */
void MqttBroker::startNetworkLoop()
{
   m_loopThread = ThreadCreateEx(MqttBroker::networkLoopStarter, 0, this);
}

/**
 * Stop broker network loop
 */
void MqttBroker::stopNetworkLoop()
{
   mosquitto_disconnect(m_handle);
   ThreadJoin(m_loopThread);
   m_loopThread = INVALID_THREAD_HANDLE;
}

/**
 * Message callback
 */
void MqttBroker::messageCallback(struct mosquitto *handle, void *userData, const struct mosquitto_message *msg)
{
   ((MqttBroker *)userData)->processMessage(msg);
}

/**
 * Process broker message
 */
void MqttBroker::processMessage(const struct mosquitto_message *msg)
{
   if (msg->payloadlen <= 0)
      return;  // NULL message

   nxlog_debug(6, _T("MQTT: message received: %hs=\"%hs\""), msg->topic, (const char *)msg->payload);
   for(int i = 0; i < m_topics.size(); i++)
   {
      m_topics.get(i)->processMessage(msg->topic, (const char *)msg->payload);
   }
}
