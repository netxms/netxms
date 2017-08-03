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
 * Simple MQTT client constructor
 */
MqttClient::MqttClient(const ConfigEntry *config)
{
   mosquitto_lib_init();
#ifdef UNICODE
   m_hostname = UTF8StringFromWideString(config->getSubEntryValue(L"Hostname", 0, L"127.0.0.1"));
   m_pattern = UTF8StringFromWideString(config->getSubEntryValue(L"Topic", 0, L"#"));
#else
   m_hostname = strdup(config->getSubEntryValue("Hostname", 0, "127.0.0.1"));
   m_pattern = strdup(config->getSubEntryValue("Topic", 0, "#"));
#endif
   m_port = (UINT16)config->getSubEntryValueAsUInt(_T("Port"), 0, 1883);;
   m_loopThread = INVALID_THREAD_HANDLE;

   char clientId[128];
   strcpy(clientId, "nxlora/");
   char *guid = uuid::generate().toString().getUTF8String();
   strcat(clientId, guid);
   free(guid);
   m_handle = mosquitto_new(clientId, true, this);
   m_messageHandler = NULL;
}

/**
 * Simple MQTT client destructor
 */
MqttClient::~MqttClient()
{
   if (m_loopThread != INVALID_THREAD_HANDLE)
      ThreadJoin(m_loopThread);
   if (m_handle != NULL)
      mosquitto_destroy(m_handle);
   free(m_hostname);
   free(m_pattern);
}

/**
 * MQTT Network loop
 */
void MqttClient::networkLoop()
{
   while(mosquitto_connect(m_handle, m_hostname, m_port, 120) != MOSQ_ERR_SUCCESS)
   {
      nxlog_debug(4, _T("LoraWAN Module: MQTT unable to connect to broker at %hs:%d, will retry in 60 seconds"), m_hostname, m_port);
      if (AgentSleepAndCheckForShutdown(60000))
         return; // Server shutdown
   }
   nxlog_debug(3, _T("LoraWAN Module: MQTT connected to broker %hs:%d"), m_hostname, m_port);

   if (mosquitto_subscribe(m_handle, NULL, m_pattern, 0) == MOSQ_ERR_SUCCESS)
   {
      nxlog_debug(4, _T("LoraWAN Module: MQTT subscribed to topic %hs on broker %hs:%d"), m_pattern, m_hostname, m_port);
      mosquitto_message_callback_set(m_handle, MqttClient::messageCallback);
      mosquitto_loop_forever(m_handle, -1, 1);
   }
   else
      nxlog_debug(4, _T("LoraWAN Module: MQTT cannot subscribe to topic %hs on broker %hs:%d"), m_pattern, m_hostname, m_port);
}

/**
 * MQTT Network loop starter
 */
THREAD_RESULT THREAD_CALL MqttClient::networkLoopStarter(void *arg)
{
   ((MqttClient *)arg)->networkLoop();
   return THREAD_OK;
}

/**
 * Start MQTT network loop
 */
void MqttClient::startNetworkLoop()
{
   m_loopThread = ThreadCreateEx(MqttClient::networkLoopStarter, 0, this);
}

/**
 * Stop MQTT network loop
 */
void MqttClient::stopNetworkLoop()
{
   mosquitto_disconnect(m_handle);
   ThreadJoin(m_loopThread);
   m_loopThread = INVALID_THREAD_HANDLE;
   mosquitto_lib_cleanup();
}

/**
 * Set MQTT message handler
 */
void MqttClient::setMessageHandler(void (*messageHandler)(const char *, char *))
{
   m_messageHandler = messageHandler;
}

/**
 * MQTT message callback
 */
void MqttClient::messageCallback(struct mosquitto *mosq, void *userData, const struct mosquitto_message *msg)
{
   if (msg->payloadlen <= 0)
      return;

   nxlog_debug(6, _T("LoraWAN Module:[MQTTMessageCallback] message received: %hs=\"%hs\""), msg->topic, (const char *)msg->payload);
      ((MqttClient *)userData)->executeMessageHandler((const char *)msg->payload, msg->topic);
}
