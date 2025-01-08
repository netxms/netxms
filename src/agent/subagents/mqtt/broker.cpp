/*
 ** MQTT subagent
 ** Copyright (C) 2017-2023 Raden Solutions
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
   nxlog_debug_tag(DEBUG_TAG, (level == MOSQ_LOG_DEBUG) ? 7 : 3, _T("libmosquitto: %hs"), message);
}

/**
 * Topic parameter handler
 */
static LONG H_TopicData(const TCHAR *name, const TCHAR *arg, TCHAR *result, AbstractCommSession *session)
{
   return ((Topic*)arg)->retrieveData(result, MAX_RESULT_LENGTH) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Topic structured parameter handler
 */
static LONG H_TopicStructuredData(const TCHAR *name, const TCHAR *arg, TCHAR *result, AbstractCommSession *session)
{
   return ((Topic*)arg)->retrieveData(name, result, MAX_RESULT_LENGTH);
}

/**
 * Topic structured list handler
 */
static LONG H_TopicStructuredListData(const TCHAR *name, const TCHAR *arg, StringList *result, AbstractCommSession *session)
{
   return ((Topic*)arg)->retrieveListData(name, result);
}

/**
 * Broker constructor
 */
MqttBroker::MqttBroker(const uuid& guid, const TCHAR *name) : m_topics(16, 16, Ownership::True), m_topicLock(MutexType::FAST)
{
   m_guid = guid;
   m_name = UTF8StringFromTString(name);
   m_locallyConfigured = true;
   m_hostname = nullptr;
   m_port = 0;
   m_login = nullptr;
   m_password = nullptr;
   m_loopThread = INVALID_THREAD_HANDLE;
   m_connected = false;

   char clientId[128];
   strcpy(clientId, "nxagentd/");
   _uuid_to_stringA(m_guid, &clientId[9]);
   m_handle = mosquitto_new(clientId, true, this);
   if (m_handle != nullptr)
   {
#if HAVE_MOSQUITTO_THREADED_SET
      mosquitto_threaded_set(m_handle, true);
#endif
      mosquitto_log_callback_set(m_handle, LogCallback);
      mosquitto_message_callback_set(m_handle, MqttBroker::messageCallback);
   }
}

/**
 * Broker destructor
 */
MqttBroker::~MqttBroker()
{
   if (m_loopThread != INVALID_THREAD_HANDLE)
      ThreadJoin(m_loopThread);
   if (m_handle != nullptr)
      mosquitto_destroy(m_handle);
   MemFree(m_hostname);
   MemFree(m_login);
   MemFree(m_password);
   MemFree(m_name);
}

/**
 * Create broker object from configuration
 */
MqttBroker *MqttBroker::createFromConfig(const ConfigEntry *config, StructArray<NETXMS_SUBAGENT_PARAM> *parameters, StructArray<NETXMS_SUBAGENT_LIST> *lists)
{
   MqttBroker *broker = new MqttBroker(uuid::generate(), config->getName());
   if (broker->m_handle == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Cannot create MQTT client instance"));
      delete broker;
      return nullptr;
   }

#ifdef UNICODE
   broker->m_hostname = UTF8StringFromWideString(config->getSubEntryValue(L"Hostname", 0, L"127.0.0.1"));
#else
   broker->m_hostname = MemCopyStringA(config->getSubEntryValue("Hostname", 0, "127.0.0.1"));
#endif
   broker->m_port = static_cast<uint16_t>(config->getSubEntryValueAsUInt(_T("Port"), 0, 1883));
#ifdef UNICODE
   broker->m_login = UTF8StringFromWideString(config->getSubEntryValue(_T("Login")));
   broker->m_password = UTF8StringFromWideString(config->getSubEntryValue(_T("Password")));
#else
   broker->m_login = MemCopyStringA(config->getSubEntryValue(_T("Login")));
   broker->m_password = MemCopyStringA(config->getSubEntryValue(_T("Password")));
#endif

   const ConfigEntry *metricRoot = config->findEntry(_T("Metrics"));
   if (metricRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> metrics = metricRoot->getSubEntries(_T("*"));
      for(int i = 0; i < metrics->size(); i++)
      {
         ConfigEntry *e = metrics->get(i);
         Topic *t = new Topic(e->getValue(), nullptr);
         broker->m_topics.add(t);

         NETXMS_SUBAGENT_PARAM p;
         memset(&p, 0, sizeof(NETXMS_SUBAGENT_PARAM));
         _tcslcpy(p.name, e->getName(), MAX_PARAM_NAME);
         p.arg = (const TCHAR *)t;
         p.dataType = DCI_DT_STRING;
         p.handler = H_TopicData;
         _sntprintf(p.description, MAX_DB_STRING, _T("MQTT topic %hs"), t->getPattern());
         parameters->add(p);
      }
   }

   const ConfigEntry *extractorsRoot = config->findEntry(_T("Extractors"));
   if (extractorsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> extractors = extractorsRoot->getSubEntries(_T("*"));
      for(int i = 0; i < extractors->size(); i++)
      {
         ConfigEntry *extractor = extractors->get(i);
         const TCHAR *topic = extractor->getSubEntryValue(_T("Topic"));
         if (topic == nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("Skipping extractor without topic: %s"), extractor->getName());
            continue;
         }

         bool parseAsText = extractor->getSubEntryValueAsBoolean(_T("ForcePlainTextParser"), false);
         const TCHAR *description = extractor->getSubEntryValue(_T("Description"), 0, nullptr);

         Topic *t = new Topic(topic, extractor->getName(), parseAsText);
         broker->m_topics.add(t);

         NETXMS_SUBAGENT_PARAM p;
         memset(&p, 0, sizeof(NETXMS_SUBAGENT_PARAM));
         _tcslcpy(p.name, t->getGenericParameterName(), MAX_PARAM_NAME);
         p.arg = (const TCHAR *)t;
         p.dataType = DCI_DT_STRING;
         p.handler = H_TopicStructuredData;
         if (description != nullptr)
            _tcslcpy(p.description, description, MAX_DB_STRING);
         else
            _sntprintf(p.description, MAX_DB_STRING, _T("MQTT topic %s"), topic);
         parameters->add(&p);

         StringMap *metricDefenitions = new StringMap();
         const ConfigEntry *metricRoot = extractor->findEntry(_T("Metrics"));
         if (metricRoot != nullptr)
         {
            unique_ptr<ObjectArray<ConfigEntry>> metrics = metricRoot->getSubEntries(_T("*"));
            for(int i = 0; i < metrics->size(); i++)
            {
               ConfigEntry *e = metrics->get(i);
               String name = e->getName();
               if (name.endsWith(_T(".description")) || name.endsWith(_T(".dataType")))
               {
                  continue;
               }

               TCHAR tmp[512];
               _tcscpy(tmp, name);
               _tcscat(tmp, _T(".description"));
               const TCHAR *description = metricRoot->getSubEntryValue(tmp, 0 , _T(""));

               _tcscpy(tmp, name);
               _tcscat(tmp, _T(".dataType"));
               int dataType = TextToDataType(metricRoot->getSubEntryValue(tmp, 0 , _T("")));

               metricDefenitions->set(e->getName(), e->getValue());

               NETXMS_SUBAGENT_PARAM p;
               memset(&p, 0, sizeof(NETXMS_SUBAGENT_PARAM));
               _tcslcpy(p.name, e->getName(), MAX_PARAM_NAME);
               p.arg = (const TCHAR *)t;
               p.dataType = (dataType == -1) ? DCI_DT_STRING : dataType;
               p.handler = H_TopicStructuredData;
               if (description != nullptr)
                  _tcslcpy(p.description, description, MAX_DB_STRING);
               else
                  _sntprintf(p.description, MAX_DB_STRING, _T("MQTT topic %s"), topic);
               parameters->add(&p);
            }
         }
         t->setExtractors(metricDefenitions);

         NETXMS_SUBAGENT_LIST l;
         memset(&l, 0, sizeof(NETXMS_SUBAGENT_LIST));
         _tcslcpy(l.name, t->getGenericParameterName(), MAX_PARAM_NAME);
         l.arg = (const TCHAR *)t;
         l.handler = H_TopicStructuredListData;
         if (description != nullptr)
            _tcslcpy(l.description, description, MAX_DB_STRING);
         else
            _sntprintf(l.description, MAX_DB_STRING, _T("MQTT topic %s"), topic);
         lists->add(&l);

         StringMap *listDefenitions = new StringMap();
         metricRoot = extractor->findEntry(_T("Lists"));
         if (metricRoot != nullptr)
         {
            unique_ptr<ObjectArray<ConfigEntry>> metrics = metricRoot->getSubEntries(_T("*"));
            for(int i = 0; i < metrics->size(); i++)
            {
               ConfigEntry *e = metrics->get(i);
               String name = e->getName();
               if (name.endsWith(_T(".description")))
               {
                  continue;
               }

               TCHAR tmp[512];
               _tcscpy(tmp, name);
               _tcscat(tmp, _T(".description"));
               const TCHAR *description = metricRoot->getSubEntryValue(tmp, 0 , _T(""));

               listDefenitions->set(e->getName(), e->getValue());

               NETXMS_SUBAGENT_LIST l;
               memset(&l, 0, sizeof(NETXMS_SUBAGENT_LIST));
               _tcslcpy(l.name, e->getName(), MAX_PARAM_NAME);
               l.arg = (const TCHAR *)t;
               l.handler = H_TopicStructuredListData;
               if (description != nullptr)
                  _tcslcpy(l.description, description, MAX_DB_STRING);
               else
                  _sntprintf(l.description, MAX_DB_STRING, _T("MQTT topic %s"), topic);
               lists->add(&l);
            }
         }
         t->setListExtractors(listDefenitions);
      }
   }

   const ConfigEntry *eventRoot = config->findEntry(_T("Events"));
   if (eventRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> events = eventRoot->getSubEntries(_T("*"));
      for(int i = 0; i < events->size(); i++)
      {
         ConfigEntry *e = events->get(i);
         Topic *t = new Topic(e->getValue(), e->getName());
         broker->m_topics.add(t);
      }
   }
   return broker;
}

/**
 * Create broker object from NXCP message
 */
MqttBroker *MqttBroker::createFromMessage(const NXCPMessage *msg)
{
   uuid guid = msg->getFieldAsGUID(VID_GUID);
   if (guid.isNull())
      guid = uuid::generate();

   TCHAR name[128];
   msg->getFieldAsString(VID_NAME, name, 128);

   MqttBroker *broker = new MqttBroker(guid, name);
   if (broker->m_handle == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot create MQTT client instance"));
      delete broker;
      return nullptr;
   }

   broker->m_hostname = msg->getFieldAsUtf8String(VID_HOSTNAME);
   broker->m_port = msg->getFieldAsUInt16(VID_PORT);
   broker->m_login = msg->getFieldAsUtf8String(VID_LOGIN_NAME);
   if ((broker->m_login != nullptr) && (broker->m_login[0] != 0))
   {
      broker->m_password = msg->getFieldAsUtf8String(VID_PASSWORD);
   }
   else
   {
      MemFreeAndNull(broker->m_login);
   }
   return broker;
}

/**
 * Broker network loop
 */
void MqttBroker::networkLoop()
{
   mosquitto_username_pw_set(m_handle, m_login, m_password);

   while(mosquitto_connect(m_handle, m_hostname, m_port, 600) != MOSQ_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Unable to connect to MQTT broker at %hs:%d, will retry in 60 seconds"), m_hostname, (int)m_port);
      if (AgentSleepAndCheckForShutdown(60000))
         return;  // Agent shutdown
   }

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Connected to MQTT broker %hs:%d as %hs"), m_hostname, (int)m_port, (m_login != nullptr) ? m_login : "anonymous");
   m_connected = true;

   m_topicLock.lock();
   for(int i = 0; i < m_topics.size(); i++)
   {
      Topic *t = m_topics.get(i);
      if (mosquitto_subscribe(m_handle, nullptr, t->getPattern(), 0) == MOSQ_ERR_SUCCESS)
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Subscribed to topic %hs on broker %hs:%d"), t->getPattern(), m_hostname, (int)m_port);
      else
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot subscribe to topic %hs on MQTT broker %hs:%d"), t->getPattern(), m_hostname, (int)m_port);
   }
   m_topicLock.unlock();

   mosquitto_loop_forever(m_handle, -1, 1);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Network loop stopped for MQTT broker %hs:%d"), m_hostname, (int)m_port);
   m_connected = false;
}

/**
 * Start broker network loop
 */
void MqttBroker::startNetworkLoop()
{
   m_loopThread = ThreadCreateEx(this, &MqttBroker::networkLoop);
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
void MqttBroker::messageCallback(struct mosquitto *handle, void *context, const struct mosquitto_message *msg)
{
   static_cast<MqttBroker*>(context)->processMessage(msg);
}

/**
 * Process broker message
 */
void MqttBroker::processMessage(const struct mosquitto_message *msg)
{
   if (msg->payloadlen <= 0)
      return;  // NULL message

   nxlog_debug_tag(DEBUG_TAG, 6, _T("MQTT message received: %hs=\"%hs\""), msg->topic, static_cast<const char*>(msg->payload));
   m_topicLock.lock();
   for(int i = 0; i < m_topics.size(); i++)
   {
      m_topics.get(i)->processMessage(msg->topic, static_cast<const char*>(msg->payload));
   }
   m_topicLock.unlock();
}

/**
 * Get data for given topic
 */
LONG MqttBroker::getTopicData(const char *topicName, TCHAR *value, bool enableAutoRegistration)
{
   LONG rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   m_topicLock.lock();
   for(int i = 0; i < m_topics.size(); i++)
   {
      Topic *topic = m_topics.get(i);
      if (!stricmp(topic->getPattern(), topicName))
      {
         rc = topic->retrieveData(value, MAX_RESULT_LENGTH) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
         break;
      }
   }
   if ((rc == SYSINFO_RC_NO_SUCH_INSTANCE) && enableAutoRegistration)
   {
      if (mosquitto_subscribe(m_handle, nullptr, topicName, 0) == MOSQ_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Subscribed to topic %hs on MQTT broker %hs:%d"), topicName, m_hostname, (int)m_port);
         m_topics.add(new Topic(topicName));
         rc = SYSINFO_RC_ERROR;
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot subscribe to topic %hs on MQT broker %hs:%d"), topicName, m_hostname, (int)m_port);
      }
   }
   m_topicLock.unlock();
   return rc;
}
