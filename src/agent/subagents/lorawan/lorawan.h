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

#include <nms_agent.h>
#include <nms_util.h>
#include <mosquitto.h>
#include <curl/curl.h>
#include <nxmbapi.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif   /* _WIN32 */

/**
 * Constants
 */
#define MAX_AUTH_LENGTH     1024

/**
 * LoraWAN device map
 */
extern HashMap<uuid, LoraDeviceData> g_deviceMap;

/**
 * Device map mutex
 */
extern MUTEX g_deviceMapMutex;

/**
 * Simple MQTT client definition
 */
class MqttClient
{
private:
   char *m_hostname;
   UINT16 m_port;
   char *m_pattern;
   THREAD m_loopThread;
   struct mosquitto *m_handle;
   void (*m_messageHandler)(const char *, char *);

   void networkLoop();
   static THREAD_RESULT THREAD_CALL networkLoopStarter(void *arg);
   static void messageCallback(struct mosquitto *mosq, void *userData, const struct mosquitto_message *msg);
   void executeMessageHandler(const char *payload, char *topic) { m_messageHandler(payload, topic); }

public:
   MqttClient(const ConfigEntry *config);
   ~MqttClient();

   void setMessageHandler(void (*messageHandler)(const char *, char *));
   void startNetworkLoop();
   void stopNetworkLoop();
};

/**
 * LoraWAN server link definition
 */
class LoraWanServerLink
{
private:
   char *m_url;
   char *m_app;
   char *m_appId;
   char *m_region;
   bool m_adr;
   UINT32 m_fcntCheck;
   char m_auth[MAX_AUTH_LENGTH];
   char m_errorBuffer[CURL_ERROR_SIZE];
   CURL *m_curl;
   long m_response;
   MUTEX m_curlHandleMutex;

   UINT32 sendRequest(const char *method, const char *url, const char *responseData = NULL, const curl_slist *headers = NULL, char *postFields = NULL);

public:
   LoraWanServerLink(const ConfigEntry *config);
   ~LoraWanServerLink();

   UINT32 registerDevice(NXCPMessage *request);
   UINT32 deleteDevice(uuid guid);

   bool connect();
   void disconnect();
};

/**
 * Communication processor functions
 */
void MqttMessageHandler(const char *payload, char *topic);

LONG H_Communication(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Device map functions
 */
UINT32 AddDevice(LoraDeviceData *data);
UINT32 RemoveDevice(uuid guid);
LoraDeviceData *FindDevice(uuid guid);
