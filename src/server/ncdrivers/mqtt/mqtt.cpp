/* 
** NetXMS - Network Management System
** Notification channel driver for MQTT
** Copyright (C) 2014-2022 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: mqtt.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <mosquitto.h>
#include <netxms-version.h>

#define DEBUG_TAG _T("ncd.mqtt")

static const NCConfigurationTemplate s_config(false, true);

/**
 * Flags
 */
#define MST_USE_CARDS   0x0001

/**
 * Microsoft Teams driver class
 */
class MQTTDriver : public NCDriver
{
private:
   char m_hostname[128];
   uint16_t m_port;
   char m_login[128];
   char m_password[128];
   uuid_t m_guid;
   mosquitto *m_handle;

   bool checkHandle();

public:
   MQTTDriver(Config *config);
   virtual ~MQTTDriver();

   virtual int send(const char* recipient, const char* subject, const char* body) override;
};

/**
 * Create driver instance
 */
MQTTDriver::MQTTDriver(Config *config) : NCDriver()
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new driver instance"));

   strcpy(m_hostname, "127.0.0.1");
   m_port = 1883;
   m_login[0] = 0;
   m_password[0] = 0;

   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("Hostname"), CT_UTF8_STRING, 0, 0, sizeof(m_hostname), 0, m_hostname },
      { _T("Login"), CT_UTF8_STRING, 0, 0, sizeof(m_login), 0, m_login },
      { _T("Password"), CT_UTF8_STRING, 0, 0, sizeof(m_password), 0, m_password },
      { _T("Port"), CT_WORD, 0, 0, 0, 0, &m_port },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };
   config->parseTemplate(_T("MQTT"), configTemplate);
   DecryptPasswordA(m_login, m_password, m_password, sizeof(m_password));

   _uuid_generate(m_guid);
   m_handle = nullptr;
}

/**
 * Driver destructor
 */
MQTTDriver::~MQTTDriver()
{
   if (m_handle != nullptr)
      mosquitto_destroy(m_handle);
}

/**
 * Log callback
 */
static void LogCallback(struct mosquitto *handle, void *userData, int level, const char *message)
{
   nxlog_debug_tag(DEBUG_TAG, (level == MOSQ_LOG_DEBUG) ? 7 : 3, _T("MQTT: %hs"), message);
}

/**
 * Check libmosquitto handle
 */
bool MQTTDriver::checkHandle()
{
   if (m_handle != nullptr)
      return true;

   char clientId[128];
   strcpy(clientId, "netxms/");
   _uuid_to_stringA(m_guid, &clientId[7]);

   m_handle = mosquitto_new(clientId, true, this);
   if (m_handle == nullptr)
      return false;

#if HAVE_MOSQUITTO_THREADED_SET
   mosquitto_threaded_set(m_handle, true);
#endif
   mosquitto_log_callback_set(m_handle, LogCallback);

   mosquitto_username_pw_set(m_handle, (m_login[0] != 0) ? m_login : nullptr, (m_login[0] != 0) ? m_password : nullptr);

   return true;
}

/**
 * Send notification
 */
int MQTTDriver::send(const char* recipient, const char* subject, const char* body)
{
   if (!checkHandle())
      return -1;

   if (mosquitto_connect(m_handle, m_hostname, m_port, 600) != MOSQ_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Unable to connect to MQTT broker at %hs:%d"), m_hostname, m_port);
      return 30;  // Retry in 30 seconds
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Connected to MQTT broker %hs:%d as %hs"), m_hostname, m_port, (m_login[0] != 0) ? m_login : "anonymous");

   const char *topic = CHECK_NULL_EX_A(recipient);
   const char *payload = CHECK_NULL_EX_A(body);

   int result;
   int rc = mosquitto_publish(m_handle, nullptr, topic, static_cast<int>(strlen(payload)), payload, 0, false);
   if (rc == MOSQ_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Successfully published via %hs:%d in topic %hs: \"%hs\""), m_hostname, m_port, topic, payload);
      result = 0;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Cannot publish message via %hs:%d in topic %hs (libmosquitto error %d)"), m_hostname, m_port, topic, rc);
      result = -1;
   }

   mosquitto_loop(m_handle, 100, 1);

   mosquitto_disconnect(m_handle);
   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(MQTT, &s_config)
{
   static VolatileCounter reentryGuarg = 0;
   static volatile bool initialized = false;
   if (InterlockedIncrement(&reentryGuarg) == 1)
   {
      mosquitto_lib_init();
      initialized = true;
   }
   else
   {
      while(!initialized)
         ThreadSleepMs(100);
   }

   return new MQTTDriver(config);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
