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
 * LoraWAN server link constructor
 */
LoraWanServerLink::LoraWanServerLink(const ConfigEntry *config)
{
   char *m_user;
   char *m_pass;
#ifdef UNICODE
   m_user = UTF8StringFromWideString(config->getSubEntryValue(L"User", 0, L"admin"));
   m_pass = UTF8StringFromWideString(config->getSubEntryValue(L"Password", 0, L"admin"));
   m_url = UTF8StringFromWideString(config->getSubEntryValue(L"URL", 0, L"http://localhost"));
   m_app = UTF8StringFromWideString(config->getSubEntryValue(L"Application", 0, L"backend"));
   m_appId = UTF8StringFromWideString(config->getSubEntryValue(L"ApplicationId", 0, L"LoraWAN Devices"));
   m_region = UTF8StringFromWideString(config->getSubEntryValue(L"Region", 0, L"EU863-870"));
#else
   m_user = strdup(config->getSubEntryValue("User", 0, "admin"));
   m_pass = strdup(config->getSubEntryValue("Password", 0, "admin"));
   m_url =  strdup(config->getSubEntryValue("URL", 0, "http://localhost"));
   m_app = strdup(config->getSubEntryValue("Application", 0, "backend"));
   m_appId = strdup(config->getSubEntryValue("ApplicationId", 0, "LoraWAN Devices"));
   m_region = strdup(config->getSubEntryValue("Region", 0, "EU863-870"));
#endif
   m_adr = config->getSubEntryValueAsBoolean(_T("ADR"), 0, true);
   m_fcntCheck = config->getSubEntryValueAsUInt(_T("FcntCheck"), 0, 3);
   m_response = 0;

   snprintf(m_auth, MAX_AUTH_LENGTH, "%s:%s", m_user, m_pass);
   m_curl = NULL;
   m_curlHandleMutex = MutexCreate();

   free(m_user);
   free(m_pass);
}

/*
 * LoraWAN server link destructor
 */
LoraWanServerLink::~LoraWanServerLink()
{
   disconnect();
   curl_global_cleanup();
   MutexDestroy(m_curlHandleMutex);
   free(m_url);
   free(m_app);
   free(m_appId);
   free(m_region);
}

/**
 * Send cURL request
 */
UINT32 LoraWanServerLink::sendRequest(const char *method, const char *url, const char *responseData, const curl_slist *headers, char *postFields)
{
   MutexLock(m_curlHandleMutex);
   curl_easy_setopt(m_curl, CURLOPT_URL, url);
   curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, method);
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, responseData);
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postFields);

   UINT32 rcc = curl_easy_perform(m_curl);
   if (rcc == CURLE_OK)
   {
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &m_response);
      nxlog_debug(7, _T("LoraWAN Module: LoraWAN server request - URL: %hs, Method: %hs, Response: %03d"), url, method, m_response);
   }
   else
      nxlog_debug(7, _T("LoraWAN Module: call to curl_easy_perform() failed: %hs"), m_errorBuffer);

   MutexUnlock(m_curlHandleMutex);
   return rcc;
}

/**
 * Connect to LoraWAN server
 */
bool LoraWanServerLink::connect()
{
   disconnect();

   bool rcc = false;
   curl_global_init(CURL_GLOBAL_ALL);
   m_curl = curl_easy_init();
   if (m_curl == NULL)
   {
      nxlog_debug(4, _T("LoraWAN Module: call to curl_easy_init() failed"));
      return rcc;
   }

   curl_easy_setopt(m_curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
   curl_easy_setopt(m_curl, CURLOPT_USERPWD, m_auth);
   curl_easy_setopt(m_curl, CURLOPT_URL, m_url);
   curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorBuffer);

   if (sendRequest("OPTIONS", m_url) == CURLE_OK)
   {
      if (m_response == 200)
      {
         nxlog_debug(4, _T("LoraWAN Module: LoraWAN server login successful"));
         rcc = true;
      }
      else
         nxlog_debug(4, _T("LoraWAN Module: LoraWAN server login failed, HTTP response code %03d"), m_response);
   }

   return rcc;
}

/**
 * Disconnect from LoraWAN server
 */
void LoraWanServerLink::disconnect()
{
   if (m_curl == NULL)
      return;

   curl_easy_cleanup(m_curl);
   m_curl = NULL;
}

/**
 * Register new LoraWAN device
 */
UINT32 LoraWanServerLink::registerDevice(NXCPMessage *request)
{
   LoraDeviceData *data = new LoraDeviceData(request);

   json_t *root = json_object();
   json_object_set_new(root, "adr_flag_set", json_integer(m_adr ? 1 : 0));
   json_object_set_new(root, "app", json_string(m_app));
   json_object_set_new(root, "appid", json_string(m_appId));
   json_object_set_new(root, "can_join", json_true());
   json_object_set_new(root, "fcnt_check", json_integer(m_fcntCheck));
   json_object_set_new(root, "region", json_string(m_region));
   json_object_set_new(root, "appargs", data->getGuid().toJson());
   json_object_set_new(root, "txwin", 0);

   char url[MAX_PATH];
   strcpy(url, m_url);
   if (data->isOtaa()) // OTAA
   {
      TCHAR appEui[17];
      TCHAR appKey[33];
      request->getFieldAsString(VID_LORA_APP_EUI, appEui, 17);
      request->getFieldAsString(VID_LORA_APP_KEY, appKey, 33);
      nxlog_debug(4, _T("LoraWAN Module: Config appEui %s"), appEui);
      nxlog_debug(4, _T("LoraWAN Module: Config appKey %s"), appKey);
      json_object_set_new(root, "deveui", json_string_t((const TCHAR*)data->getDevEui().toString(MAC_ADDR_FLAT_STRING)));
      json_object_set_new(root, "appeui", json_string_t(appEui));
      json_object_set_new(root, "appkey", json_string_t(appKey));
      strcat(url, "/devices");
   }
   else  // ABP
   {
      TCHAR appSKey[33];
      TCHAR nwkSKey[33];
      request->getFieldAsString(VID_LORA_APP_S_KEY, appSKey, 33);
      request->getFieldAsString(VID_LORA_NWK_S_KWY, nwkSKey, 33);
      json_object_set_new(root, "devaddr", json_string_t((const TCHAR*)data->getDevAddr().toString(MAC_ADDR_FLAT_STRING)));
      json_object_set_new(root, "appskey", json_string_t(appSKey));
      json_object_set_new(root, "nwkskey", json_string_t(nwkSKey));
      strcat(url, "/nodes");
   }

   char *jsonData = json_dumps(root, 0);
   struct curl_slist *headers = NULL;
   headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");

   UINT32 rcc;
   if (sendRequest("POST", url, NULL, headers, jsonData) == CURLE_OK)
   {
      if (m_response == 204)
      {
         nxlog_debug(4, _T("LoraWAN Module: New LoraWAN device successfully registered"));
         rcc = AddDevice(data);
      }
      else
      {
         nxlog_debug(4, _T("LoraWAN Module: LoraWAN device registration failed, HTTP response code %03d"), m_response);
         rcc = ERR_BAD_RESPONSE;
      }
   }
   else
      rcc = ERR_EXEC_FAILED;

   free(root);
   free(jsonData);

   return rcc;
}

/**
 * Delete LoraWAN device
 */
UINT32 LoraWanServerLink::deleteDevice(uuid guid)
{
   UINT32 rcc = ERR_INVALID_OBJECT;
   LoraDeviceData *data = FindDevice(guid);
   if (data == NULL)
      return rcc;

   char url[MAX_PATH];
   char *addr = NULL;
   if (data->isOtaa())  // if OTAA
   {
#ifdef UNICODE
      addr = UTF8StringFromWideString((const TCHAR*)data->getDevEui().toString(MAC_ADDR_FLAT_STRING));
      snprintf(url, MAX_PATH, "%s/devices/%s", m_url, addr);
      free(addr);
#else
      snprintf(url, MAX_PATH, "%s/devices/%s", m_url, (const TCHAR*)data->getDevEui().toString(MAC_ADDR_FLAT_STRING));
#endif
      if (sendRequest("DELETE", url) == CURLE_OK)
      {
         curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &m_response);
         if (m_response == 204)
         {
            nxlog_debug(4, _T("LoraWAN Module: New LoraWAN device successfully deleted"));
         }
         else
         {
            nxlog_debug(4, _T("LoraWAN Module: LoraWAN device deletion failed, HTTP response code %03d"), m_response);
            rcc = ERR_BAD_RESPONSE;
         }
      }
   }
   if (data->getDevAddr().length() != 0)
   {
#ifdef UNICODE
      addr = UTF8StringFromWideString((const TCHAR *)data->getDevAddr().toString(MAC_ADDR_FLAT_STRING));
      snprintf(url, MAX_PATH, "%s/nodes/%s", m_url, addr);
      free(addr);
#else
      snprintf(url, MAX_PATH, "%s/nodes/%s", m_url, (const TCHAR *)data->getDevAddr().toString(MAC_ADDR_FLAT_STRING));
#endif
      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Accept: application/json");
      char *responseData;
      if (sendRequest("GET", url, responseData, headers) == CURLE_OK)
      {
         if (m_response == 200)
         {
            if (sendRequest("DELETE", url) == CURLE_OK)
            {
               if (m_response == 204)
                  nxlog_debug(4, _T("LoraWAN Module: New LoraWAN node successfully deleted"));
               else
               {
                  nxlog_debug(4, _T("LoraWAN Module: LoraWAN node deletion failed, HTTP response code %03d"), m_response);
                  rcc = ERR_BAD_RESPONSE;
               }
            }
         }
         else
            nxlog_debug(4, _T("LoraWAN Module: LoraWAN node deletion failed, HTTP response code %03d"), m_response);
      }
      else
         rcc = ERR_EXEC_FAILED;

      free(responseData);
   }
   if (m_response == 204)
      rcc = RemoveDevice(data);

   return rcc;
}
