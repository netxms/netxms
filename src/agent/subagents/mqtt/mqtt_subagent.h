/*
** NetXMS Tuxedo subagent
** Copyright (C) 2014-2017 Raden Solutions
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
   Mutex m_mutex;

public:
   Topic(const TCHAR *pattern, const TCHAR *event = NULL);
   ~Topic();

   const char *getPattern() const { return m_pattern; }
   time_t getTimestamp() const { return m_timestamp; }

   void processMessage(const char *topic, const char *msg);
   bool retrieveData(TCHAR *buffer, size_t bufferLen);
};

/**
 * Broker definition
 */
class MqttBroker
{
private:
   char *m_hostname;
   UINT16 m_port;
   TCHAR *m_login;
   TCHAR *m_password;
   ObjectArray<Topic> m_topics;
   struct mosquitto *m_handle;
   THREAD m_loopThread;
   
   MqttBroker();

   static void messageCallback(struct mosquitto *handle, void *userData, const struct mosquitto_message *msg);
   void processMessage(const struct mosquitto_message *msg);

   void networkLoop();
   static THREAD_RESULT THREAD_CALL networkLoopStarter(void *arg);

public:
   ~MqttBroker();

   void startNetworkLoop();
   void stopNetworkLoop();

   static MqttBroker *createFromConfig(const ConfigEntry *e, StructArray<NETXMS_SUBAGENT_PARAM> *parameters);
};

#endif
