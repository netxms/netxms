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
** File: aruba.cpp
**
**/

#include "wlcbridge.h"
#include <nddrv.h>
#include <map>
#include <regex>
#include <array>
#include <set>
#include <sstream>
#include <vector>

#define DEBUG_TAG WLCBRIDGE_DEBUG_TAG _T(".aruba")
static const time_t COMMAND_CACHE_TTL = 5; // seconds
static const char *CMD_SHOW_SUMMARY = "show summary";
static const char *CMD_SHOW_APS = "show aps";
static const char *CMD_SHOW_CLIENTS = "show clients";

/**
 * Get custom attribute from domain object as UTF-8 string
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
 * Trim and normalize base URL (ensure scheme, remove trailing slash)
 */
static void NormalizeBaseUrl(std::string& url)
{
   url.erase(0, url.find_first_not_of(" \t\r\n"));
   url.erase(url.find_last_not_of(" \t\r\n") + 1);

   if ((url.rfind("http://", 0) != 0) && (url.rfind("https://", 0) != 0))
      url = "https://" + url;

   while (!url.empty() && url.back() == '/')
      url.pop_back();
}

/**
 * Extract host part from URL
 */
static std::string ExtractHost(const std::string& url)
{
   size_t start = url.find("://");
   start = (start == std::string::npos) ? 0 : (start + 3);
   size_t end = url.find_first_of(":/", start);
   if (end == std::string::npos)
      end = url.length();
   return url.substr(start, end - start);
}

/**
 * Login to Aruba Instant controller
 */
static bool Login(const std::string& baseUrl, const std::string& login, const std::string& password, std::string& sid)
{
   ByteStream responseData(2048);
   responseData.setAllocationStep(2048);

   char errorBuffer[CURL_ERROR_SIZE];
   CURL *curl = CreateCurlHandle(&responseData, errorBuffer);
   if (curl == nullptr)
      return false;

   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

   json_t *request = json_object();
   json_object_set_new(request, "user", json_string(login.c_str()));
   json_object_set_new(request, "passwd", json_string(password.c_str()));
   char *data = json_dumps(request, 0);
   json_decref(request);

   curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
   curl_easy_setopt(curl, CURLOPT_POST, 1L);
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Content-Type: application/json");
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   std::string url = baseUrl + "/rest/login";
   bool success = true;
   if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
      success = false;
   }

   if (success)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Call to curl_easy_perform(%hs) failed (%d: %hs)"), url.c_str(), rc, errorBuffer);
         success = false;
      }
   }

   if (success)
   {
      long httpCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      if (httpCode != 200)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Login failed: HTTP response code is %d"), httpCode);
         success = false;
      }
   }

   if (success)
   {
      responseData.write('\0');
      const char *payload = reinterpret_cast<const char*>(responseData.buffer());
      json_error_t error;
      json_t *json = json_loads(payload, 0, &error);
      if (json == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse login JSON on line %d: %hs"), error.line, error.text);
         success = false;
      }
      else
      {
         const char *status = json_object_get_string_utf8(json, "Status", nullptr);
         if ((status == nullptr) || strcasecmp(status, "Success"))
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("Login failed: %hs"), json_object_get_string_utf8(json, "Error message", "Unknown error"));
            success = false;
         }
         else
         {
            const char *sidText = json_object_get_string_utf8(json, "sid", nullptr);
            if (sidText != nullptr)
               sid = sidText;
            else
               success = false;
         }
         json_decref(json);
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(data);
   return success;
}

/**
 * Read JSON document from controller
 */
static json_t *ReadJsonFromController(const char *url)
{
   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);

   char errorBuffer[CURL_ERROR_SIZE];
   CURL *curl = CreateCurlHandle(&responseData, errorBuffer);
   if (curl == nullptr)
      return nullptr;

   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
   curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

   bool success = true;
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
      long httpCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      if (httpCode != 200)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Error response from controller: HTTP response code is %d"), httpCode);
         success = false;
      }
   }

   if (success && (responseData.size() <= 0))
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
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON on line %d: %hs"), error.line, error.text);
   }

   curl_easy_cleanup(curl);
   return json;
}

/**
 * Send show-cmd request and return JSON
 */
static json_t *DoShowCommand(NObject *wirelessDomain, const char *command)
{
   if (IsShutdownInProgress())
      return nullptr;

   std::string baseUrl = GetDomainAttribute(wirelessDomain, _T("aruba.base-url"));
   if (baseUrl.empty())
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Controller base URL not provided"));
      return nullptr;
   }
   NormalizeBaseUrl(baseUrl);

   std::string sid = GetDomainAttribute(wirelessDomain, _T("$aruba.sid"));
   if (sid.empty())
   {
      std::string login = GetDomainAttribute(wirelessDomain, _T("aruba.login"));
      std::string password = GetDomainAttribute(wirelessDomain, _T("aruba.password"));
      if (login.empty() || password.empty())
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Login/password not provided"));
         return nullptr;
      }
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Aruba Instant: logging in"));
      if (!Login(baseUrl, login, password, sid))
         return nullptr;
      wirelessDomain->setCustomAttribute(_T("$aruba.sid"), SharedString(sid.c_str(), "ASCII"), StateChange::CLEAR);
   }

   std::string iapIp = GetDomainAttribute(wirelessDomain, _T("aruba.iap-ip"));
   if (iapIp.empty())
      iapIp = ExtractHost(baseUrl);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   char errorBuffer[CURL_ERROR_SIZE];
   CURL *curl = CreateCurlHandle(&responseData, errorBuffer);
   if (curl == nullptr)
      return nullptr;

   char *cmdEncoded = curl_easy_escape(curl, command, 0);
   std::string url = baseUrl + "/rest/show-cmd?iap_ip_addr=" + iapIp + "&cmd=" + (cmdEncoded != nullptr ? cmdEncoded : "") + "&sid=" + sid;
   if (cmdEncoded != nullptr)
      curl_free(cmdEncoded);
   curl_easy_cleanup(curl);

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Show command: %hs"), url.c_str());
   json_t *json = ReadJsonFromController(url.c_str());
   if (json == nullptr)
      return nullptr;

   const char *status = json_object_get_string_utf8(json, "Status", nullptr);
   int statusCode = json_object_get_int32(json, "Status-code", 0);
   const char *message = json_object_get_string_utf8(json, "message", nullptr);
   if ((status != nullptr) && strcasecmp(status, "Success"))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Controller error: %hs"), json_object_get_string_utf8(json, "Error message", "Unknown error"));
      json_decref(json);
      return nullptr;
   }

   if (statusCode == 1 && message != nullptr && strstr(message, "Invalid session id") != nullptr)
   {
      wirelessDomain->setCustomAttribute(_T("$aruba.sid"), SharedString(), StateChange::CLEAR);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Aruba session expired; cleared $aruba.sid"));
      json_decref(json);
      return nullptr;
   }

   return json;
}

static bool ParseSummaryMacs(const char *output, std::map<std::string, MacAddress>& apNameToMac);
static bool ParseSummaryClientApMap(const char *output, std::map<std::string, std::string>& clientMacToApName);
static bool ParseApRadioChannels(const char *output, std::map<std::string, std::array<int, 3>>& apRadioChannels);

enum ApColumns
{
   AP_COL_NAME = 0,
   AP_COL_IP = 1,
   AP_COL_MODEL = 5,
   AP_COL_SERIAL = 9,
   AP_COL_RADIO0_CHANNEL = 10,
   AP_COL_RADIO0_POWER = 11,
   AP_COL_RADIO1_CHANNEL = 14,
   AP_COL_RADIO1_POWER = 15,
   AP_COL_RADIO2_CHANNEL = 18,
   AP_COL_RADIO2_POWER = 19
};

struct CommandCacheEntry
{
   time_t summaryTimestamp;
   time_t apsTimestamp;
   time_t clientsTimestamp;
   std::string summaryOutput;
   std::string apsOutput;
   std::string clientsOutput;
};

static std::map<uint32_t, CommandCacheEntry> s_commandCache;
static Mutex s_commandCacheLock(MutexType::FAST);

static bool GetCommandOutput(NObject *wirelessDomain, const char *command, std::string& output)
{
   json_t *response = DoShowCommand(wirelessDomain, command);
   if (response == nullptr)
      return false;

   const char *text = json_object_get_string_utf8(response, "Command output", nullptr);
   if (text == nullptr)
   {
      const char *status = json_object_get_string_utf8(response, "Status", "unknown");
      const char *err = json_object_get_string_utf8(response, "Error message", "");
      char *dump = json_dumps(response, JSON_COMPACT);
      if (dump != nullptr)
      {
         if (strlen(dump) > 512)
            dump[512] = 0;
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Missing Command output for %hs (status=%hs error=%hs json=%hs)"),
            command, status, err, dump);
         MemFree(dump);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Missing Command output for %hs (status=%hs error=%hs)"),
            command, status, err);
      }
      json_decref(response);
      return false;
   }

   output = text;
   json_decref(response);
   return true;
}

static bool GetCachedCommandOutput(NObject *wirelessDomain, const char *command, std::string& output)
{
   time_t now = time(nullptr);
   uint32_t id = wirelessDomain->getId();

   {
      LockGuard lock(s_commandCacheLock);
      auto it = s_commandCache.find(id);
      if (it != s_commandCache.end())
      {
         CommandCacheEntry& entry = it->second;
      if (!strcmp(command, CMD_SHOW_SUMMARY) && (entry.summaryTimestamp > 0) && (now - entry.summaryTimestamp <= COMMAND_CACHE_TTL))
      {
         output = entry.summaryOutput;
         return true;
      }
      if (!strcmp(command, CMD_SHOW_APS) && (entry.apsTimestamp > 0) && (now - entry.apsTimestamp <= COMMAND_CACHE_TTL))
      {
         output = entry.apsOutput;
         return true;
      }
      if (!strcmp(command, CMD_SHOW_CLIENTS) && (entry.clientsTimestamp > 0) && (now - entry.clientsTimestamp <= COMMAND_CACHE_TTL))
      {
         output = entry.clientsOutput;
         return true;
      }
      }
   }

   if (!GetCommandOutput(wirelessDomain, command, output))
      return false;

   {
      LockGuard lock(s_commandCacheLock);
      CommandCacheEntry& entry = s_commandCache[id];
      if (!strcmp(command, CMD_SHOW_SUMMARY))
      {
         entry.summaryTimestamp = now;
         entry.summaryOutput = output;
      }
      else if (!strcmp(command, CMD_SHOW_APS))
      {
         entry.apsTimestamp = now;
         entry.apsOutput = output;
      }
      else if (!strcmp(command, CMD_SHOW_CLIENTS))
      {
         entry.clientsTimestamp = now;
         entry.clientsOutput = output;
      }
   }

   return true;
}

static void BuildApNameToMac(NObject *wirelessDomain, std::map<std::string, MacAddress>& apNameToMac)
{
   std::string summary;
   if (GetCachedCommandOutput(wirelessDomain, CMD_SHOW_SUMMARY, summary))
      ParseSummaryMacs(summary.c_str(), apNameToMac);
}

static void BuildClientMacToApName(NObject *wirelessDomain, std::map<std::string, std::string>& clientMacToApName)
{
   std::string summary;
   if (GetCachedCommandOutput(wirelessDomain, CMD_SHOW_SUMMARY, summary))
      ParseSummaryClientApMap(summary.c_str(), clientMacToApName);
}

static void BuildApRadioChannels(NObject *wirelessDomain, std::map<std::string, std::array<int, 3>>& apRadioChannels)
{
   std::string output;
   if (GetCachedCommandOutput(wirelessDomain, CMD_SHOW_APS, output))
      ParseApRadioChannels(output.c_str(), apRadioChannels);
}

static std::string Trim(const std::string& s)
{
   size_t start = s.find_first_not_of(" \t\r\n");
   if (start == std::string::npos)
      return std::string();
   size_t end = s.find_last_not_of(" \t\r\n");
   return s.substr(start, end - start + 1);
}

static void SplitLines(const char *text, std::vector<std::string>& lines)
{
   if (text == nullptr)
      return;
   const char *p = text;
   const char *lineStart = p;
   while (*p != 0)
   {
      if (*p == '\n')
      {
         std::string line(lineStart, p - lineStart);
         if (!line.empty() && (line.back() == '\r'))
            line.pop_back();
         lines.push_back(line);
         lineStart = p + 1;
      }
      p++;
   }
   if (p != lineStart)
   {
      std::string line(lineStart, p - lineStart);
      if (!line.empty() && (line.back() == '\r'))
         line.pop_back();
      lines.push_back(line);
   }
}

static void SplitColumns(const std::string& line, std::vector<std::string>& columns)
{
   static const std::regex sep(R"(\s{2,})");
   std::sregex_token_iterator it(line.begin(), line.end(), sep, -1);
   std::sregex_token_iterator end;
   for (; it != end; ++it)
   {
      if (!it->str().empty())
         columns.push_back(it->str());
   }
}

static int32_t ParseLeadingInt(const std::string& s)
{
   size_t i = 0;
   while ((i < s.size()) && !isdigit(static_cast<unsigned char>(s[i])))
      i++;
   int32_t v = 0;
   while ((i < s.size()) && isdigit(static_cast<unsigned char>(s[i])))
   {
      v = (v * 10) + (s[i] - '0');
      i++;
   }
   return v;
}

static bool IsNumber(const std::string& s)
{
   if (s.empty())
      return false;
   for (char c : s)
   {
      if (!isdigit(static_cast<unsigned char>(c)))
         return false;
   }
   return true;
}

static void AddRadioInterfaces(AccessPointInfo *ap, uint32_t& index, const char *name, const std::string& channelText, const std::string& powerText, const MacAddress& bssid, const std::vector<std::string> *ssids)
{
   uint16_t channel = static_cast<uint16_t>(ParseLeadingInt(channelText));
   if (channel == 0)
      return;

   auto addOne = [&](const char *ssid)
   {
      RadioInterfaceInfo rif;
      memset(&rif, 0, sizeof(rif));
      rif.index = index;
      rif.ifIndex = index;
      utf8_to_tchar(name, -1, rif.name, MAX_OBJECT_NAME);
      if (bssid.isValid())
         memcpy(rif.bssid, bssid.value(), MAC_ADDR_LENGTH);
      utf8_to_tchar(ssid, -1, rif.ssid, MAX_SSID_LENGTH);
      rif.channel = channel;
      rif.band = (channel < 15) ? RADIO_BAND_2_4_GHZ : RADIO_BAND_5_GHZ;
      rif.frequency = WirelessChannelToFrequency(rif.band, rif.channel);
      rif.powerDBm = ParseLeadingInt(powerText);
      ap->addRadioInterface(rif);
      index++;
   };

   if ((ssids != nullptr) && !ssids->empty())
   {
      for (const auto& s : *ssids)
         addOne(s.c_str());
   }
   else
   {
      addOne("multiple");
   }
}

static bool ParseSummaryMacs(const char *output, std::map<std::string, MacAddress>& apNameToMac)
{
   std::vector<std::string> lines;
   SplitLines(output, lines);

   static const std::regex summaryRegex(
      R"(^([0-9A-Fa-f:]{17})\s{2}(\d+.\d+.\d+.\d+)\s+(.{10}).*$)");

   int matches = 0;
   for (const auto& line : lines)
   {
      if (line.empty())
         continue;
      if (line.find("Restricted Management Access Subnets") != std::string::npos)
         break;

      std::smatch m;
      if (!std::regex_match(line, m, summaryRegex))
         continue;

      std::string macText = m[1].str();
      std::string nameText = m[3].str();
      nameText = Trim(nameText);

      MacAddress macAddr = MacAddress::parse(macText.c_str());
      if (!macAddr.isValid())
         continue;

      apNameToMac[nameText] = macAddr;
      matches++;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Parsed %d AP MAC mappings from show summary"), matches);
   return !apNameToMac.empty();
}

static bool ParseSummaryClientApMap(const char *output, std::map<std::string, std::string>& clientMacToApName)
{
   std::vector<std::string> lines;
   SplitLines(output, lines);

   static const std::regex clientApRegex(
      R"(^([0-9A-Fa-f:]{17})\s{2}(.{25})\s(\d+.\d+.\d+.\d+)\s+(.{10})\s(.+)$)");

   int matches = 0;
   for (const auto& line : lines)
   {
      if (line.empty())
         continue;
      if (line.find("Networks") != std::string::npos)
         break;

      std::smatch m;
      if (!std::regex_match(line, m, clientApRegex))
         continue;

      std::string clientMac = m[1].str();
      std::string apName = Trim(m[5].str());
      if (!clientMac.empty() && !apName.empty())
      {
         clientMacToApName[clientMac] = apName;
         matches++;
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Parsed %d client->AP mappings from show summary"), matches);
   return !clientMacToApName.empty();
}

static bool ParseApRadioChannels(const char *output, std::map<std::string, std::array<int, 3>>& apRadioChannels)
{
   std::vector<std::string> lines;
   SplitLines(output, lines);

   for (const auto& line : lines)
   {
      if (line.empty())
         continue;
      if (!line.compare(0, 4, "Name") || !line.compare(0, 4, "----") || line.find("Access Points") != std::string::npos)
         continue;

      std::vector<std::string> cols;
      SplitColumns(line, cols);
      if (cols.size() <= AP_COL_RADIO2_CHANNEL)
         continue;

      std::string nameText = cols[AP_COL_NAME];
      std::array<int, 3> channels = {0, 0, 0};
      channels[0] = ParseLeadingInt(cols[AP_COL_RADIO0_CHANNEL]);
      channels[1] = ParseLeadingInt(cols[AP_COL_RADIO1_CHANNEL]);
      channels[2] = ParseLeadingInt(cols[AP_COL_RADIO2_CHANNEL]);
      apRadioChannels[nameText] = channels;
   }

   return !apRadioChannels.empty();
}

static bool ParseAps(const char *output, std::vector<AccessPointInfo*>& apList, const std::map<std::string, MacAddress>* apNameToMac, const std::vector<std::string> *ssids)
{
   std::vector<std::string> lines;
   SplitLines(output, lines);

   uint32_t index = 1;
   int matches = 0;
   for (const auto& line : lines)
   {
      if (line.empty())
         continue;

      if (!line.compare(0, 4, "Name") || !line.compare(0, 4, "----") || line.find("Access Points") != std::string::npos)
         continue;

      std::vector<std::string> cols;
      SplitColumns(line, cols);
      if (cols.size() <= AP_COL_SERIAL)
         continue;

      std::string nameText = cols[AP_COL_NAME];
      std::string ipText = cols[AP_COL_IP];
      std::string modelText = cols[AP_COL_MODEL];
      std::string serialText = cols[AP_COL_SERIAL];

      if (!ipText.empty() && (ipText.back() == '*'))
         ipText.pop_back();

      InetAddress ipAddr = InetAddress::parse(ipText.c_str());
      if (!ipAddr.isValid())
         ipAddr = InetAddress::parse("0.0.0.0");

      TCHAR name[MAX_OBJECT_NAME];
      utf8_to_tchar(nameText.c_str(), -1, name, MAX_OBJECT_NAME);

      TCHAR model[MAX_OBJECT_NAME];
      if (!modelText.empty())
         utf8_to_tchar(modelText.c_str(), -1, model, MAX_OBJECT_NAME);

      TCHAR serial[MAX_OBJECT_NAME];
      utf8_to_tchar(serialText.c_str(), -1, serial, MAX_OBJECT_NAME);

      MacAddress macAddr = MacAddress::parse("00:00:00:00:00:00");
      if (apNameToMac != nullptr)
      {
         auto it = apNameToMac->find(nameText);
         if (it != apNameToMac->end())
            macAddr = it->second;
      }

      auto ap = new AccessPointInfo(index++, macAddr, ipAddr, AP_UP,
         name, _T("Aruba Networks"), modelText.empty() ? nullptr : model, serialText.empty() ? nullptr : serial);

      uint32_t radioIndex = 1;
      if (cols.size() > AP_COL_RADIO0_POWER)
         AddRadioInterfaces(ap, radioIndex, "radio0", cols[AP_COL_RADIO0_CHANNEL], cols[AP_COL_RADIO0_POWER], macAddr, ssids);
      if (cols.size() > AP_COL_RADIO1_POWER)
         AddRadioInterfaces(ap, radioIndex, "radio1", cols[AP_COL_RADIO1_CHANNEL], cols[AP_COL_RADIO1_POWER], macAddr, ssids);
      if (cols.size() > AP_COL_RADIO2_POWER)
         AddRadioInterfaces(ap, radioIndex, "radio2", cols[AP_COL_RADIO2_CHANNEL], cols[AP_COL_RADIO2_POWER], macAddr, ssids);

      apList.push_back(ap);
      matches++;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Parsed %d access points from show aps"), matches);
   return !apList.empty();
}

static bool ParseClients(const char *output, std::vector<WirelessStationInfo*>& wsList, const std::map<std::string, MacAddress>& apNameToMac, const std::map<std::string, std::string>& clientMacToApName, const std::map<std::string, std::array<int, 3>>& apRadioChannels)
{
   std::vector<std::string> lines;
   SplitLines(output, lines);

   static const std::regex clientRegex(
      R"(^\s*(.*?)\s{2,}(\S+)\s{2,}([0-9A-Fa-f:]{17})\s{2,}(\S+(?:\s+\S+)?)\s{2,}(\S+)\s{2,}(\S+)\s{2,}(\S+)\s{2,}(\S+)\s{2,}(\S+)\s{2,}(\S+)\s{2,}([0-9]+)\([^)]*\)\s{2,}(\S+)\([^)]*\)\s*$)");

   int matches = 0;
   for (const auto& line : lines)
   {
      if (line.find("Number of Clients") != std::string::npos)
         break;

      std::smatch m;
      if (!std::regex_match(line, m, clientRegex))
         continue;

      std::string ipText = m[2].str();
      std::string macText = m[3].str();
      std::string ssidText = m[5].str();
      std::string apName = m[6].str();
      std::string channelText = m[7].str();
      std::string signalText = m[11].str();

      int channel = ParseLeadingInt(channelText);

      auto ws = new WirelessStationInfo;
      memset(ws, 0, sizeof(WirelessStationInfo));

      memcpy(ws->macAddr, MacAddress::parse(macText.c_str()).value(), MAC_ADDR_LENGTH);
      ws->ipAddr = InetAddress::parse(ipText.c_str());
      utf8_to_wchar(ssidText.c_str(), -1, ws->ssid, MAX_SSID_LENGTH);

      auto it = apNameToMac.find(apName);
      if (it != apNameToMac.end())
      {
         memcpy(ws->bssid, it->second.value(), MAC_ADDR_LENGTH);
      }
      else
      {
         auto cm = clientMacToApName.find(macText);
         if (cm != clientMacToApName.end())
         {
            auto it2 = apNameToMac.find(cm->second);
            if (it2 != apNameToMac.end())
               memcpy(ws->bssid, it2->second.value(), MAC_ADDR_LENGTH);
         }
      }

      ws->rssi = ParseLeadingInt(signalText);
      ws->rfIndex = 0;
      auto rc = apRadioChannels.find(apName);
      if (rc != apRadioChannels.end())
      {
         const auto& c = rc->second;
         if (channel != 0)
         {
            if (c[0] == channel)
               ws->rfIndex = 1;
            else if (c[1] == channel)
               ws->rfIndex = 2;
            else if (c[2] == channel)
               ws->rfIndex = 3;
         }
      }
      if (ws->rfIndex == 0)
         ws->rfIndex = channel;

      ws->apMatchPolicy = AP_MATCH_BY_BSSID;
      wsList.push_back(ws);
      matches++;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Parsed %d wireless stations from show clients"), matches);
   return !wsList.empty();
}

static std::vector<std::string> BuildSsidsFromSummary(const char *output)
{
   std::vector<std::string> lines;
   SplitLines(output, lines);

   bool inNetworks = false;
   bool inTable = false;
   std::vector<std::string> ssids;
   std::set<std::string> seen;
   for (const auto& line : lines)
   {
      if (line.find("Networks") != std::string::npos)
      {
         inNetworks = true;
         continue;
      }
      if (!inNetworks)
         continue;

      if (line.empty())
         break;
      if (line.find("Access Points") != std::string::npos)
         break;
      if (line.find("Profile Name") != std::string::npos)
      {
         inTable = true;
         continue;
      }
      if (!inTable)
         continue;
      if (!line.compare(0, 12, "------------"))
         continue;

      std::vector<std::string> cols;
      SplitColumns(line, cols);
      if (cols.size() < 5)
         continue;
      if (!IsNumber(cols[2]))
         continue;
      if (cols[3] != "Enabled" && cols[3] != "Disabled")
         continue;

      std::string essid = Trim(cols[1]);
      if (!essid.empty() && (seen.insert(essid).second))
         ssids.push_back(essid);
   }

   if (ssids.empty())
      nxlog_debug_tag(DEBUG_TAG, 7, _T("No SSIDs found in show summary Networks section"));
   else
      nxlog_debug_tag(DEBUG_TAG, 7, _T("SSIDs from summary: %d"), static_cast<int>(ssids.size()));

   return ssids;
}

/**
 * Get access points
 */
static ObjectArray<AccessPointInfo> *GetAccessPoints(NObject *wirelessDomain)
{
   std::map<std::string, MacAddress> apNameToMac;
   BuildApNameToMac(wirelessDomain, apNameToMac);

   std::vector<std::string> ssids;
   std::string summary;
   if (GetCachedCommandOutput(wirelessDomain, CMD_SHOW_SUMMARY, summary))
      ssids = BuildSsidsFromSummary(summary.c_str());

   std::string output;
   if (!GetCachedCommandOutput(wirelessDomain, CMD_SHOW_APS, output))
      return nullptr;

   std::vector<AccessPointInfo*> aps;
   if (!ParseAps(output.c_str(), aps, apNameToMac.empty() ? nullptr : &apNameToMac, ssids.empty() ? nullptr : &ssids))
      return nullptr;

   auto apList = new ObjectArray<AccessPointInfo>(static_cast<int>(aps.size()), 16, Ownership::True);
   for (auto ap : aps)
      apList->add(ap);

   return apList;
}

/**
 * Get wireless stations
 */
static ObjectArray<WirelessStationInfo> *GetWirelessStations(NObject *wirelessDomain)
{
   std::map<std::string, MacAddress> apNameToMac;
   BuildApNameToMac(wirelessDomain, apNameToMac);
   std::map<std::string, std::string> clientMacToApName;
   BuildClientMacToApName(wirelessDomain, clientMacToApName);
   std::map<std::string, std::array<int, 3>> apRadioChannels;
   BuildApRadioChannels(wirelessDomain, apRadioChannels);

   std::string output;
   if (!GetCachedCommandOutput(wirelessDomain, CMD_SHOW_CLIENTS, output))
      return nullptr;

   std::vector<WirelessStationInfo*> stations;
   if (!ParseClients(output.c_str(), stations, apNameToMac, clientMacToApName, apRadioChannels))
      return nullptr;

   auto wsList = new ObjectArray<WirelessStationInfo>(static_cast<int>(stations.size()), 16, Ownership::True);
   for (auto ws : stations)
      wsList->add(ws);

   return wsList;
}

/**
 * Get access point state
 */
static AccessPointState GetAccessPointState(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *serial, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   std::map<std::string, MacAddress> apNameToMac;
   BuildApNameToMac(wirelessDomain, apNameToMac);

   std::string output;
   if (!GetCachedCommandOutput(wirelessDomain, CMD_SHOW_APS, output))
      return AP_UNKNOWN;

   std::vector<AccessPointInfo*> aps;
   if (!ParseAps(output.c_str(), aps, apNameToMac.empty() ? nullptr : &apNameToMac, nullptr))
      return AP_UNKNOWN;

   AccessPointState state = AP_UNKNOWN;
   for (auto ap : aps)
   {
      if (ap->getMacAddr().equals(macAddr))
      {
         state = ap->getState();
         break;
      }
   }

   for (auto ap : aps)
      delete ap;

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
   std::map<std::string, MacAddress> apNameToMac;
   BuildApNameToMac(wirelessDomain, apNameToMac);
   std::map<std::string, std::string> clientMacToApName;
   BuildClientMacToApName(wirelessDomain, clientMacToApName);
   std::map<std::string, std::array<int, 3>> apRadioChannels;
   BuildApRadioChannels(wirelessDomain, apRadioChannels);

   std::string output;
   if (!GetCachedCommandOutput(wirelessDomain, CMD_SHOW_CLIENTS, output))
      return nullptr;

   std::vector<WirelessStationInfo*> stations;
   if (!ParseClients(output.c_str(), stations, apNameToMac, clientMacToApName, apRadioChannels))
      return nullptr;

   auto wsList = new ObjectArray<WirelessStationInfo>(0, 128, Ownership::True);
   for (auto ws : stations)
   {
      if (!memcmp(ws->bssid, macAddr.value(), MAC_ADDR_LENGTH))
         wsList->add(ws);
      else
         delete ws;
   }

   return wsList;
}

/**
 * Bridge interface
 */
WirelessControllerBridge g_arubaBridge =
{
   GetAccessPoints,
   GetWirelessStations,
   GetAccessPointState,
   GetAccessPointMetric,
   GetAccessPointWirelessStations
};
