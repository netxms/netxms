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

#define DEBUG_TAG WLCBRIDGE_DEBUG_TAG _T(".aruba")
static const time_t COMMAND_CACHE_TTL = 5; // seconds
static const char *CMD_SHOW_SUMMARY = "show summary";
static const char *CMD_SHOW_APS = "show aps";
static const char *CMD_SHOW_CLIENTS = "show clients";

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
         if ((status == nullptr) || stricmp(status, "Success"))
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
static json_t *ReadJsonFromController(const char *url, long *httpStatus)
{
   if (httpStatus != nullptr)
      *httpStatus = 0;

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
      if (httpStatus != nullptr)
         *httpStatus = httpCode;
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
static json_t *DoShowCommand(NObject *wirelessDomain, const char *command, long *httpStatus)
{
   if (httpStatus != nullptr)
      *httpStatus = 0;

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
   json_t *json = ReadJsonFromController(url.c_str(), httpStatus);
   if (json == nullptr)
      return nullptr;

   const char *status = json_object_get_string_utf8(json, "Status", nullptr);
   int statusCode = json_object_get_int32(json, "Status-code", 0);
   const char *message = json_object_get_string_utf8(json, "message", nullptr);
   if ((status != nullptr) && stricmp(status, "Success"))
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

struct ApMacMapping
{
   char name[MAX_OBJECT_NAME];
   MacAddress mac;
};

struct ClientApMapping
{
   MacAddress clientMac;
   char apName[MAX_OBJECT_NAME];
};

struct ApRadioMapping
{
   char apName[MAX_OBJECT_NAME];
   int32_t channels[3];
};

struct SsidEntry
{
   char ssid[MAX_SSID_LENGTH];
};

static bool ParseSummaryMacs(const char *output, StructArray<ApMacMapping> *apNameToMac);
static bool ParseSummaryClientApMap(const char *output, StructArray<ClientApMapping> *clientMacToApName);
static bool ParseApRadioChannels(const char *output, StructArray<ApRadioMapping> *apRadioChannels);

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

class CommandCacheEntry
{
public:
   uint32_t domainId;
   time_t summaryTimestamp;
   time_t apsTimestamp;
   time_t clientsTimestamp;
   std::string summaryOutput;
   std::string apsOutput;
   std::string clientsOutput;

   CommandCacheEntry(uint32_t _domainId) : domainId(_domainId), summaryTimestamp(0), apsTimestamp(0), clientsTimestamp(0) { }
};

static ObjectArray<CommandCacheEntry> s_commandCache(0, 16, Ownership::True);
static Mutex s_commandCacheLock(MutexType::FAST);

static CommandCacheEntry *FindCommandCacheEntry(uint32_t domainId)
{
   for(int i = 0; i < s_commandCache.size(); i++)
   {
      CommandCacheEntry *entry = s_commandCache.get(i);
      if (entry->domainId == domainId)
         return entry;
   }
   return nullptr;
}

static bool GetCommandOutput(NObject *wirelessDomain, const char *command, std::string& output, long *httpStatus)
{
   json_t *response = DoShowCommand(wirelessDomain, command, httpStatus);
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
   uint32_t domainId = wirelessDomain->getId();

   {
      LockGuard lock(s_commandCacheLock);
      CommandCacheEntry *entry = FindCommandCacheEntry(domainId);
      if (entry != nullptr)
      {
         if (!strcmp(command, CMD_SHOW_SUMMARY) && (entry->summaryTimestamp > 0) && (now - entry->summaryTimestamp <= COMMAND_CACHE_TTL))
         {
            output = entry->summaryOutput;
            return true;
         }
         if (!strcmp(command, CMD_SHOW_APS) && (entry->apsTimestamp > 0) && (now - entry->apsTimestamp <= COMMAND_CACHE_TTL))
         {
            output = entry->apsOutput;
            return true;
         }
         if (!strcmp(command, CMD_SHOW_CLIENTS) && (entry->clientsTimestamp > 0) && (now - entry->clientsTimestamp <= COMMAND_CACHE_TTL))
         {
            output = entry->clientsOutput;
            return true;
         }
      }
   }

   long httpStatus = 0;
   if (!GetCommandOutput(wirelessDomain, command, output, &httpStatus))
   {
      if ((httpStatus == 501) || (httpStatus == 502) || (httpStatus == 503))
      {
         LockGuard lock(s_commandCacheLock);
         CommandCacheEntry *entry = FindCommandCacheEntry(domainId);
         if (entry != nullptr)
         {
            if (!strcmp(command, CMD_SHOW_SUMMARY) && !entry->summaryOutput.empty())
            {
               output = entry->summaryOutput;
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Using cached output for %hs due to HTTP %d"), command, httpStatus);
               return true;
            }
            if (!strcmp(command, CMD_SHOW_APS) && !entry->apsOutput.empty())
            {
               output = entry->apsOutput;
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Using cached output for %hs due to HTTP %d"), command, httpStatus);
               return true;
            }
            if (!strcmp(command, CMD_SHOW_CLIENTS) && !entry->clientsOutput.empty())
            {
               output = entry->clientsOutput;
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Using cached output for %hs due to HTTP %d"), command, httpStatus);
               return true;
            }
         }
      }
      return false;
   }

   {
      LockGuard lock(s_commandCacheLock);
      CommandCacheEntry *entry = FindCommandCacheEntry(domainId);
      if (entry == nullptr)
      {
         entry = new CommandCacheEntry(domainId);
         s_commandCache.add(entry);
      }
      if (!strcmp(command, CMD_SHOW_SUMMARY))
      {
         entry->summaryTimestamp = now;
         entry->summaryOutput = output;
      }
      else if (!strcmp(command, CMD_SHOW_APS))
      {
         entry->apsTimestamp = now;
         entry->apsOutput = output;
      }
      else if (!strcmp(command, CMD_SHOW_CLIENTS))
      {
         entry->clientsTimestamp = now;
         entry->clientsOutput = output;
      }
   }

   return true;
}

static void BuildApNameToMac(NObject *wirelessDomain, StructArray<ApMacMapping> *apNameToMac)
{
   std::string summary;
   if (GetCachedCommandOutput(wirelessDomain, CMD_SHOW_SUMMARY, summary))
      ParseSummaryMacs(summary.c_str(), apNameToMac);
}

static void BuildClientMacToApName(NObject *wirelessDomain, StructArray<ClientApMapping> *clientMacToApName)
{
   std::string summary;
   if (GetCachedCommandOutput(wirelessDomain, CMD_SHOW_SUMMARY, summary))
      ParseSummaryClientApMap(summary.c_str(), clientMacToApName);
}

static void BuildApRadioChannels(NObject *wirelessDomain, StructArray<ApRadioMapping> *apRadioChannels)
{
   std::string output;
   if (GetCachedCommandOutput(wirelessDomain, CMD_SHOW_APS, output))
      ParseApRadioChannels(output.c_str(), apRadioChannels);
}

static const char *ReadLine(const char *text, char *line, size_t size)
{
   if ((text == nullptr) || (*text == 0))
      return nullptr;

   size_t pos = 0;
   while((*text != 0) && (*text != '\n'))
   {
      if ((*text != '\r') && (pos < size - 1))
         line[pos++] = *text;
      text++;
   }
   line[pos] = 0;

   if (*text == '\n')
      text++;
   return text;
}

static int SplitColumns(char *line, char **columns, int maxColumns)
{
   int count = 0;
   char *p = line;
   while((*p != 0) && (count < maxColumns))
   {
      while((*p != 0) && isspace(*p))
         p++;
      if (*p == 0)
         break;

      columns[count++] = p;
      int spaces = 0;
      while(*p != 0)
      {
         if (isspace(*p))
         {
            spaces++;
            if (spaces >= 2)
               break;
         }
         else
         {
            spaces = 0;
         }
         p++;
      }

      if (*p == 0)
         break;

      char *end = p - 1;
      while((end >= columns[count - 1]) && isspace(*end))
         end--;
      *(end + 1) = 0;

      while((*p != 0) && isspace(*p))
         p++;
   }
   return count;
}

static int32_t ParseLeadingInt(const char *s)
{
   if (s == nullptr)
      return 0;

   while((*s != 0) && !isdigit(static_cast<unsigned char>(*s)))
      s++;
   if (*s == 0)
      return 0;

   long v = strtol(s, nullptr, 10);
   if (v < INT32_MIN)
      return INT32_MIN;
   if (v > INT32_MAX)
      return INT32_MAX;
   return static_cast<int32_t>(v);
}

static bool IsNumber(const char *s)
{
   if ((s == nullptr) || (*s == 0))
      return false;
   while(*s != 0)
   {
      if (!isdigit(static_cast<unsigned char>(*s)))
         return false;
      s++;
   }
   return true;
}

/**
 * Determine radio band from channel number
 */
static RadioBand RadioBandFromChannel(uint16_t channel)
{
   if (channel <= 14)
      return RADIO_BAND_2_4_GHZ;

   // 6 GHz channels are above standard 5 GHz set in Aruba show output.
   if (channel > 177)
      return RADIO_BAND_6_GHZ;

   return RADIO_BAND_5_GHZ;
}

static const MacAddress *FindApMac(const StructArray<ApMacMapping> *apNameToMac, const char *name)
{
   for(int i = 0; i < apNameToMac->size(); i++)
   {
      ApMacMapping *m = apNameToMac->get(i);
      if (!strcmp(m->name, name))
         return &m->mac;
   }
   return nullptr;
}

static const char *FindApNameByClientMac(const StructArray<ClientApMapping> *clientMacToApName, const MacAddress& clientMac)
{
   for(int i = 0; i < clientMacToApName->size(); i++)
   {
      ClientApMapping *m = clientMacToApName->get(i);
      if (m->clientMac.equals(clientMac))
         return m->apName;
   }
   return nullptr;
}

static const ApRadioMapping *FindApRadioChannels(const StructArray<ApRadioMapping> *apRadioChannels, const char *apName)
{
   for(int i = 0; i < apRadioChannels->size(); i++)
   {
      ApRadioMapping *m = apRadioChannels->get(i);
      if (!strcmp(m->apName, apName))
         return m;
   }
   return nullptr;
}

static void AddRadioInterfaces(AccessPointInfo *ap, uint32_t *index, const char *name, const char *channelText, const char *powerText, const MacAddress& bssid, const StructArray<SsidEntry> *ssids)
{
   uint16_t channel = static_cast<uint16_t>(ParseLeadingInt(channelText));
   if (channel == 0)
      return;

   int ssidCount = (ssids != nullptr) ? ssids->size() : 0;
   if (ssidCount == 0)
   {
      RadioInterfaceInfo rif;
      memset(&rif, 0, sizeof(rif));
      rif.index = *index;
      rif.ifIndex = *index;
      utf8_to_tchar(name, -1, rif.name, MAX_OBJECT_NAME);
      if (bssid.isValid())
         memcpy(rif.bssid, bssid.value(), MAC_ADDR_LENGTH);
      utf8_to_tchar("multiple", -1, rif.ssid, MAX_SSID_LENGTH);
      rif.channel = channel;
      rif.band = RadioBandFromChannel(channel);
      rif.frequency = WirelessChannelToFrequency(rif.band, rif.channel);
      rif.powerDBm = ParseLeadingInt(powerText);
      ap->addRadioInterface(rif);
      (*index)++;
      return;
   }

   for(int i = 0; i < ssidCount; i++)
   {
      RadioInterfaceInfo rif;
      memset(&rif, 0, sizeof(rif));
      rif.index = *index;
      rif.ifIndex = *index;
      utf8_to_tchar(name, -1, rif.name, MAX_OBJECT_NAME);
      if (bssid.isValid())
         memcpy(rif.bssid, bssid.value(), MAC_ADDR_LENGTH);
      utf8_to_tchar(ssids->get(i)->ssid, -1, rif.ssid, MAX_SSID_LENGTH);
      rif.channel = channel;
      rif.band = RadioBandFromChannel(channel);
      rif.frequency = WirelessChannelToFrequency(rif.band, rif.channel);
      rif.powerDBm = ParseLeadingInt(powerText);
      ap->addRadioInterface(rif);
      (*index)++;
   }
}

static bool ParseSummaryMacs(const char *output, StructArray<ApMacMapping> *apNameToMac)
{
   int matches = 0;
   bool accessPointSection = false;
   char line[8192];
   for(const char *p = output; p != nullptr; )
   {
      p = ReadLine(p, line, sizeof(line));
      if (line[0] == 0)
         continue;

      if (strstr(line, "Access Points") != nullptr)
      {
         accessPointSection = true;
         continue;
      }
      if (!accessPointSection)
         continue;
      if (strstr(line, "Restricted Management Access Subnets") != nullptr)
         break;
      if (!strncmp(line, "MAC", 3) || !strncmp(line, "---", 3))
         continue;

      char *columns[16];
      int columnCount = SplitColumns(line, columns, 16);
      if (columnCount < 3)
         continue;

      MacAddress macAddr = MacAddress::parse(columns[0]);
      if (!macAddr.isValid())
         continue;

      TrimA(columns[2]);
      ApMacMapping *entry = apNameToMac->addPlaceholder();
      strlcpy(entry->name, columns[2], MAX_OBJECT_NAME);
      entry->mac = macAddr;
      matches++;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Parsed %d AP MAC mappings from show summary"), matches);
   return (apNameToMac->size() > 0);
}

static bool ParseSummaryClientApMap(const char *output, StructArray<ClientApMapping> *clientMacToApName)
{
   int matches = 0;
   char line[8192];
   for(const char *p = output; p != nullptr; )
   {
      p = ReadLine(p, line, sizeof(line));
      if (line[0] == 0)
         continue;
      if (strstr(line, "Networks") != nullptr)
         break;
      if (!strncmp(line, "---", 3))
         continue;

      char *columns[24];
      int columnCount = SplitColumns(line, columns, 24);
      if (columnCount < 5)
         continue;

      MacAddress clientMac = MacAddress::parse(columns[0]);
      if (!clientMac.isValid())
         continue;
      InetAddress clientIp = InetAddress::parse(columns[2]);
      if (!clientIp.isValid())
         continue;

      TrimA(columns[4]);
      ClientApMapping *entry = clientMacToApName->addPlaceholder();
      entry->clientMac = clientMac;
      strlcpy(entry->apName, columns[4], MAX_OBJECT_NAME);
      matches++;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Parsed %d client->AP mappings from show summary"), matches);
   return (clientMacToApName->size() > 0);
}

static bool ParseApRadioChannels(const char *output, StructArray<ApRadioMapping> *apRadioChannels)
{
   char line[8192];
   for(const char *p = output; p != nullptr; )
   {
      p = ReadLine(p, line, sizeof(line));
      if (line[0] == 0)
         continue;
      if (!strncmp(line, "Name", 4) || !strncmp(line, "----", 4) || (strstr(line, "Access Points") != nullptr))
         continue;

      char *columns[40];
      int columnCount = SplitColumns(line, columns, 40);
      if (columnCount <= AP_COL_RADIO2_CHANNEL)
         continue;

      ApRadioMapping *entry = apRadioChannels->addPlaceholder();
      strlcpy(entry->apName, columns[AP_COL_NAME], MAX_OBJECT_NAME);
      entry->channels[0] = ParseLeadingInt(columns[AP_COL_RADIO0_CHANNEL]);
      entry->channels[1] = ParseLeadingInt(columns[AP_COL_RADIO1_CHANNEL]);
      entry->channels[2] = ParseLeadingInt(columns[AP_COL_RADIO2_CHANNEL]);
   }
   return (apRadioChannels->size() > 0);
}

static ObjectArray<AccessPointInfo> *ParseAps(const char *output, const StructArray<ApMacMapping> *apNameToMac, const StructArray<SsidEntry> *ssids)
{
   ObjectArray<AccessPointInfo> *apList = new ObjectArray<AccessPointInfo>(32, 32, Ownership::True);
   uint32_t index = 1;
   int matches = 0;

   char line[8192];
   for(const char *p = output; p != nullptr; )
   {
      p = ReadLine(p, line, sizeof(line));
      if (line[0] == 0)
         continue;
      if (!strncmp(line, "Name", 4) || !strncmp(line, "----", 4) || (strstr(line, "Access Points") != nullptr))
         continue;

      char *columns[40];
      int columnCount = SplitColumns(line, columns, 40);
      if (columnCount <= AP_COL_SERIAL)
         continue;

      if (columns[AP_COL_IP][0] != 0)
      {
         size_t len = strlen(columns[AP_COL_IP]);
         if (columns[AP_COL_IP][len - 1] == '*')
            columns[AP_COL_IP][len - 1] = 0;
      }

      InetAddress ipAddr = InetAddress::parse(columns[AP_COL_IP]);
      if (!ipAddr.isValid())
         ipAddr = InetAddress::parse("0.0.0.0");

      MacAddress macAddr = MacAddress::parse("00:00:00:00:00:00");
      if (apNameToMac != nullptr)
      {
         const MacAddress *m = FindApMac(apNameToMac, columns[AP_COL_NAME]);
         if (m != nullptr)
            macAddr = *m;
      }

      TCHAR name[MAX_OBJECT_NAME], model[MAX_OBJECT_NAME], serial[MAX_OBJECT_NAME];
      utf8_to_tchar(columns[AP_COL_NAME], -1, name, MAX_OBJECT_NAME);
      utf8_to_tchar(columns[AP_COL_MODEL], -1, model, MAX_OBJECT_NAME);
      utf8_to_tchar(columns[AP_COL_SERIAL], -1, serial, MAX_OBJECT_NAME);
      AccessPointInfo *ap = new AccessPointInfo(index++, macAddr, ipAddr, AP_UP, name, _T("Aruba Networks"), model, serial);

      uint32_t radioIndex = 1;
      if (columnCount > AP_COL_RADIO0_POWER)
         AddRadioInterfaces(ap, &radioIndex, "radio0", columns[AP_COL_RADIO0_CHANNEL], columns[AP_COL_RADIO0_POWER], macAddr, ssids);
      if (columnCount > AP_COL_RADIO1_POWER)
         AddRadioInterfaces(ap, &radioIndex, "radio1", columns[AP_COL_RADIO1_CHANNEL], columns[AP_COL_RADIO1_POWER], macAddr, ssids);
      if (columnCount > AP_COL_RADIO2_POWER)
         AddRadioInterfaces(ap, &radioIndex, "radio2", columns[AP_COL_RADIO2_CHANNEL], columns[AP_COL_RADIO2_POWER], macAddr, ssids);

      apList->add(ap);
      matches++;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Parsed %d access points from show aps"), matches);
   if (apList->isEmpty())
   {
      delete apList;
      return nullptr;
   }
   return apList;
}

static ObjectArray<WirelessStationInfo> *ParseClients(const char *output, const StructArray<ApMacMapping> *apNameToMac, const StructArray<ClientApMapping> *clientMacToApName, const StructArray<ApRadioMapping> *apRadioChannels)
{
   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(32, 32, Ownership::True);
   enum { CLIENT_COL_IP = 1, CLIENT_COL_MAC = 2, CLIENT_COL_SSID = 4, CLIENT_COL_AP_NAME = 5, CLIENT_COL_CHANNEL = 6, CLIENT_COL_SIGNAL = 10 };

   int matches = 0;
   char line[8192];
   for(const char *p = output; p != nullptr; )
   {
      p = ReadLine(p, line, sizeof(line));
      if (strstr(line, "Number of Clients") != nullptr)
         break;
      if (line[0] == 0 || !strncmp(line, "---", 3))
         continue;

      char *columns[40];
      int columnCount = SplitColumns(line, columns, 40);
      if (columnCount <= CLIENT_COL_SIGNAL)
         continue;

      MacAddress clientMac = MacAddress::parse(columns[CLIENT_COL_MAC]);
      if (!clientMac.isValid())
         continue;

      int channel = ParseLeadingInt(columns[CLIENT_COL_CHANNEL]);

      WirelessStationInfo *ws = new WirelessStationInfo;
      memset(ws, 0, sizeof(WirelessStationInfo));
      memcpy(ws->macAddr, clientMac.value(), MAC_ADDR_LENGTH);
      ws->ipAddr = InetAddress::parse(columns[CLIENT_COL_IP]);
      utf8_to_wchar(columns[CLIENT_COL_SSID], -1, ws->ssid, MAX_SSID_LENGTH);

      const MacAddress *apMac = FindApMac(apNameToMac, columns[CLIENT_COL_AP_NAME]);
      if (apMac == nullptr)
      {
         const char *apName = FindApNameByClientMac(clientMacToApName, clientMac);
         if (apName != nullptr)
            apMac = FindApMac(apNameToMac, apName);
      }
      if (apMac != nullptr)
         memcpy(ws->bssid, apMac->value(), MAC_ADDR_LENGTH);

      ws->rssi = ParseLeadingInt(columns[CLIENT_COL_SIGNAL]);
      ws->rfIndex = 0;
      const ApRadioMapping *channels = FindApRadioChannels(apRadioChannels, columns[CLIENT_COL_AP_NAME]);
      if ((channels != nullptr) && (channel != 0))
      {
         if (channels->channels[0] == channel)
            ws->rfIndex = 1;
         else if (channels->channels[1] == channel)
            ws->rfIndex = 2;
         else if (channels->channels[2] == channel)
            ws->rfIndex = 3;
      }
      if (ws->rfIndex == 0)
         ws->rfIndex = channel;

      ws->apMatchPolicy = AP_MATCH_BY_BSSID;
      wsList->add(ws);
      matches++;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("Parsed %d wireless stations from show clients"), matches);
   if (wsList->isEmpty())
   {
      delete wsList;
      return nullptr;
   }
   return wsList;
}

static void BuildSsidsFromSummary(const char *output, StructArray<SsidEntry> *ssids)
{
   bool inNetworks = false;
   bool inTable = false;
   char line[8192];
   for(const char *p = output; p != nullptr; )
   {
      p = ReadLine(p, line, sizeof(line));
      if (strstr(line, "Networks") != nullptr)
      {
         inNetworks = true;
         continue;
      }
      if (!inNetworks)
         continue;

      if (line[0] == 0)
         break;
      if (strstr(line, "Access Points") != nullptr)
         break;
      if (strstr(line, "Profile Name") != nullptr)
      {
         inTable = true;
         continue;
      }
      if (!inTable)
         continue;
      if (!strncmp(line, "------------", 12))
         continue;

      char *columns[12];
      int columnCount = SplitColumns(line, columns, 12);
      if (columnCount < 5)
         continue;
      if (!IsNumber(columns[2]))
         continue;
      if (strcmp(columns[3], "Enabled") && strcmp(columns[3], "Disabled"))
         continue;

      TrimA(columns[1]);
      bool exists = false;
      for(int i = 0; i < ssids->size(); i++)
      {
         if (!strcmp(ssids->get(i)->ssid, columns[1]))
         {
            exists = true;
            break;
         }
      }
      if (!exists && (columns[1][0] != 0))
      {
         SsidEntry *entry = ssids->addPlaceholder();
         strlcpy(entry->ssid, columns[1], MAX_SSID_LENGTH);
      }
   }

   if (ssids->isEmpty())
      nxlog_debug_tag(DEBUG_TAG, 7, _T("No SSIDs found in show summary Networks section"));
   else
      nxlog_debug_tag(DEBUG_TAG, 7, _T("SSIDs from summary: %d"), ssids->size());
}

/**
 * Get access points
 */
static ObjectArray<AccessPointInfo> *GetAccessPoints(NObject *wirelessDomain)
{
   StructArray<ApMacMapping> apNameToMac(0, 32);
   BuildApNameToMac(wirelessDomain, &apNameToMac);

   StructArray<SsidEntry> ssids(0, 16);
   std::string summary;
   if (GetCachedCommandOutput(wirelessDomain, CMD_SHOW_SUMMARY, summary))
      BuildSsidsFromSummary(summary.c_str(), &ssids);

   std::string output;
   if (!GetCachedCommandOutput(wirelessDomain, CMD_SHOW_APS, output))
      return nullptr;

   return ParseAps(output.c_str(), apNameToMac.isEmpty() ? nullptr : &apNameToMac, ssids.isEmpty() ? nullptr : &ssids);
}

/**
 * Get wireless stations
 */
static ObjectArray<WirelessStationInfo> *GetWirelessStations(NObject *wirelessDomain)
{
   StructArray<ApMacMapping> apNameToMac(0, 32);
   BuildApNameToMac(wirelessDomain, &apNameToMac);
   StructArray<ClientApMapping> clientMacToApName(0, 64);
   BuildClientMacToApName(wirelessDomain, &clientMacToApName);
   StructArray<ApRadioMapping> apRadioChannels(0, 32);
   BuildApRadioChannels(wirelessDomain, &apRadioChannels);

   std::string output;
   if (!GetCachedCommandOutput(wirelessDomain, CMD_SHOW_CLIENTS, output))
      return nullptr;

   return ParseClients(output.c_str(), &apNameToMac, &clientMacToApName, &apRadioChannels);
}

/**
 * Get access point state
 */
static AccessPointState GetAccessPointState(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *serial, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   StructArray<ApMacMapping> apNameToMac(0, 32);
   BuildApNameToMac(wirelessDomain, &apNameToMac);

   std::string output;
   if (!GetCachedCommandOutput(wirelessDomain, CMD_SHOW_APS, output))
      return AP_UNKNOWN;

   ObjectArray<AccessPointInfo> *aps = ParseAps(output.c_str(), apNameToMac.isEmpty() ? nullptr : &apNameToMac, nullptr);
   if (aps == nullptr)
      return AP_UNKNOWN;

   AccessPointState state = AP_UNKNOWN;
   for(int i = 0; i < aps->size(); i++)
   {
      AccessPointInfo *ap = aps->get(i);
      if (ap->getMacAddr().equals(macAddr))
      {
         state = ap->getState();
         break;
      }
   }
   delete aps;

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
   StructArray<ApMacMapping> apNameToMac(0, 32);
   BuildApNameToMac(wirelessDomain, &apNameToMac);
   StructArray<ClientApMapping> clientMacToApName(0, 64);
   BuildClientMacToApName(wirelessDomain, &clientMacToApName);
   StructArray<ApRadioMapping> apRadioChannels(0, 32);
   BuildApRadioChannels(wirelessDomain, &apRadioChannels);

   std::string output;
   if (!GetCachedCommandOutput(wirelessDomain, CMD_SHOW_CLIENTS, output))
      return nullptr;

   ObjectArray<WirelessStationInfo> *stations = ParseClients(output.c_str(), &apNameToMac, &clientMacToApName, &apRadioChannels);
   if (stations == nullptr)
      return nullptr;

   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(stations->size(), 16, Ownership::True);
   for(int i = 0; i < stations->size(); i++)
   {
      WirelessStationInfo *ws = stations->get(i);
      if (!memcmp(ws->bssid, macAddr.value(), MAC_ADDR_LENGTH))
         wsList->add(MemCopyBlock(ws, sizeof(WirelessStationInfo)));
   }
   delete stations;

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
