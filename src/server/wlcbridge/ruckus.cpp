/*
** NetXMS - Network Management System
** Copyright (C) 2023-2024 Raden Solutions
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
** File: ruckus.cpp
**
**/

#include "wlcbridge.h"
#include <nddrv.h>
#include <nxlibcurl.h>
#include <jansson.h>
#include <netxms-version.h>

#define DEBUG_TAG WLCBRIDGE_DEBUG_TAG _T(".ruckus")

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *context)
{
   size_t bytes = size * nmemb;
   static_cast<ByteStream*>(context)->write(ptr, bytes);
   return bytes;
}

/**
 * Read JSON document from bridge process
 */
static json_t *ReadJsonFromBridge(const char *endpoint)
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return nullptr;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS WLC Bridge/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   bool success = true;

   char url[256];
   snprintf(url, 256, "http://127.0.0.1:5000/%s", endpoint);
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
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errorBuffer);
         success = false;
      }
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes"), static_cast<int>(responseData.size()));
      long httpCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      if (httpCode != 200)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from bridge process: HTTP response code is %d"), httpCode);
         success = false;
      }
   }

   if (success && responseData.size() <= 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Empty response from bridge process"));
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

   curl_easy_cleanup(curl);
   return json;
}

/**
 * Get access points
 */
static ObjectArray<AccessPointInfo> *GetAccessPoints(NObject *wirelessDomain)
{
   json_t *inventory = ReadJsonFromBridge("inventory");
   if (inventory == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GetAccessPoints: cannot read inventory document from bridge process"));
      return nullptr;
   }

   if (!json_is_array(inventory))
   {
      json_decref(inventory);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GetAccessPoints: invalid inventory document received from bridge process"));
      return nullptr;
   }

   auto apList = new ObjectArray<AccessPointInfo>(256, 256, Ownership::True);

   size_t i;
   json_t *json;
   json_array_foreach(inventory, i, json)
   {
      const char *macAddrText = json_object_get_string_utf8(json, "ap", nullptr);
      if (macAddrText == nullptr)
         continue;

      MacAddress macAddr = MacAddress::parse(macAddrText);
      if (!macAddr.isValid())
         continue;

      InetAddress ipAddr = InetAddress::parse(json_object_get_string_utf8(json, "ip", "0.0.0.0"));

      AccessPointState state;
      switch(json_object_get_int32(json, "apState", 0))
      {
         case 1:
            state = AP_UP;
            break;
         case 2:
            state = AP_DOWN;
            break;
         default:
            state = AP_UNKNOWN;
            break;
      }

      TCHAR *name = json_object_get_string_t(json, "deviceName", nullptr);
      TCHAR *model = json_object_get_string_t(json, "model", nullptr);
      TCHAR *serial = json_object_get_string_t(json, "serialNumber", nullptr);

      auto ap = new AccessPointInfo(i, macAddr, ipAddr, state, (name != nullptr) ? name : macAddr.toString(), _T("Ruckus Networks"), model, serial);
      apList->add(ap);

      MemFree(name);
      MemFree(model);
      MemFree(serial);

      json_t *radios = json_object_get(json, "radios");
      if (json_is_object(radios))
      {
         const char *key;
         json_t *radio;
         json_object_foreach(radios, key, radio)
         {
            RadioInterfaceInfo rif;
            memset(&rif, 0, sizeof(rif));
            rif.index = strtoul(key, nullptr, 10);
            _sntprintf(rif.name, MAX_OBJECT_NAME, _T("radio%u"), rif.index);
            rif.channel = static_cast<uint16_t>(json_object_get_uint32(radio, "channel", 0));
            rif.powerDBm = strtol(json_object_get_string_utf8(radio, "txPower", "0"), nullptr, 10);
            rif.powerMW = (int)pow(10.0, (double)rif.powerDBm / 10.0);

            const char *band = json_object_get_string_utf8(radio, "band", nullptr);
            if (band != nullptr)
            {
               if (!strcmp(band, "2.4G"))
                  rif.band = RADIO_BAND_2_4_GHZ;
               else if (!strcmp(band, "5G"))
                  rif.band = RADIO_BAND_5_GHZ;
            }

            json_t *wlans = json_object_get(radio, "wlans");
            if (json_is_array(wlans))
            {
               int j;
               json_t *wlan;
               json_array_foreach(wlans, j, wlan)
               {
                  MacAddress bssid = MacAddress::parse(json_object_get_string_utf8(wlan, "bssid", ""));
                  if (bssid.isValid() && (bssid.length() == MAC_ADDR_LENGTH))
                  {
                     memcpy(rif.bssid, bssid.value(), MAC_ADDR_LENGTH);
                     utf8_to_tchar(json_object_get_string_utf8(wlan, "ssid", ""), -1, rif.ssid, MAX_SSID_LENGTH);
                     ap->addRadioInterface(rif);
                  }
               }
            }
         }
      }
   }

   json_decref(inventory);
   return apList;
}

/**
 * Get wireless stations
 */
static ObjectArray<WirelessStationInfo> *GetWirelessStations(NObject *wirelessDomain)
{
   return nullptr;
}

/**
 * Get access point state
 */
static AccessPointState GetAccessPointState(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   char endpoint[64], macAddrText[24];
   snprintf(endpoint, 64, "inventory/%s", BinToStrExA(macAddr.value(), MAC_ADDR_LENGTH, macAddrText, ':', 0));
   json_t *ap = ReadJsonFromBridge(endpoint);
   if (ap == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GetAccessPointState: cannot read inventory document from bridge process"));
      return AP_UNKNOWN;
   }

   if (!json_is_object(ap))
   {
      json_decref(ap);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GetAccessPointState: invalid inventory document received from bridge process"));
      return AP_UNKNOWN;
   }

   AccessPointState state;
   switch(json_object_get_int32(ap, "apState", 0))
   {
      case 1:
         state = AP_UP;
         break;
      case 2:
         state = AP_DOWN;
         break;
      default:
         state = AP_UNKNOWN;
         break;
   }

   json_decref(ap);
   return state;
}

/**
 * Get access point metric
 */
static DataCollectionError GetAccessPointMetric(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *name, TCHAR *value, size_t size)
{
   char endpoint[64], macAddrText[24];
   snprintf(endpoint, 64, "inventory/%s", BinToStrExA(macAddr.value(), MAC_ADDR_LENGTH, macAddrText, ':', 0));
   json_t *ap = ReadJsonFromBridge(endpoint);
   if (ap == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GetAccessPointMetric: cannot read inventory document from bridge process"));
      return DCE_COLLECTION_ERROR;
   }

   if (!json_is_object(ap))
   {
      json_decref(ap);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GetAccessPointMetric: invalid inventory document received from bridge process"));
      return DCE_COLLECTION_ERROR;
   }

   DataCollectionError status;
   char key[128];
   tchar_to_utf8(name, -1, key, 128);
   key[127] = 0;
   json_t *data = json_object_get(ap, key);
   if (data != nullptr)
   {
      TCHAR buffer[32];
      switch(json_typeof(data))
      {
         case JSON_STRING:
            utf8_to_tchar(json_string_value(data), -1, value, size);
            value[size - 1] = 0;
            break;
         case JSON_INTEGER:
            IntegerToString(static_cast<int64_t>(json_integer_value(data)), buffer);
            _tcslcpy(value, buffer, size);
            break;
         case JSON_REAL:
            _sntprintf(value, size, _T("%f"), json_real_value(data));
            break;
         case JSON_TRUE:
            _tcslcpy(value, _T("true"), size);
            break;
         case JSON_FALSE:
            _tcslcpy(value, _T("false"), size);
            break;
         default:
            *value = 0;
            break;
      }
      status = DCE_SUCCESS;
   }
   else
   {
      status = DCE_NOT_SUPPORTED;
   }

   json_decref(ap);
   return status;
}

/**
 * Bridge interface
 */
WirelessControllerBridge g_ruckusBridge =
{
   .getAccessPoints = GetAccessPoints,
   .getWirelessStations = GetWirelessStations,
   .getAccessPointState = GetAccessPointState,
   .getAccessPointMetric = GetAccessPointMetric
};
