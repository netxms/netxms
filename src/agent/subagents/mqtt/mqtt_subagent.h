/*
** NetXMS Tuxedo subagent
** Copyright (C) 2014-2025 Raden Solutions
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
** File: mqtt_subagent.h
**
**/

#ifndef _mqtt_subagent_h_
#define _mqtt_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <mosquitto.h>
#include <nxsde.h>

#define DEBUG_TAG _T("mqtt")

/**
 * Topic definition
 */
class Topic
{
private:
   char *m_pattern;
   char m_lastName[MAX_DB_STRING];
   char m_lastValue[MAX_RESULT_LENGTH];
   time_t m_timestamp;
   TCHAR *m_event;
   TCHAR m_genericParamName[MAX_PARAM_NAME];
   StringMap *m_parameters;
   StringMap *m_lists;
   StructuredDataExtractor *m_dataExtractor;
   bool m_parseAsText;
   Mutex m_mutex;

public:
   Topic(const TCHAR *pattern, const TCHAR *event);
   Topic(const char *pattern);
   Topic(const TCHAR *pattern, const TCHAR *name, bool parseAsText);
   ~Topic();

   const char *getPattern() const { return m_pattern; }
   time_t getTimestamp() const { return m_timestamp; }
   const TCHAR *getGenericParameterName() { return m_genericParamName; }

   void setExtractors(StringMap *parameters) { m_parameters = parameters; }
   void setListExtractors(StringMap *lists) { m_lists = lists; }

   void processMessage(const char *topic, const char *msg);
   bool retrieveData(TCHAR *buffer, size_t bufferLen);
   LONG retrieveData(const TCHAR *metricName, TCHAR *buffer, size_t bufferLen);
   LONG retrieveListData(const TCHAR *metricName, StringList *buffer);
};

/**
 * Broker definition
 */
class MqttBroker
{
private:
   uuid m_guid;
   char *m_name;
   bool m_locallyConfigured;
   char *m_hostname;
   uint16_t m_port;
   char *m_login;
   char *m_password;
   ObjectArray<Topic> m_topics;
   Mutex m_topicLock;
   struct mosquitto *m_handle;
   THREAD m_loopThread;
   bool m_connected;

   MqttBroker(const uuid& guid, const TCHAR *name);

   static void messageCallback(struct mosquitto *handle, void *context, const struct mosquitto_message *msg);
   void processMessage(const struct mosquitto_message *msg);

   void networkLoop();

public:
   ~MqttBroker();

   void startNetworkLoop();
   void stopNetworkLoop();

   const uuid& getGuid() const { return m_guid; }
   const char *getName() const { return m_name; }
   bool isLocallyConfigured() const { return m_locallyConfigured; }
   const char *getHostname() const { return m_hostname; }
   uint16_t getPort() const { return m_port; }
   const char *getLogin() const { return m_login; }
   int getTopicCount() const { return m_topics.size(); }
   bool isConnected() const { return m_connected; }

   LONG getTopicData(const char *topicName, TCHAR *value, bool enableAutoRegistration);

   static MqttBroker *createFromConfig(const ConfigEntry *e, StructArray<NETXMS_SUBAGENT_PARAM> *parameters, StructArray<NETXMS_SUBAGENT_LIST> *lists);
   static MqttBroker *createFromMessage(const NXCPMessage *msg);
};

#endif
