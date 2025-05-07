/* 
** NetXMS - Network Management System
** Notification channel driver for Telegram messenger
** Copyright (C) 2014-2025 Raden Solutions
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
** File: telegram.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("ncd.telegram")

#define DISABLE_IP_V4 1
#define DISABLE_IP_V6 2
#define LONG_POLLING  4

/**
 * Chat information
 */
struct Chat
{
   int64_t id;
   TCHAR *userName;
   TCHAR *firstName;
   TCHAR *lastName;
   StringMap topics;

   /**
    * Create from Telegram server message
    */
   Chat(json_t *json)
   {
      id = json_object_get_int64(json, "id", -1);
      firstName = json_object_get_string_t(json, "first_name", _T(""));
      lastName = json_object_get_string_t(json, "last_name", _T(""));
      const char *type = json_object_get_string_utf8(json, "type", "unknown");
      userName = json_object_get_string_t(json, (!strcmp(type, "group") || !strcmp(type, "channel")) ? "title" : "username", _T(""));
   }

   /**
    * Create from channel persistent storage entry
    */
   Chat(const TCHAR *key, const TCHAR *value)
   {
      const TCHAR *p = _tcschr(key, _T('.'));
      if (p != nullptr)
      {
         p++;
         id = _tcstoll(p, nullptr, 10);
      }
      else
      {
         id = 0;
      }

      p = value;
      firstName = extractSubstring(&p);
      lastName = extractSubstring(&p);
      userName = extractSubstring(&p);
   }

   /**
    * Destructor
    */
   ~Chat()
   {
      MemFree(userName);
      MemFree(firstName);
      MemFree(lastName);
   }

   /**
    * Save to channel persistent storage
    */
   void save(NCDriverStorageManager *storageManager)
   {
      TCHAR key[64], value[2000];
      _sntprintf(key, 64, _T("Chat.") INT64_FMT, id);
      _sntprintf(value, 2000, _T("%d/%s%d/%s%d/%s"), static_cast<int>(_tcslen(firstName)), firstName,
            static_cast<int>(_tcslen(lastName)), lastName, static_cast<int>(_tcslen(userName)), userName);
      storageManager->set(key, value);

      topics.forEach(
         [this, storageManager] (const TCHAR *key, const void *value) -> EnumerationCallbackResult
         {
            TCHAR skey[64];
            int64_t topicId = _tcstoll(static_cast<const TCHAR*>(value), nullptr, 10);
            _sntprintf(skey, 64, _T("Topic.") INT64_FMT _T(".") INT64_FMT, id, topicId);
            storageManager->set(skey, key);
            return _CONTINUE;
         });
   }

   /**
    * Remove channel from persistent storage
    */
   void remove(NCDriverStorageManager *storageManager)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("Chat.") INT64_FMT, id);
      storageManager->clear(key);
      topics.forEach(
         [this, storageManager] (const TCHAR *key, const void *value) -> EnumerationCallbackResult
         {
            TCHAR skey[64];
            int64_t topicId = _tcstoll(static_cast<const TCHAR*>(value), nullptr, 10);
            _sntprintf(skey, 64, _T("Topic.") INT64_FMT _T(".") INT64_FMT, id, topicId);
            storageManager->clear(skey);
            return _CONTINUE;
         });
   }

   /**
    * Extract substring from given position
    */
   static TCHAR *extractSubstring(const TCHAR **start)
   {
      TCHAR *eptr;
      int l = _tcstol(*start, &eptr, 10);
      if (*eptr != _T('/'))
         return MemCopyString(_T(""));
      eptr++;
      TCHAR *s = MemAllocString(l + 1);
      memcpy(s, eptr, l * sizeof(TCHAR));
      s[l] = 0;
      *start = eptr + l;
      return s;
   }
};

/**
 * Proxy info structure
 */
struct ProxyInfo
{
   char hostname[MAX_PATH];
   uint16_t port;
   uint16_t protocol;
   char user[128];
   char password[256];
};

/**
 * API call response
 */
struct CallResponse
{
   int statusCode;
   bool allowRetry;
   json_t *data;

   CallResponse()
   {
      statusCode = 400;
      allowRetry = false;
      data = nullptr;
   }
};

/**
 * Telegram driver class
 */
class TelegramDriver : public NCDriver
{
private:
   THREAD m_updateHandlerThread;
   char m_authToken[64];
   long m_ipVersion;
   ProxyInfo *m_proxy;
   TCHAR *m_botName;
   StringObjectMap<Chat> m_chats;
   Mutex m_chatsLock;
   Condition m_shutdownCondition;
   bool m_shutdownFlag;
   int64_t m_nextUpdateId;
   NCDriverStorageManager *m_storageManager;   
   TCHAR m_parseMode[32];
   bool m_longPollingMode;
   uint32_t m_pollingInterval;

   TelegramDriver(NCDriverStorageManager *storageManager) : NCDriver(), m_chats(Ownership::True), m_chatsLock(MutexType::FAST), m_shutdownCondition(true)
   {
      m_updateHandlerThread = INVALID_THREAD_HANDLE;
      memset(m_authToken, 0, sizeof(m_authToken));
      m_ipVersion = CURL_IPRESOLVE_WHATEVER;
      m_proxy = nullptr;
      m_botName = nullptr;
      m_shutdownFlag = false;
      m_nextUpdateId = 0;
      m_storageManager = storageManager;
      m_longPollingMode = true;
      m_pollingInterval = 300;
   }

   static void updateHandler(TelegramDriver *driver);

public:
   virtual ~TelegramDriver();

   virtual int send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;

   virtual bool checkHealth() override;

   bool isShutdown() const { return m_shutdownFlag; }
   void processUpdate(json_t *data);

   static TelegramDriver *createInstance(Config *config, NCDriverStorageManager *storageManager);
};

/**
 * Driver destructor
 */
TelegramDriver::~TelegramDriver()
{
   m_shutdownFlag = true;
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Waiting for update handler thread completion for bot %s"), m_botName);
   ThreadJoin(m_updateHandlerThread);
   MemFree(m_botName);
   MemFree(m_proxy);
}

/**
 * Send request to Telegram API
 */
static CallResponse SendTelegramRequest(const char *token, const ProxyInfo *proxy, long ipVersion, const char *method, json_t *data)
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return CallResponse();
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Telegram Driver/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
   curl_easy_setopt(curl, CURLOPT_IPRESOLVE, ipVersion);

   if (proxy != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SendTelegramRequest: using proxy %hs"), proxy->hostname);
      curl_easy_setopt(curl, CURLOPT_PROXY, proxy->hostname);
      if (proxy->port != 0)
      {
         curl_easy_setopt(curl, CURLOPT_PROXYPORT, (long)proxy->port);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SendTelegramRequest: proxy port %d"), proxy->port);
      }
      if (proxy->protocol != 0x7FFF)
      {
         curl_easy_setopt(curl, CURLOPT_PROXYTYPE, (long)proxy->protocol);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SendTelegramRequest: proxy type %d"), proxy->protocol);
      }
      if (proxy->user[0] != 0)
      {
         curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxy->user);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SendTelegramRequest: proxy login %hs"), proxy->user);
      }
      if (proxy->password[0] != 0)
      {
         curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, proxy->password);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SendTelegramRequest: proxy password set"));
      }
   }

   struct curl_slist *headers = nullptr;
   char *json;
   if (data != nullptr)
   {
      json = json_dumps(data, 0);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
      headers = curl_slist_append(headers, "Content-Type: application/json");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Request to send: %hs"), json);
   }
   else
   {
      json = nullptr;
   }

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   CallResponse response;

   char url[256];
   snprintf(url, 256, "https://api.telegram.org/bot%s/%s", token, method);
   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         long responseCode;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("HTTP response %03d, %d bytes"), static_cast<int>(responseCode), static_cast<int>(responseData.size()));
         response.statusCode = static_cast<int>(responseCode);
         response.allowRetry = ((responseCode >= 500) && (responseCode <= 599));

         if (responseData.size() > 0)
         {
            responseData.write('\0');
            json_error_t error;
            response.data = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
            if (response.data == nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot parse API response (%hs)"), error.text);
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Empty API response (HTTP response status code %03d)"), static_cast<int>(responseCode));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errorBuffer);
         response.allowRetry = (rc == CURLE_COULDNT_RESOLVE_PROXY) || (rc == CURLE_COULDNT_RESOLVE_HOST) || (rc == CURLE_COULDNT_CONNECT) ||
                  (rc == CURLE_OPERATION_TIMEDOUT) || (rc == CURLE_SSL_CONNECT_ERROR) || (rc == CURLE_SEND_ERROR) || (rc == CURLE_RECV_ERROR);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
   }
   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(json);

   return response;
}

/**
 * Restore chats from persistent data
 */
static EnumerationCallbackResult RestoreChats(const TCHAR *key, const TCHAR *value, StringObjectMap<Chat> *chats)
{
   if (_tcsncmp(key, _T("Chat."), 5))
      return _CONTINUE; // Not a chat record

   auto chat = new Chat(key, value);
   if ((chat->id != 0) && (chat->userName != nullptr) && (chat->userName[0] != 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Loaded chat object %s = ") INT64_FMT, chat->userName, chat->id);
      chats->set(chat->userName, chat);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Error loading chat object from storage entry \"%s\" = \"%s\""), key, value);
      delete chat;
   }
   return _CONTINUE;
}

/**
 * Restore topics from persistent data
 */
static EnumerationCallbackResult RestoreTopics(const TCHAR *key, const TCHAR *value, StringObjectMap<Chat> *chats)
{
   if (_tcsncmp(key, _T("Topic."), 6))
      return _CONTINUE; // Not a char record

   const TCHAR *p = _tcschr(&key[6], _T('.'));
   if (p == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Error loading topic object from storage entry \"%s\" = \"%s\""), key, value);
      return _CONTINUE;
   }

   TCHAR *eptr;
   int64_t chatId = _tcstoll(&key[6], &eptr, 10);
   if (eptr != p)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Error loading topic object from storage entry \"%s\" = \"%s\""), key, value);
      return _CONTINUE;
   }

   Chat *chat = (Chat*)chats->findElement(
      [chatId] (const TCHAR *name, const void *object) -> bool
      {
         return static_cast<const Chat*>(object)->id == chatId;
      });
   if (chat == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Error loading topic object from storage entry \"%s\" = \"%s\" (chat object not found)"), key, value);
      return _CONTINUE;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Loaded topic object %s = %s for chat %s"), value, p + 1, chat->userName);
   chat->topics.set(value, p + 1);
   return _CONTINUE;
}

/**
 * Get supported IP version from options
 */
static long IPVersionFromOptions(uint32_t options)
{
   if (options & DISABLE_IP_V4)
      return CURL_IPRESOLVE_V6;
   if (options & DISABLE_IP_V6)
      return CURL_IPRESOLVE_V4;
   return CURL_IPRESOLVE_WHATEVER;
}

/**
 * Get proxy protocol code from name
 */
uint16_t ProxyProtocolCodeFromName(const char *protocolName)
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
 * Create driver instance
 */
TelegramDriver *TelegramDriver::createInstance(Config *config, NCDriverStorageManager *storageManager)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new driver instance"));

   ProxyInfo proxy;
   memset(&proxy, 0, sizeof(ProxyInfo));
   proxy.protocol = 0x7FFF;   // "unset" indicator

   char authToken[64];
   char protocol[8] = "http";
   uint32_t options = LONG_POLLING;
   uint32_t pollingInterval = 300;
   TCHAR parseMode[32] = _T("");
   NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("AuthToken"), CT_MB_STRING, 0, 0, sizeof(authToken), 0, authToken, nullptr },
		{ _T("DisableIPv4"), CT_BOOLEAN_FLAG_32, 0, 0, DISABLE_IP_V4, 0, &options, nullptr },
		{ _T("DisableIPv6"), CT_BOOLEAN_FLAG_32, 0, 0, DISABLE_IP_V6, 0, &options, nullptr },
      { _T("LongPolling"), CT_BOOLEAN_FLAG_32, 0, 0, LONG_POLLING, 0, &options, nullptr },
      { _T("PollingInterval"), CT_LONG, 0, 0, 0, 0, &pollingInterval, nullptr },
		{ _T("Proxy"), CT_MB_STRING, 0, 0, sizeof(proxy.hostname), 0, proxy.hostname, nullptr },
		{ _T("ProxyPort"), CT_WORD, 0, 0, 0, 0, &proxy.port, nullptr },
		{ _T("ProxyType"), CT_MB_STRING, 0, 0, sizeof(protocol), 0, protocol, nullptr },
		{ _T("ProxyUser"), CT_MB_STRING, 0, 0, sizeof(proxy.user), 0, proxy.user, nullptr },
		{ _T("ProxyPassword"), CT_MB_STRING, 0, 0, sizeof(proxy.password), 0, proxy.password, nullptr },
      { _T("ParseMode"), CT_STRING, 0, 0, sizeof(parseMode), 0, parseMode, nullptr },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
	};

   if (!config->parseTemplate(_T("Telegram"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
      return nullptr;
   }

   if ((options & (DISABLE_IP_V4 | DISABLE_IP_V6)) == (DISABLE_IP_V4 | DISABLE_IP_V6))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Inconsistent configuration - both IPv4 and IPv6 are disabled"));
      return nullptr;
   }

   proxy.protocol = ProxyProtocolCodeFromName(protocol);
   if (proxy.protocol == 0xFFFF)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unsupported proxy type %hs"), protocol);
      return nullptr;
   }

   TelegramDriver *driver = nullptr;
   CallResponse response = SendTelegramRequest(authToken, &proxy, IPVersionFromOptions(options), "getMe", nullptr);
   json_t *info = response.data;
	if (info != nullptr)
	{
	   json_t *ok = json_object_get(info, "ok");
	   if (json_is_true(ok))
	   {
	      nxlog_debug_tag(DEBUG_TAG, 2, _T("Received valid API response"));
	      json_t *result = json_object_get(info, "result");
	      if (json_is_object(result))
	      {
	         json_t *name = json_object_get(result, "first_name");
	         if (json_is_string(name))
	         {
	            driver = new TelegramDriver(storageManager);
	            strcpy(driver->m_authToken, authToken);
	            driver->m_longPollingMode = (options & LONG_POLLING) ? true : false;
	            driver->m_pollingInterval = pollingInterval;
	            driver->m_proxy = (proxy.hostname[0] != 0) ? MemCopyBlock(&proxy, sizeof(ProxyInfo)) : nullptr;
               driver->m_ipVersion = IPVersionFromOptions(options);
#ifdef UNICODE
	            driver->m_botName = WideStringFromUTF8String(json_string_value(name));
#else
               driver->m_botName = MBStringFromUTF8String(json_string_value(name));
#endif
               nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Telegram driver instantiated for bot %s"), driver->m_botName);

               StringMap *data = storageManager->getAll();
               data->forEach(RestoreChats, &driver->m_chats);
               data->forEach(RestoreTopics, &driver->m_chats);
               delete data;

               driver->m_updateHandlerThread = ThreadCreateEx(TelegramDriver::updateHandler, driver);
               _tcslcpy(driver->m_parseMode, parseMode, 32);
	         }
	         else
	         {
	            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Malformed response from Telegram API"));
	         }
	      }
	      else
	      {
	         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Malformed response from Telegram API"));
	      }
	   }
	   else
	   {
	      json_t *d = json_object_get(info, "description");
	      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Telegram API call failed (%hs), driver configuration could be incorrect"),
	               json_is_string(d) ? json_string_value(d) : "Unknown reason");
	   }
	   json_decref(info);
	}
	else
	{
	   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Telegram API call failed, driver configuration could be incorrect"));
	}
   return driver;
}

/**
 * cURL progress callback
 */
static int ProgressCallback(void *context, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
   return static_cast<TelegramDriver*>(context)->isShutdown();
}

#if LIBCURL_VERSION_NUM < 0x072000

/**
 * cURL progress callback - wrapper for cURL older than 7.32
 */
static int ProgressCallbackPreV_7_32(void *context, double dltotal, double dlnow, double ultotal, double ulnow)
{
   return ProgressCallback(context, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal, (curl_off_t)ulnow);
}

#endif

/**
 * Handler for incoming updates
 */
void TelegramDriver::updateHandler(TelegramDriver *driver)
{
   // Main loop
   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   while(!driver->m_shutdownFlag)
   {
      CURL *curl = curl_easy_init();
      if (curl == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateHandler(%s): Call to curl_easy_init() failed"), driver->m_botName);
         if (driver->m_shutdownCondition.wait(60000))
            break;
         continue;
      }

#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
#endif

      curl_easy_setopt(curl, CURLOPT_HEADER, 0L); // do not include header in data
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Telegram Driver/" NETXMS_VERSION_STRING_A);

      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
#if LIBCURL_VERSION_NUM < 0x072000
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressCallbackPreV_7_32);
      curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, driver);
#else
      curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
      curl_easy_setopt(curl, CURLOPT_XFERINFODATA, driver);
#endif
      curl_easy_setopt(curl, CURLOPT_IPRESOLVE, driver->m_ipVersion);

      if (driver->m_proxy != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("UpdateHandler(%s): proxy is enabled"), driver->m_botName);
         curl_easy_setopt(curl, CURLOPT_PROXY, driver->m_proxy->hostname);
         if (driver->m_proxy->port != 0)
            curl_easy_setopt(curl, CURLOPT_PROXYPORT, (long)driver->m_proxy->port);
         if (driver->m_proxy->protocol != 0x7FFF)
            curl_easy_setopt(curl, CURLOPT_PROXYTYPE, (long)driver->m_proxy->protocol);
         if (driver->m_proxy->user[0] != 0)
            curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, driver->m_proxy->user);
         if (driver->m_proxy->password[0] != 0)
            curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, driver->m_proxy->password);
      }

      // Inner loop while connection is active
      while(!driver->m_shutdownFlag)
      {
         if (!driver->m_longPollingMode)
         {
            if (SleepAndCheckForShutdown(driver->m_pollingInterval))
               break;
         }

         char url[256];
         snprintf(url, 256, "https://api.telegram.org/bot%s/getUpdates?timeout=%u&offset=" INT64_FMTA,
               driver->m_authToken, driver->m_longPollingMode ? driver->m_pollingInterval : 0, driver->m_nextUpdateId);
         if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
         {
            if (curl_easy_perform(curl) == CURLE_OK)
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("UpdateHandler(%s): got %d bytes"), driver->m_botName, static_cast<int>(responseData.size()));
               if (responseData.size() > 0)
               {
                  responseData.write('\0');
                  json_error_t error;
                  json_t *data = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
                  if (data != nullptr)
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("UpdateHandler(%s): valid JSON document received"), driver->m_botName);
                     driver->processUpdate(data);
                     json_decref(data);
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateHandler(%s): Cannot parse API response (%hs)"), driver->m_botName, error.text);
                  }
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateHandler(%s): Call to curl_easy_perform() failed"), driver->m_botName);
               break;
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateHandler(%s): Call to curl_easy_setopt(CURLOPT_URL) failed"), driver->m_botName);
            break;
         }
         responseData.clear();
      }

      curl_easy_cleanup(curl);
      responseData.clear();
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Update handler thread for Telegram bot %s stopped"), driver->m_botName);
}

/**
 * Process update message from Telegram server
 */
void TelegramDriver::processUpdate(json_t *data)
{
   if (nxlog_get_debug_level_tag(DEBUG_TAG) > 7)
   {
      char *s = json_dumps(data, JSON_INDENT(3));
      nxlog_debug_tag(DEBUG_TAG, 8, _T("Received document: %hs"), s);
      MemFree(s);
   }

   json_t *ok = json_object_get(data, "ok");
   if (!json_is_true(ok))
      return;

   json_t *result = json_object_get(data, "result");
   if (!json_is_array(result))
      return;

   size_t count = json_array_size(result);
   for(size_t i = 0; i < count; i++)
   {
      json_t *update = json_array_get(result, i);
      if (!json_is_object(update))
         continue;

      int64_t id = json_object_get_int64(update, "update_id", -1);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Received update_id=") INT64_FMT, id);
      if (id >= m_nextUpdateId)
         m_nextUpdateId = id + 1;

      json_t *message = json_object_get(update, "message");
      if (!json_is_object(message))
      {
         message = json_object_get(update, "channel_post");
         if (!json_is_object(message))
            continue;
      }

      json_t *chat = json_object_get(message, "chat");
      if (!json_is_object(chat))
         continue;

      const char *type = json_object_get_string_utf8(chat, "type", "unknown");
      TCHAR *username = json_object_get_string_t(chat, (!strcmp(type, "group") || !strcmp(type, "channel") || !strcmp(type, "supergroup")) ? "title" : "username", nullptr);
      if (username == nullptr)
         continue;

      m_chatsLock.lock();
      Chat *chatObject = m_chats.get(username);
      if (chatObject != nullptr)
      {
         int64_t id = json_object_get_int64(chat, "id", -1);
         int64_t newId = json_object_get_int64(message, "migrate_to_chat_id", 0);
         if ((newId != 0) || (chatObject->id != id)) // newId != 0 means group migration to supergroup
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat ID change for %s: ") INT64_FMT _T(" -> ") INT64_FMT, username, chatObject->id, (newId != 0) ? newId : id);
            chatObject->remove(m_storageManager);
            chatObject->id = (newId != 0) ? newId : id;
            chatObject->save(m_storageManager);
         }
      }
      else // Create chat object
      {
         chatObject = new Chat(chat);
         m_chats.set(username, chatObject);
         chatObject->save(m_storageManager);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat ID registered: %s = ") INT64_FMT, username, chatObject->id);
      }
      uint64_t chatId = chatObject->id;

      int64_t topicId = json_object_get_int64(message, "message_thread_id", 0);
      if (topicId != 0)
      {
         json_t *topicName = json_object_get_by_path_a(message, "reply_to_message/forum_topic_created/name");
         if (json_is_string(topicName))
         {
            TCHAR key[256];
            utf8_to_tchar(json_string_value(topicName), -1, key, 256);
            const TCHAR *topicIdText = chatObject->topics.get(key);
            if (topicIdText != nullptr)
            {
               int64_t storedTopicId = _tcstoll(topicIdText, nullptr, 10);
               if (storedTopicId != topicId)
               {
                  TCHAR buffer[64];
                  IntegerToString(topicId, buffer);
                  chatObject->topics.set(key, buffer);
                  chatObject->save(m_storageManager);
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat ID change for %s/%s: ") INT64_FMT _T(" -> ") INT64_FMT, username, key, storedTopicId, topicId);
               }
            }
            else
            {
               TCHAR buffer[64];
               IntegerToString(topicId, buffer);
               chatObject->topics.set(key, buffer);
               chatObject->save(m_storageManager);
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Topic ID registered: %s/%s = ") INT64_FMT, username, key, topicId);
            }
         }
      }

      m_chatsLock.unlock();

      TCHAR *text = json_object_get_string_t(message, "text", _T(""));
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Received %hs message from %s (") INT64_FMT _T("): %s"), type, username, chatId, text);
      MemFree(text);

      MemFree(username);
   }
}

/**
 * Send notification
 */
int TelegramDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   int result = -1;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Sending to %s: \"%s\""), recipient, body);

   // Recipient name started with @ indicates public channel
   // In that case use channel name instead of chat ID
   int64_t chatId = 0;
   bool useRecipientName = recipient[0] == _T('@');
   if (!useRecipientName)
   {
      size_t numCount = _tcsspn((recipient[0] != _T('-')) ? recipient : (recipient + 1), _T("0123456789"));
      size_t textLen = _tcslen(recipient);
      useRecipientName = (recipient[0] == _T('-')) ? (numCount == (textLen - 1)) : (numCount == textLen);
   }

   int64_t topicId = 0;
   if ((subject != nullptr) && (*subject != 0))
   {
      topicId = _tcstoll(subject, nullptr, 10);
   }

   if (!useRecipientName)
   {
      m_chatsLock.lock();
      Chat *chatObject = m_chats.get(recipient);
      if (chatObject != nullptr)
      {
         chatId = chatObject->id;
         if ((subject != nullptr) && (*subject != 0) && (topicId == 0))
         {
            const TCHAR *id = chatObject->topics.get(subject);
            if (id != nullptr)
               topicId = _tcstoll(id, nullptr, 10);
         }
      }
      m_chatsLock.unlock();
   }

   if ((chatId != 0) || useRecipientName)
   {
      json_t *request = json_object();
      json_object_set_new(request, "chat_id", useRecipientName ? json_string_t(recipient) : json_integer(chatId));
      json_object_set_new(request, "text", json_string_t(body));
      if (*m_parseMode != 0)
      {
         json_object_set_new(request, "parse_mode", json_string_t(m_parseMode));
      }
      if (topicId != 0)
      {
         json_object_set_new(request, "message_thread_id", json_integer(topicId));
      }

      CallResponse response = SendTelegramRequest(m_authToken, m_proxy, m_ipVersion, "sendMessage", request);
      json_decref(request);

      if (json_is_object(response.data))
      {
         if (json_is_true(json_object_get(response.data, "ok")))
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Message from bot %s to recipient %s successfully sent"), m_botName, recipient);
            result = 0;
         }
         else
         {
            int errorCode = json_object_get_int32(response.data, "error_code", response.statusCode);
            switch (errorCode)
            {
               case 420: // FLOOD
               case 429: // Too many requests
                  result = json_object_get_int32(json_object_get(response.data, "parameters"), "retry_after", 15);
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Too many requests, retry is allowed in %d seconds (message from bot %s to recipient %s)"), result, m_botName, recipient);
                  break;
               default:
                  result = -1;
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot send message from bot %s to recipient %s: API error (%hs)"),
                           m_botName, recipient, json_object_get_string_utf8(response.data, "description", "Unknown reason"));
                  break;
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot send message from bot %s to recipient %s: invalid API response (HTTP response status code %03d)"), m_botName, recipient, response.statusCode);
         if (response.allowRetry)
            result = 60;   // Retry in 60 seconds if allowed by SendTelegramRequest
      }
      json_decref(response.data);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot find chat ID for recipient %s and bot %s"), recipient, m_botName);
   }
   return result;
}

/**
 * Returns driver health status
 */
bool TelegramDriver::checkHealth()
{
   bool status = false;
   CallResponse response = SendTelegramRequest(m_authToken, m_proxy, m_ipVersion, "getMe", nullptr);
   if (response.data != nullptr)
   {
      json_t *ok = json_object_get(response.data, "ok");
      if (json_is_true(ok))
      {
         status = true;
      }
      json_decref(response.data);
   }
   return status;
}

/**
 * Configuration template
 */
static const NCConfigurationTemplate s_config(true, true);

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Telegram, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return TelegramDriver::createInstance(config, storageManager);
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
