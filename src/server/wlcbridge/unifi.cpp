/*
** NetXMS - Network Management System
** Copyright (C) 2023-2026 Raden Solutions
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
** File: unifi.cpp
**
**/

#include "wlcbridge.h"
#include <nddrv.h>
#include <map>

#define DEBUG_TAG WLCBRIDGE_DEBUG_TAG _T(".unifi")

#define MAX_AUTH_TOKEN_SIZE   512
#define MAX_READER_BATCH_SIZE 64
#define MAX_COOKIE_SIZE       1024

/**
 * Get custom attribute from domain object as UTF-8 sting
 */
static std::string GetDomainAttribute(NObject *wirelessDomain, const TCHAR *name)
{
   TCHAR buffer[1024];
   if (wirelessDomain->getCustomAttribute(name, buffer, 1024) == nullptr)
      return std::string();

   char utf8buffer[1024];
   wchar_to_utf8(buffer, -1, utf8buffer, 1024);
   return std::string(utf8buffer);
}

/**
 * Callback for processing headers received from cURL
 */
static size_t OnCurlHeaderReceived(char *ptr, size_t size, size_t nmemb, void *context)
{
   size_t bytes = size * nmemb;
   if ((bytes > 12) && !strncasecmp(ptr, "Set-Cookie:", 11))
   {
      char *p = ptr + 11;
      while ((*p != 0) && isspace(static_cast<unsigned char>(*p)))
         p++;

      char *end = p;
      while ((*end != 0) && (*end != ';') && (*end != '\r') && (*end != '\n'))
         end++;

      if (end > p)
      {
         char *cookie = static_cast<char*>(context);
         size_t existing = strlen(cookie);
         if (existing + 2 < MAX_COOKIE_SIZE)
         {
            if (existing > 0)
            {
               cookie[existing++] = ';';
               cookie[existing++] = ' ';
               cookie[existing] = 0;
            }
            size_t len = static_cast<size_t>(end - p);
            if (existing + len + 1 < MAX_COOKIE_SIZE)
            {
               memcpy(cookie + existing, p, len);
               cookie[existing + len] = 0;
            }
         }
      }
   }
   return bytes;
}

/**
 * Login to controller at given address
 */
static bool Login(const char *baseUrl, const char *login, const char *password, const bool isAppliance, char *cookie)
{
   ByteStream responseData(2048);
   responseData.setAllocationStep(2048);

   char errorBuffer[CURL_ERROR_SIZE];

   CURL *curl = CreateCurlHandle(&responseData, errorBuffer);
   if (curl == nullptr)
      return false;

   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
   curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); // enable cookie engine
   curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &OnCurlHeaderReceived);
   curl_easy_setopt(curl, CURLOPT_HEADERDATA, cookie);

   json_t *request = json_object();
   json_object_set_new(request, "username", json_string(login));
   json_object_set_new(request, "password", json_string(password));
   if (!isAppliance)
      json_object_set_new(request, "rememberMe", json_true());

   char *data = json_dumps(request, 0);
   json_decref(request);
   curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
   curl_easy_setopt(curl, CURLOPT_POST, 1L);
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Content-Type: application/json");
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   bool success = true;

   char url[256];
   if (isAppliance)
      snprintf(url, 256, "%s/api/login", baseUrl);
   else
      snprintf(url, 256, "%s/api/auth/login", baseUrl);

   if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
      success = false;
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 9, _T("Sending login request '%hs' to %hs"), data, url);
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errorBuffer);
         success = false;
      }
   }

   if (success)
   {
      long httpCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

      if (httpCode != 200)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Error response from controller: HTTP response code is %d"), httpCode);
         success = false;
      }
   }
   if (success && ((cookie == nullptr) || (cookie[0] == 0)))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Login did not return cookie"));
      success = false;
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(data);

   return success;
}

/**
 * Read JSON document from controller
 */
static json_t *ReadJsonFromController(NObject *wirelessDomain, const char *url, const char *token, const char *cookie, const char *requestData = nullptr)
{
   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);

   char errorBuffer[CURL_ERROR_SIZE];

   CURL *curl = CreateCurlHandle(&responseData, errorBuffer);
   if (curl == nullptr)
      return nullptr;

   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
   if ((cookie != nullptr) && (cookie[0] != 0))
      curl_easy_setopt(curl, CURLOPT_COOKIE, cookie);

   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Content-Type: application/json");
   if(token != nullptr)
      headers = curl_slist_append(headers, token);
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   if (requestData != nullptr)
   {
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData);
   }
   else
   {
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
   }

   bool success = true;

   // HTTP method set above based on request body
   if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
      success = false;
   }

   if (success)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Call to curl_easy_perform(%hs) failed (%d: %hs)"), url, rc, errorBuffer);
         success = false;
      }
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes from %hs"), static_cast<int>(responseData.size()), url);
      long httpCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Response from controller: HTTP response code is %d"), httpCode);
      if (httpCode != 200)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Error response from controller: HTTP response code is %d"), httpCode);
         if ((httpCode == 401) && (wirelessDomain != nullptr))
         {
            wirelessDomain->setCustomAttribute(_T("$unifi.cookie"), SharedString(), StateChange::CLEAR);
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Authorization failed (401), cleared unifi cookie"));
         }
         success = false;
      }
   }

   if (success && responseData.size() <= 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Empty response from controller"));
      success = false;
   }

   json_t *json = nullptr;
   if (success)
   {
      responseData.write('\0');
      const char *data = reinterpret_cast<const char*>(responseData.buffer());
      json_error_t error;
      json = json_loads(data, 0, &error);
      if (json == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON on line %d: %hs"), error.line, error.text);
         success = false;
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   return json;
}

/**
 * Do request from controller
 */
static json_t *DoRequest(NObject *wirelessDomain, const char *endpoint, const char *requestData = nullptr)
{
   if (IsShutdownInProgress())
      return nullptr;

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Sending request to controller for domain %s [%u]: endpoint=%hs, data=%hs"),
      wirelessDomain->getName(), wirelessDomain->getId(), endpoint, CHECK_NULL_A(requestData));

   std::string baseUrl = GetDomainAttribute(wirelessDomain, _T("unifi.base-url"));
   if (baseUrl.length() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Controller base URL not provided"));
      return nullptr;
   }

   // Normalize base URL: trim whitespace, ensure scheme, remove trailing slash
   baseUrl.erase(0, baseUrl.find_first_not_of(" \t\r\n"));
   baseUrl.erase(baseUrl.find_last_not_of(" \t\r\n") + 1);
   if ((baseUrl.rfind("http://", 0) != 0) && (baseUrl.rfind("https://", 0) != 0))
      baseUrl = "https://" + baseUrl;
   while (!baseUrl.empty() && baseUrl.back() == '/')
      baseUrl.pop_back();

   std::string site = GetDomainAttribute(wirelessDomain, _T("unifi.site"));
   if (site.empty())
      site = "default";

   auto attrIsTrue = [&](const TCHAR *name) -> bool
   {
      std::string v = GetDomainAttribute(wirelessDomain, name);
      if (v.empty())
         return false;
      for (auto &c : v)
         c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
      return (v == "1") || (v == "true") || (v == "yes") || (v == "on");
   };

   bool isAppliance = attrIsTrue(_T("unifi.isAppliance"));

   char token[MAX_AUTH_TOKEN_SIZE + 12];
   std::string tk = GetDomainAttribute(wirelessDomain, _T("unifi.token"));
   std::string cookieAttr = GetDomainAttribute(wirelessDomain, _T("$unifi.cookie"));
   if (tk.length() == 0)
   {
      if (cookieAttr.empty())
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Controller API token not provided. Attempting to login."));
         char cookieBuf[MAX_COOKIE_SIZE];
         cookieBuf[0] = 0;
         if (!Login(baseUrl.c_str(), GetDomainAttribute(wirelessDomain, _T("unifi.login")).c_str(), GetDomainAttribute(wirelessDomain, _T("unifi.password")).c_str(), isAppliance, cookieBuf))
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("Login to controller at %hs failed"), baseUrl.c_str());
            return nullptr;
         }
         wirelessDomain->setCustomAttribute(_T("$unifi.cookie"), SharedString(cookieBuf, "ASCII"), StateChange::CLEAR);
         cookieAttr = cookieBuf;
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Login to controller at %hs successful"), baseUrl.c_str());
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Controller API token not provided. Using cached cookie."));
      }
   }
   else
   {
      snprintf(token, MAX_AUTH_TOKEN_SIZE + 12, "X-API-KEY: %s", tk.c_str());
   }

   char url[256];
   if (isAppliance)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Controller is an appliance"));
      snprintf(url, 256, "%s/api/s/%s/%s", baseUrl.c_str(), site.c_str(), endpoint);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Controller is an not appliance"));
      snprintf(url, 256, "%s/proxy/network/api/s/%s/%s", baseUrl.c_str(), site.c_str(), endpoint);
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("UniFi request: url=%hs"), url);

   const char *cookie = cookieAttr.empty() ? nullptr : cookieAttr.c_str();
   json_t *response = ReadJsonFromController(wirelessDomain, url, tk.length() != 0 ? token : nullptr, cookie, requestData);
   if (response == nullptr)
      return nullptr;

   return response;
}

/**
 * Parse radio index from radio name (e.g. wifi1 -> 1)
 */
static uint32_t ParseRadioIndex(const char *name)
{
   if (name == nullptr)
      return 0;

   const char *p = name + strlen(name);
   while ((p > name) && isdigit(static_cast<unsigned char>(*(p - 1))))
      p--;
   if (*p == 0)
      return 0;

   return strtoul(p, nullptr, 10);
}

/**
 * Build radio interface list from device JSON
 */
static void AddRadioInterfaces(AccessPointInfo *ap, json_t *device)
{
   if (ap == nullptr || device == nullptr)
      return;

   std::map<std::string, uint32_t> radioIndex;
   std::map<std::string, uint16_t> radioChannel;

   json_t *radioTable = json_object_get(device, "radio_table");
   if (json_is_array(radioTable))
   {
      size_t i;
      json_t *radio;
      json_array_foreach(radioTable, i, radio)
      {
         const char *name = json_object_get_string_utf8(radio, "name", nullptr);
         if (name == nullptr)
            continue;
         uint16_t channel = static_cast<uint16_t>(json_object_get_int32(radio, "channel", 0));
         radioChannel[name] = channel;
      }
   }

   json_t *vaps = json_object_get(device, "vap_table");
   if (!json_is_array(vaps))
      return;

   size_t i;
   json_t *vap;
   json_array_foreach(vaps, i, vap)
   {
      const char *bssidText = json_object_get_string_utf8(vap, "bssid", nullptr);
      if (bssidText == nullptr)
         continue;

      MacAddress bssid = MacAddress::parse(bssidText);
      if (!bssid.isValid() || (bssid.length() != MAC_ADDR_LENGTH))
         continue;

      const char *ssidText = json_object_get_string_utf8(vap, "essid", nullptr);
      if (ssidText == nullptr)
         ssidText = json_object_get_string_utf8(vap, "ssid", "");

      const char *radioName = json_object_get_string_utf8(vap, "radio_name", nullptr);
      if (radioName == nullptr)
         radioName = json_object_get_string_utf8(vap, "radio", "radio");

      uint32_t idx;
      auto it = radioIndex.find(radioName);
      if (it == radioIndex.end())
      {
         idx = static_cast<uint32_t>(radioIndex.size()) + 1;
         radioIndex[radioName] = idx;
      }
      else
      {
         idx = it->second;
      }

      RadioInterfaceInfo rif;
      memset(&rif, 0, sizeof(rif));
      rif.index = idx;
      rif.ifIndex = idx;
      utf8_to_tchar(radioName, -1, rif.name, MAX_OBJECT_NAME);
      memcpy(rif.bssid, bssid.value(), MAC_ADDR_LENGTH);
      utf8_to_tchar(ssidText, -1, rif.ssid, MAX_SSID_LENGTH);

      uint16_t channel = static_cast<uint16_t>(json_object_get_int32(vap, "channel", 0));
      if (channel == 0)
      {
         auto c = radioChannel.find(radioName);
         if (c != radioChannel.end())
            channel = c->second;
      }
      rif.channel = channel;
      if (channel != 0)
      {
         rif.band = (channel < 15) ? RADIO_BAND_2_4_GHZ : RADIO_BAND_5_GHZ;
         rif.frequency = WirelessChannelToFrequency(rif.band, rif.channel);
      }

      rif.powerDBm = json_object_get_int32(vap, "tx_power", 0);
      ap->addRadioInterface(rif);
   }
}

/**
 * Get access points
 */
static ObjectArray<AccessPointInfo> *GetAccessPoints(NObject *wirelessDomain)
{
   json_t *devices = DoRequest(wirelessDomain, "stat/device");
   if (devices == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetAccessPoints: cannot read device list from controller"));
      return nullptr;
   }

   json_t *data = json_is_object(devices) ? json_object_get(devices, "data") : nullptr;
   if (!json_is_array(data))
   {
      json_decref(devices);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetAccessPoints: invalid device list document"));
      return nullptr;
   }

   auto apList = new ObjectArray<AccessPointInfo>(256, 256, Ownership::True);
   size_t i;
   json_t *dev;
   uint32_t index = 1;
   json_array_foreach(data, i, dev)
   {
      if (!json_is_object(dev))
         continue;

      const char *type = json_object_get_string_utf8(dev, "type", "");
      if (strcmp(type, "uap"))
         continue;

      const char *macText = json_object_get_string_utf8(dev, "mac", nullptr);
      if (macText == nullptr)
         continue;

      MacAddress macAddr = MacAddress::parse(macText);
      if (!macAddr.isValid())
         continue;

      InetAddress ipAddr = InetAddress::parse(json_object_get_string_utf8(dev, "ip", ""));
      if (!ipAddr.isValid())
      {
         json_t *cn = json_object_get(dev, "config_network");
         if (json_is_object(cn))
            ipAddr = InetAddress::parse(json_object_get_string_utf8(cn, "ip", ""));
      }

      AccessPointState state = AP_UNKNOWN;
      int32_t stateValue = json_object_get_int32(dev, "state", -1);
      if (stateValue == 1)
         state = AP_UP;
      else if (stateValue == 0)
         state = AP_DOWN;

      TCHAR *name = json_object_get_string_t(dev, "name", nullptr);
      TCHAR *model = json_object_get_string_t(dev, "model", nullptr);
      TCHAR *serial = json_object_get_string_t(dev, "serial", nullptr);

      auto ap = new AccessPointInfo(index++, macAddr, ipAddr, state,
         (name != nullptr) ? name : macAddr.toString(), _T("Ubiquiti, Inc."), model, serial);
      AddRadioInterfaces(ap, dev);
      apList->add(ap);

      MemFree(name);
      MemFree(model);
      MemFree(serial);
   }

   json_decref(devices);
   return apList;
}

/**
 * Create wireless station info from JSON document
 */
static WirelessStationInfo *WirelessStationInfoFromJSON(json_t *client)
{
   auto ws = new WirelessStationInfo;
   memset(ws, 0, sizeof(WirelessStationInfo));

   const char *macText = json_object_get_string_utf8(client, "mac", "00:00:00:00:00:00");
   memcpy(ws->macAddr, MacAddress::parse(macText).value(), MAC_ADDR_LENGTH);

   const char *ipText = json_object_get_string_utf8(client, "ip", nullptr);
   if (ipText == nullptr)
      ipText = json_object_get_string_utf8(client, "last_ip", "0.0.0.0");
   ws->ipAddr = InetAddress::parse(ipText);

   ws->vlan = json_object_get_int32(client, "vlan", 0);

   const char *bssidText = json_object_get_string_utf8(client, "bssid", "00:00:00:00:00:00");
   memcpy(ws->bssid, MacAddress::parse(bssidText).value(), MAC_ADDR_LENGTH);

   const char *ssidText = json_object_get_string_utf8(client, "essid", nullptr);
   if (ssidText == nullptr)
      ssidText = json_object_get_string_utf8(client, "ssid", "");
   utf8_to_wchar(ssidText, -1, ws->ssid, MAX_SSID_LENGTH);

   ws->rssi = json_object_get_int32(client, "signal", 0);
   if (ws->rssi == 0)
      ws->rssi = json_object_get_int32(client, "rssi", 0);

   const char *radioName = json_object_get_string_utf8(client, "radio_name", nullptr);
   if (radioName != nullptr)
      ws->rfIndex = ParseRadioIndex(radioName);

   ws->apMatchPolicy = AP_MATCH_BY_BSSID;
   return ws;
}

/**
 * Get wireless stations
 */
static ObjectArray<WirelessStationInfo> *GetWirelessStations(NObject *wirelessDomain)
{
   json_t *clients = DoRequest(wirelessDomain, "stat/sta");
   if (clients == nullptr)
      return nullptr;

   json_t *data = json_is_object(clients) ? json_object_get(clients, "data") : nullptr;
   if (!json_is_array(data))
   {
      json_decref(clients);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetWirelessStations: invalid station list document"));
      return nullptr;
   }

   auto wsList = new ObjectArray<WirelessStationInfo>(json_array_size(data), 16, Ownership::True);
   size_t i;
   json_t *client;
   json_array_foreach(data, i, client)
   {
      if (!json_is_object(client))
         continue;
      if (json_is_true(json_object_get(client, "is_wired")))
         continue;
      wsList->add(WirelessStationInfoFromJSON(client));
   }

   json_decref(clients);
   return wsList;
}

/**
 * Get access point state
 */
static AccessPointState GetAccessPointState(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *serial, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   json_t *devices = DoRequest(wirelessDomain, "stat/device");
   if (devices == nullptr)
      return AP_UNKNOWN;

   json_t *data = json_is_object(devices) ? json_object_get(devices, "data") : nullptr;
   if (!json_is_array(data))
   {
      json_decref(devices);
      return AP_UNKNOWN;
   }

   AccessPointState state = AP_UNKNOWN;
   size_t i;
   json_t *dev;
   json_array_foreach(data, i, dev)
   {
      const char *type = json_object_get_string_utf8(dev, "type", "");
      if (strcmp(type, "uap"))
         continue;

      const char *macText = json_object_get_string_utf8(dev, "mac", nullptr);
      if (macText == nullptr)
         continue;

      if (!MacAddress::parse(macText).equals(macAddr))
         continue;

      int32_t stateValue = json_object_get_int32(dev, "state", -1);
      if (stateValue == 1)
         state = AP_UP;
      else if (stateValue == 0)
         state = AP_DOWN;
      break;
   }

   json_decref(devices);
   return state;
}

/**
 * Get access point metric
 */
static DataCollectionError GetAccessPointMetric(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *serial, const TCHAR *name, TCHAR *value, size_t size)
{
   return DCE_NOT_SUPPORTED;
}

/**
 * Get list of stations registered at given access point
 */
static ObjectArray<WirelessStationInfo> *GetAccessPointWirelessStations(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *serial)
{
   json_t *clients = DoRequest(wirelessDomain, "stat/sta");
   if (clients == nullptr)
      return nullptr;

   json_t *data = json_is_object(clients) ? json_object_get(clients, "data") : nullptr;
   if (!json_is_array(data))
   {
      json_decref(clients);
      return nullptr;
   }

   auto wsList = new ObjectArray<WirelessStationInfo>(0, 128, Ownership::True);
   size_t i;
   json_t *client;
   json_array_foreach(data, i, client)
   {
      if (!json_is_object(client))
         continue;
      if (json_is_true(json_object_get(client, "is_wired")))
         continue;

      const char *apMacText = json_object_get_string_utf8(client, "ap_mac", nullptr);
      if (apMacText == nullptr)
         continue;

      if (!MacAddress::parse(apMacText).equals(macAddr))
         continue;

      wsList->add(WirelessStationInfoFromJSON(client));
   }

   json_decref(clients);
   return wsList;
}

/**
 * Bridge interface
 */
WirelessControllerBridge g_unifiBridge =
{
   GetAccessPoints,
   GetWirelessStations,
   GetAccessPointState,
   GetAccessPointMetric,
   GetAccessPointWirelessStations
};
