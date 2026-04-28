/*
** NetXMS - Network Management System
** Notification channel driver for Matrix messenger
** Copyright (C) 2026 Raden Solutions
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
** File: matrix.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.matrix")

/**
 * Cached room alias entry
 */
struct RoomAliasCacheEntry
{
   char *roomId;
   time_t timestamp;

   RoomAliasCacheEntry(const char *id)
   {
      roomId = MemCopyStringA(id);
      timestamp = time(nullptr);
   }

   ~RoomAliasCacheEntry()
   {
      MemFree(roomId);
   }
};

/**
 * Proxy info structure
 */
struct ProxyInfo
{
   char hostname[256];
   uint16_t port;
   uint16_t protocol;
   char user[128];
   char password[256];
};

/**
 * Get proxy protocol code from name
 */
static uint16_t ProxyProtocolCodeFromName(const char *protocolName)
{
   if (!stricmp(protocolName, "http"))
      return CURLPROXY_HTTP;
#if HAVE_DECL_CURLPROXY_HTTPS
   if (!stricmp(protocolName, "https"))
      return CURLPROXY_HTTPS;
#endif
   if (!stricmp(protocolName, "socks4"))
      return CURLPROXY_SOCKS4;
   if (!stricmp(protocolName, "socks4a"))
      return CURLPROXY_SOCKS4A;
   if (!stricmp(protocolName, "socks5"))
      return CURLPROXY_SOCKS5;
   if (!stricmp(protocolName, "socks5h"))
      return CURLPROXY_SOCKS5_HOSTNAME;
   return 0xFFFF;
}

/**
 * Set proxy options on CURL handle
 */
static void SetCurlProxy(CURL *curl, const ProxyInfo *proxy)
{
   curl_easy_setopt(curl, CURLOPT_PROXY, proxy->hostname);
   if (proxy->port != 0)
      curl_easy_setopt(curl, CURLOPT_PROXYPORT, (long)proxy->port);
   if (proxy->protocol != 0x7FFF)
      curl_easy_setopt(curl, CURLOPT_PROXYTYPE, (long)proxy->protocol);
   if (proxy->user[0] != 0)
      curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxy->user);
   if (proxy->password[0] != 0)
      curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, proxy->password);
}

/**
 * Parse proxy configuration entry
 */
static bool ParseProxyEntry(ConfigEntry *entry, ProxyInfo *proxy)
{
   memset(proxy, 0, sizeof(ProxyInfo));
   proxy->protocol = 0x7FFF;

   const wchar_t *hostname = entry->getSubEntryValue(L"Hostname");
   if ((hostname == nullptr) || (*hostname == 0))
   {
      wchar_to_utf8(entry->getName(), -1, proxy->hostname, 256);
   }
   else
   {
      wchar_to_utf8(hostname, -1, proxy->hostname, 256);
   }
   proxy->port = static_cast<uint16_t>(entry->getSubEntryValueAsUInt(L"Port", 0, 0));

   const wchar_t *proxyType = entry->getSubEntryValue(L"Type", 0, L"http");
   char protocol[8];
   wchar_to_utf8(proxyType, -1, protocol, sizeof(protocol));
   proxy->protocol = ProxyProtocolCodeFromName(protocol);
   if (proxy->protocol == 0xFFFF)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Unsupported proxy type %s for proxy %s", proxyType, hostname);
      proxy->protocol = 0x7FFF;
   }

   const wchar_t *user = entry->getSubEntryValue(L"User");
   if (user != nullptr)
      wchar_to_utf8(user, -1, proxy->user, sizeof(proxy->user));

   const wchar_t *password = entry->getSubEntryValue(L"Password");
   if (password != nullptr)
      wchar_to_utf8(password, -1, proxy->password, sizeof(proxy->password));

   return true;
}

/**
 * Matrix driver class
 */
class MatrixDriver : public NCDriver
{
private:
   char m_serverUrl[1024];
   char m_accessToken[256];
   bool m_htmlFormatting;
   int64_t m_txnId;
   Mutex m_txnLock;
   NCDriverStorageManager *m_storageManager;
   StructArray<ProxyInfo> m_proxies;
   StringObjectMap<RoomAliasCacheEntry> m_aliasCache;
   Mutex m_aliasCacheLock;
   uint32_t m_aliasCacheTTL;

   MatrixDriver(NCDriverStorageManager *storageManager) : NCDriver(), m_txnLock(MutexType::FAST), m_aliasCache(Ownership::True), m_aliasCacheLock(MutexType::FAST)
   {
      m_serverUrl[0] = 0;
      m_accessToken[0] = 0;
      m_htmlFormatting = false;
      m_txnId = 0;
      m_storageManager = storageManager;
      m_aliasCacheTTL = 3600;
   }

   int64_t nextTxnId();
   json_t *doRequest(const char *method, const char *url, json_t *payload);
   char *resolveRoomAlias(const char *alias);
   char *getRoomId(const TCHAR *recipient);

public:
   virtual ~MatrixDriver();

   virtual int send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
   virtual bool checkHealth() override;

   static MatrixDriver *createInstance(Config *config, NCDriverStorageManager *storageManager);
};

/**
 * Destructor
 */
MatrixDriver::~MatrixDriver()
{
}

/**
 * Get next transaction ID (persistent across restarts)
 */
int64_t MatrixDriver::nextTxnId()
{
   m_txnLock.lock();
   m_txnId++;
   TCHAR value[32];
   IntegerToString(m_txnId, value);
   m_storageManager->set(_T("TxnId"), value);
   m_txnLock.unlock();
   return m_txnId;
}

/**
 * Perform HTTP request to Matrix API. Caller must json_decref() the result.
 */
json_t *MatrixDriver::doRequest(const char *method, const char *url, json_t *payload)
{
   int proxyCount = m_proxies.size();
   int totalAttempts = (proxyCount > 0) ? proxyCount : 1;

   for(int attempt = 0; attempt < totalAttempts; attempt++)
   {
      const ProxyInfo *proxy = (proxyCount > 0) ? m_proxies.get(attempt) : nullptr;

      CURL *curl = curl_easy_init();
      if (curl == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
         return nullptr;
      }

#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

#if HAVE_DECL_CURLOPT_PROTOCOLS_STR
      curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
      curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif

      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Matrix Driver/" NETXMS_VERSION_STRING_A);

      ByteStream responseData(32768);
      responseData.setAllocationStep(32768);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

      if (proxy != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Using proxy %hs:%u (attempt %d/%d)"), proxy->hostname, proxy->port, attempt + 1, totalAttempts);
         SetCurlProxy(curl, proxy);
      }

      // Set authorization header
      struct curl_slist *headers = nullptr;
      char authHeader[300];
      snprintf(authHeader, sizeof(authHeader), "Authorization: Bearer %s", m_accessToken);
      headers = curl_slist_append(headers, authHeader);
      headers = curl_slist_append(headers, "Content-Type: application/json");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

      // Set HTTP method
      if (!strcmp(method, "PUT"))
         curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

      // Set request body
      char *json = nullptr;
      if (payload != nullptr)
      {
         json = json_dumps(payload, 0);
         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Request body: %hs"), json);
      }

      char errorBuffer[CURL_ERROR_SIZE];
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
      curl_easy_setopt(curl, CURLOPT_URL, url);

      json_t *result = nullptr;
      bool allowRetry = false;

      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         long httpCode = 0;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
         nxlog_debug_tag(DEBUG_TAG, 7, _T("HTTP %hs %hs -> %d (%d bytes)"), method, url, static_cast<int>(httpCode), static_cast<int>(responseData.size()));

         if (responseData.size() > 0)
         {
            responseData.write('\0');
            json_error_t error;
            result = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
            if (result == nullptr)
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON response from %hs: %hs"), url, error.text);
         }

         allowRetry = (httpCode >= 500) && (httpCode <= 599);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Request to %hs failed: %hs"), url, errorBuffer);
         allowRetry = (rc == CURLE_COULDNT_RESOLVE_PROXY) || (rc == CURLE_COULDNT_RESOLVE_HOST) ||
                      (rc == CURLE_COULDNT_CONNECT) || (rc == CURLE_OPERATION_TIMEDOUT) ||
                      (rc == CURLE_SSL_CONNECT_ERROR) || (rc == CURLE_SEND_ERROR) || (rc == CURLE_RECV_ERROR);
      }

      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
      MemFree(json);

      if (!allowRetry || (result != nullptr))
         return result;

      nxlog_debug_tag(DEBUG_TAG, 4, _T("Transport failure via %hs, trying next proxy"), (proxy != nullptr) ? proxy->hostname : "direct connection");
   }

   return nullptr;
}

/**
 * Resolve room alias (e.g. #room:server) to room ID (!xxx:server).
 * Caller must MemFree() the result.
 */
char *MatrixDriver::resolveRoomAlias(const char *alias)
{
   CURL *curlEncode = curl_easy_init();
   if (curlEncode == nullptr)
      return nullptr;
   char *encoded = curl_easy_escape(curlEncode, alias, 0);
   curl_easy_cleanup(curlEncode);

   char url[2048];
   snprintf(url, sizeof(url), "%s/_matrix/client/v3/directory/room/%s", m_serverUrl, encoded);
   curl_free(encoded);

   json_t *response = doRequest("GET", url, nullptr);
   if (response == nullptr)
      return nullptr;

   char *roomId = nullptr;
   const char *id = json_string_value(json_object_get(response, "room_id"));
   if (id != nullptr)
   {
      roomId = MemCopyStringA(id);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Resolved alias %hs to %hs"), alias, roomId);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot resolve room alias %hs"), alias);
   }
   json_decref(response);
   return roomId;
}

/**
 * Get room ID from recipient string. Handles both room IDs and aliases.
 * Room aliases are cached with configurable TTL.
 * Caller must MemFree() the result.
 */
char *MatrixDriver::getRoomId(const TCHAR *recipient)
{
#ifdef UNICODE
   char *recipientUtf8 = MBStringFromWideString(recipient);
#else
   char *recipientUtf8 = MemCopyStringA(recipient);
#endif

   // If it starts with '!' it's already a room ID
   if (recipientUtf8[0] != '#')
      return recipientUtf8;

   // Check alias cache
   m_aliasCacheLock.lock();
   RoomAliasCacheEntry *cached = m_aliasCache.get(recipient);
   if (cached != nullptr)
   {
      time_t now = time(nullptr);
      if (now - cached->timestamp < m_aliasCacheTTL)
      {
         char *roomId = MemCopyStringA(cached->roomId);
         m_aliasCacheLock.unlock();
         MemFree(recipientUtf8);
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Room alias cache hit for %s -> %hs"), recipient, roomId);
         return roomId;
      }
      // Expired - remove from cache
      m_aliasCache.remove(recipient);
   }
   m_aliasCacheLock.unlock();

   // Resolve alias via API
   char *roomId = resolveRoomAlias(recipientUtf8);
   MemFree(recipientUtf8);

   if (roomId != nullptr)
   {
      m_aliasCacheLock.lock();
      m_aliasCache.set(recipient, new RoomAliasCacheEntry(roomId));
      m_aliasCacheLock.unlock();
   }

   return roomId;
}

/**
 * Send notification
 */
int MatrixDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Sending to %s: \"%s\""), recipient, body);

   char *roomId = getRoomId(recipient);
   if (roomId == nullptr)
      return -1;

   // URL-encode room ID for the URL path
   CURL *curlEncode = curl_easy_init();
   if (curlEncode == nullptr)
   {
      MemFree(roomId);
      return -1;
   }
   char *encodedRoomId = curl_easy_escape(curlEncode, roomId, 0);
   curl_easy_cleanup(curlEncode);
   MemFree(roomId);

   // Build URL with unique transaction ID
   int64_t txn = nextTxnId();
   char url[2048];
   snprintf(url, sizeof(url), "%s/_matrix/client/v3/rooms/%s/send/m.room.message/nxms" INT64_FMTA, m_serverUrl, encodedRoomId, txn);
   curl_free(encodedRoomId);

   // Build message JSON
   json_t *message = json_object();

   json_object_set_new(message, "msgtype", json_string("m.text"));
   if (m_htmlFormatting)
   {
      // Plain text fallback
      StringBuffer plainText;
      if (subject != nullptr && *subject != 0)
      {
         plainText.append(subject);
         plainText.append(_T("\n\n"));
      }
      plainText.append(body);
      json_object_set_new(message, "body", json_string_t(plainText));

      // HTML formatted version
      StringBuffer htmlBody;
      if (subject != nullptr && *subject != 0)
      {
         htmlBody.append(_T("<strong>"));
         htmlBody.append(subject);
         htmlBody.append(_T("</strong><br/><br/>"));
      }
      htmlBody.append(body);
      json_object_set_new(message, "format", json_string("org.matrix.custom.html"));
      json_object_set_new(message, "formatted_body", json_string_t(htmlBody));
   }
   else
   {
      StringBuffer text;
      if (subject != nullptr && *subject != 0)
      {
         text.append(subject);
         text.append(_T("\n\n"));
      }
      text.append(body);
      json_object_set_new(message, "body", json_string_t(text));
   }

   json_t *response = doRequest("PUT", url, message);
   json_decref(message);

   int result;
   if (response != nullptr)
   {
      const char *eventId = json_string_value(json_object_get(response, "event_id"));
      if (eventId != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Message sent to %s, event ID: %hs"), recipient, eventId);
         result = 0;
      }
      else
      {
         const char *errcode = json_string_value(json_object_get(response, "errcode"));
         if (errcode != nullptr && !strcmp(errcode, "M_LIMIT_EXCEEDED"))
         {
            int retryMs = static_cast<int>(json_integer_value(json_object_get(response, "retry_after_ms")));
            result = (retryMs > 0) ? (retryMs / 1000 + 1) : 30;
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Rate limited sending to %s, retry in %d seconds"), recipient, result);
         }
         else
         {
            const char *error = json_string_value(json_object_get(response, "error"));
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to send message to %s: %hs (%hs)"),
               recipient, (error != nullptr) ? error : "unknown error", (errcode != nullptr) ? errcode : "?");
            result = -1;
         }
      }
      json_decref(response);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("No response from server when sending to %s"), recipient);
      result = -1;
   }

   return result;
}

/**
 * Check driver health by calling /_matrix/client/v3/account/whoami
 */
bool MatrixDriver::checkHealth()
{
   char url[2048];
   snprintf(url, sizeof(url), "%s/_matrix/client/v3/account/whoami", m_serverUrl);

   json_t *response = doRequest("GET", url, nullptr);
   if (response == nullptr)
      return false;

   bool healthy = (json_object_get(response, "user_id") != nullptr);
   if (healthy)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Health check OK: %hs"),
         json_string_value(json_object_get(response, "user_id")));
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Health check failed"));
   }
   json_decref(response);
   return healthy;
}

/**
 * Create driver instance from configuration
 */
MatrixDriver *MatrixDriver::createInstance(Config *config, NCDriverStorageManager *storageManager)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new driver instance"));

   char serverUrl[1024] = "";
   char accessToken[256] = "";
   bool htmlFormatting = false;
   uint32_t aliasCacheTTL = 3600;

   ProxyInfo proxy;
   memset(&proxy, 0, sizeof(ProxyInfo));
   proxy.protocol = 0x7FFF;
   char protocol[8] = "http";

   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("AccessToken"), CT_MB_STRING, 0, 0, sizeof(accessToken), 0, accessToken, nullptr },
      { _T("AliasCacheTTL"), CT_LONG, 0, 0, 0, 0, &aliasCacheTTL, nullptr },
      { _T("HtmlFormatting"), CT_BOOLEAN, 0, 0, 0, 0, &htmlFormatting, nullptr },
      { _T("Proxy"), CT_MB_STRING, 0, 0, sizeof(proxy.hostname), 0, proxy.hostname, nullptr },
      { _T("ProxyPort"), CT_WORD, 0, 0, 0, 0, &proxy.port, nullptr },
      { _T("ProxyType"), CT_MB_STRING, 0, 0, sizeof(protocol), 0, protocol, nullptr },
      { _T("ProxyUser"), CT_MB_STRING, 0, 0, sizeof(proxy.user), 0, proxy.user, nullptr },
      { _T("ProxyPassword"), CT_MB_STRING, 0, 0, sizeof(proxy.password), 0, proxy.password, nullptr },
      { _T("ServerURL"), CT_MB_STRING, 0, 0, sizeof(serverUrl), 0, serverUrl, nullptr },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
   };

   if (!config->parseTemplate(_T("Matrix"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
      return nullptr;
   }

   if (serverUrl[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Matrix server URL is not configured"));
      return nullptr;
   }

   // Strip trailing slash
   size_t len = strlen(serverUrl);
   if ((len > 0) && (serverUrl[len - 1] == '/'))
      serverUrl[len - 1] = 0;

   if (accessToken[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Matrix access token is not configured"));
      return nullptr;
   }

   proxy.protocol = ProxyProtocolCodeFromName(protocol);
   if (proxy.protocol == 0xFFFF)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unsupported proxy type %hs"), protocol);
      return nullptr;
   }

   // Build proxy list
   StructArray<ProxyInfo> proxies;

   if (proxy.hostname[0] != 0)
   {
      proxies.add(proxy);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Added primary proxy %hs"), proxy.hostname);
   }

   unique_ptr<ObjectArray<ConfigEntry>> proxyEntries = config->getOrderedSubEntries(L"/FallbackProxy", L"*");
   if (proxyEntries != nullptr)
   {
      for(int i = 0; i < proxyEntries->size(); i++)
      {
         ProxyInfo additionalProxy;
         if (ParseProxyEntry(proxyEntries->get(i), &additionalProxy))
         {
            proxies.add(additionalProxy);
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Added fallback proxy %hs"), additionalProxy.hostname);
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Invalid fallback proxy entry"));
         }
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Configured %d proxy(ies)"), proxies.size());

   // Create driver instance
   MatrixDriver *driver = new MatrixDriver(storageManager);
   strlcpy(driver->m_serverUrl, serverUrl, sizeof(driver->m_serverUrl));
   strlcpy(driver->m_accessToken, accessToken, sizeof(driver->m_accessToken));
   driver->m_htmlFormatting = htmlFormatting;
   driver->m_aliasCacheTTL = aliasCacheTTL;
   for(int i = 0; i < proxies.size(); i++)
      driver->m_proxies.add(*proxies.get(i));

   // Restore persistent transaction ID
   TCHAR *txnValue = storageManager->get(_T("TxnId"));
   if (txnValue != nullptr)
   {
      driver->m_txnId = _tcstoll(txnValue, nullptr, 10);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Restored transaction ID: ") INT64_FMT, driver->m_txnId);
      MemFree(txnValue);
   }

   // Validate connection by calling whoami
   char url[2048];
   snprintf(url, sizeof(url), "%s/_matrix/client/v3/account/whoami", serverUrl);
   json_t *response = driver->doRequest("GET", url, nullptr);
   if (response != nullptr)
   {
      const char *userId = json_string_value(json_object_get(response, "user_id"));
      if (userId != nullptr)
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Matrix driver instantiated for user %hs on %hs"), userId, serverUrl);
      }
      else
      {
         const char *errcode = json_string_value(json_object_get(response, "errcode"));
         const char *error = json_string_value(json_object_get(response, "error"));
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Matrix API authentication failed: %hs (%hs)"),
            (error != nullptr) ? error : "unknown error", (errcode != nullptr) ? errcode : "?");
         json_decref(response);
         delete driver;
         return nullptr;
      }
      json_decref(response);
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot connect to Matrix server %hs"), serverUrl);
      delete driver;
      return nullptr;
   }

   return driver;
}

/**
 * Configuration template
 */
static const NCConfigurationTemplate s_config(true, true);

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Matrix, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return MatrixDriver::createInstance(config, storageManager);
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
