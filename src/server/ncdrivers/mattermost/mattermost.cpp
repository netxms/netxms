/* 
** NetXMS - Network Management System
** Notification channel and chat bot driver for Mattermost
** Copyright (C) 2024-2026 Raden Solutions
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
** File: mattermost.cpp
**
**/

#include <ncdrv.h>
#include <chatdrv.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <netxms-version.h>
#include <nxlibcurl.h>
#include <nxjson.h>
#include <nxmarkdown.h>
#include <websocket.h>

#define DEBUG_TAG _T("ncd.mattermost")

#define MM_USE_ATTACHMENTS 0x01

/**
 * Maximum length of server URL and authentication token
 */
#define MAX_SERVER_URL_LEN 1024
#define MAX_TOKEN_LEN      64

/**
 * Execute HTTP request against Mattermost REST API. Request is GET if postData is nullptr and POST otherwise.
 * Returns HTTP response code, or -1 on transport failure. Response body (if any) is appended to responseData.
 */
static long ExecuteRequest(const char *url, const char *token, const char *postData, ByteStream *responseData)
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return -1;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

#if HAVE_DECL_CURLOPT_PROTOCOLS_STR
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseData);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Mattermost Driver/" NETXMS_VERSION_STRING_A);

   if (postData != nullptr)
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);

   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Content-Type: application/json");
   char authHeader[MAX_TOKEN_LEN + 32] = "Authorization: Bearer ";
   strlcat(authHeader, token, sizeof(authHeader));
   headers = curl_slist_append(headers, authHeader);
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   long httpCode = -1;
   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes"), static_cast<int>(responseData->size()));
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed (%d: %hs)"), rc, errorBuffer);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_setopt(CURLOPT_URL) failed"));
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   return httpCode;
}

/**
 * Check connection to server by requesting bot's own user record (GET /api/v4/users/me)
 */
static bool CheckServerConnection(const char *serverUrl, const char *token)
{
   char url[MAX_SERVER_URL_LEN + 32];
   strcpy(url, serverUrl);
   strcat(url, "api/v4/users/me");

   ByteStream responseData(4096);
   long httpCode = ExecuteRequest(url, token, nullptr, &responseData);
   if (httpCode == 200)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Status check successful"));
      return true;
   }
   if (httpCode != -1)
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from server: HTTP response code is %d"), static_cast<int>(httpCode));
   return false;
}

/**
 * Read server URL and authentication token from driver configuration. Returned URL always ends with '/'.
 */
static bool ParseConnectionSettings(Config *config, char *url, char *token)
{
   url[0] = 0;
   token[0] = 0;
   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("AuthToken"), CT_UTF8_STRING, 0, 0, MAX_TOKEN_LEN, 0, token },
      { _T("ServerURL"), CT_UTF8_STRING, 0, 0, MAX_SERVER_URL_LEN, 0, url },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };

   if (!config->parseTemplate(_T("Mattermost"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
      return false;
   }

   if (url[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Mattermost server URL is mandatory but not provided"));
      return false;
   }
   if ((strncmp(url, "http://", 7) != 0) && (strncmp(url, "https://", 8) != 0))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Mattermost server URL must start with http:// or https://"));
      return false;
   }
   if (url[strlen(url) - 1] != '/')
      strlcat(url, "/", MAX_SERVER_URL_LEN);

   if (token[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Mattermost authentication token is mandatory but not provided"));
      return false;
   }

   return true;
}

/**
 * Load channel alias mappings from /Channels section of driver configuration
 */
static void LoadChannelMappings(Config *config, StringMap *channels)
{
   unique_ptr<ObjectArray<ConfigEntry>> entries = config->getSubEntries(_T("/Channels"), _T("*"));
   if (entries == nullptr)
      return;

   for(int i = 0; i < entries->size(); i++)
   {
      ConfigEntry *channel = entries->get(i);
      channels->set(channel->getName(), channel->getValue());
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Added channel mapping %s = %s"), channel->getName(), channel->getValue());
   }
}

/**
 * Resolve recipient to channel ID using alias mappings (aliases are stored as wide strings).
 * Returns dynamically allocated string.
 */
static char *ResolveChannelAlias(const StringMap& channels, const char *recipient)
{
   TCHAR *key = TStringFromUTF8String(recipient);
   const TCHAR *alias = channels.get(key);
   char *channelId = (alias != nullptr) ? UTF8StringFromTString(alias) : MemCopyStringA(recipient);
   MemFree(key);
   return channelId;
}

/**
 * Mattermost notification channel driver class
 */
class MattermostDriver : public NCDriver
{
private:
   char *m_postUrl;
   char *m_serverUrl;
   char m_token[MAX_TOKEN_LEN];
   char *m_footer;
   char m_color[16];
   uint32_t m_flags;
   StringMap m_channels;

   MattermostDriver(uint32_t flags, const char *serverUrl, const char *token, const TCHAR *color, const TCHAR *footer) : NCDriver()
   {
      m_flags = flags;

      m_serverUrl = MemCopyStringA(serverUrl);

      size_t l = strlen(serverUrl);
      m_postUrl = MemAllocStringA(l + 16);
      strcpy(m_postUrl, serverUrl);
      strcat(m_postUrl, "api/v4/posts");

      strlcpy(m_token, token, MAX_TOKEN_LEN);
      m_footer = UTF8StringFromTString(footer);
      char *color8 = UTF8StringFromTString(color);
      strlcpy(m_color, color8, 16);
      MemFree(color8);
   }

public:
   virtual ~MattermostDriver()
   {
      MemFree(m_postUrl);
      MemFree(m_serverUrl);
      MemFree(m_footer);
   }

   virtual int send(const NotificationContext& context) override;

   virtual bool checkHealth() override
   {
      return CheckServerConnection(m_serverUrl, m_token);
   }

   static MattermostDriver *createInstance(Config *config);
};

/**
 * Create driver instance
 */
MattermostDriver *MattermostDriver::createInstance(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new Mattermost driver instance"));

   char url[MAX_SERVER_URL_LEN], token[MAX_TOKEN_LEN];
   if (!ParseConnectionSettings(config, url, token))
      return nullptr;

   uint32_t flags = MM_USE_ATTACHMENTS;
   TCHAR colorDefinition[16] = _T("");
   TCHAR footer[1024] = _T("");
   NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("Color"), CT_STRING, 0, 0, sizeof(colorDefinition) / sizeof(TCHAR), 0, colorDefinition },
      { _T("Footer"), CT_STRING, 0, 0, sizeof(footer) / sizeof(TCHAR), 0, footer },
      { _T("UseAttachments"), CT_BOOLEAN_FLAG_32, 0, 0, MM_USE_ATTACHMENTS, 0, &flags },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
	};

	if (!config->parseTemplate(_T("Mattermost"), configTemplate))
	{
	   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
	   return nullptr;
	}

   if (colorDefinition[0] != 0)
   {
      Color color = Color::parseCSS(colorDefinition);
      _tcscpy(colorDefinition, color.toCSS(true));
   }

   MattermostDriver *driver = new MattermostDriver(flags, url, token, colorDefinition, footer);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Mattermost driver instantiated"));
   LoadChannelMappings(config, &driver->m_channels);
   return driver;
}

/**
 * Append UTF-8 text to request being built
 */
static inline void AppendText(ByteStream& request, const char *text)
{
   request.write(text, strlen(text));
}

/**
 * Send notification
 */
int MattermostDriver::send(const NotificationContext& context)
{
   const char *recipient = context.recipient;
   const char *subject = context.subject;
   const char *body = (context.markdownBody != nullptr) ? context.markdownBody : context.body;   // Mattermost renders markdown natively
   char *channelId = ResolveChannelAlias(m_channels, recipient);

   char *jsubject = EscapeStringForJSONUtf8(subject);
   char *jbody = EscapeStringForJSONUtf8(body);

   ByteStream request(4096);
   AppendText(request, "{ \"channel_id\":\"");
   AppendText(request, channelId);
   AppendText(request, "\", ");
   if (m_flags & MM_USE_ATTACHMENTS)
   {
      AppendText(request, "\"message\":\"\", \"props\":{ \"attachments\": [{ \"fallback\":\"");
      if (jsubject[0] == 0)
      {
         AppendText(request, jbody);
      }
      else
      {
         AppendText(request, jsubject);
         AppendText(request, "\", \"title\":\"");
         AppendText(request, jsubject);
      }
      AppendText(request, "\", \"text\":\"");
      AppendText(request, jbody);
      AppendText(request, "\"");
      if (m_color[0] != 0)
      {
         AppendText(request, ", \"color\":\"");
         AppendText(request, m_color);
         AppendText(request, "\"");
      }
      if (m_footer[0] != 0)
      {
         AppendText(request, ", \"footer\":\"");
         char *jfooter = EscapeStringForJSONUtf8(m_footer);
         AppendText(request, jfooter);
         MemFree(jfooter);
         AppendText(request, "\"");
      }
      AppendText(request, "}]}");
   }
   else
   {
      AppendText(request, "\"message\":\"");
      AppendText(request, jsubject);
      if ((jsubject[0] != 0) && (jbody[0] != 0))
         AppendText(request, "\\n\\n");
      AppendText(request, jbody);
      AppendText(request, "\"");
   }
   AppendText(request, " }");
   request.write('\0');

   MemFree(channelId);
   MemFree(jsubject);
   MemFree(jbody);

   const char *json = reinterpret_cast<const char*>(request.buffer());
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Prepared request: %hs"), json);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   long httpCode = ExecuteRequest(m_postUrl, m_token, json, &responseData);
   if (httpCode == 201)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Message successfully sent"));
      return 0;
   }
   if (httpCode != -1)
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from server: HTTP response code is %d"), static_cast<int>(httpCode));
   return -1;
}

/**
 * Configuration template
 */
static const NCConfigurationTemplate s_config(true, true);

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Mattermost, &s_config)
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return MattermostDriver::createInstance(config);
}

/**
 * Reconnect backoff limits (milliseconds)
 */
#define MIN_RECONNECT_DELAY 5000
#define MAX_RECONNECT_DELAY 300000

/**
 * Idle read timeout after which client ping is sent over WebSocket (milliseconds)
 */
#define WS_IDLE_TIMEOUT 60000

/**
 * Chat bot driver. Inbound direct messages are received as "posted" events over Mattermost WebSocket API
 * (/api/v4/websocket), outbound messages are posted via REST API to direct message channel between bot
 * and peer. Peer ID is Mattermost user ID.
 */
class MattermostChatBot : public ChatBotDriver
{
private:
   char m_serverUrl[MAX_SERVER_URL_LEN];
   char m_token[MAX_TOKEN_LEN];
   char m_botUserId[64];
   StringMap m_channels;               // channel aliases for outbound messages
   StringMap m_directChannels;         // peer user ID -> direct message channel ID
   Mutex m_directChannelsLock;
   ChatBotMessageSink *m_sink;
   THREAD m_readerThread;
   WebSocketClient *m_webSocket;       // protected by m_webSocketLock, non-null only while reader thread owns a connection
   Mutex m_webSocketLock;
   Condition m_shutdownCondition;
   bool m_shutdownFlag;
   bool m_connected;

   bool fetchBotIdentity();
   bool runConnection();
   void processEvent(json_t *event);
   void processPostedEvent(json_t *data);
   bool resolveUserName(const char *userName, char *userId, size_t userIdSize);
   bool getDirectChannel(const char *userId, char *channelId, size_t channelIdSize);
   bool resolveRecipient(const char *peerId, char *channelId, size_t channelIdSize);
   bool postMessage(const char *channelId, const char *text);

   static void readerThread(MattermostChatBot *bot);

public:
   MattermostChatBot(const char *serverUrl, const char *token) : ChatBotDriver(), m_directChannelsLock(MutexType::FAST), m_webSocketLock(MutexType::FAST), m_shutdownCondition(true)
   {
      strlcpy(m_serverUrl, serverUrl, MAX_SERVER_URL_LEN);
      strlcpy(m_token, token, MAX_TOKEN_LEN);
      m_botUserId[0] = 0;
      m_sink = nullptr;
      m_readerThread = INVALID_THREAD_HANDLE;
      m_webSocket = nullptr;
      m_shutdownFlag = false;
      m_connected = false;
   }

   virtual ~MattermostChatBot()
   {
      stop();
   }

   virtual bool start(ChatBotMessageSink *sink) override;
   virtual void stop() override;
   virtual bool sendMessage(const char *peerId, const char *text, bool isMarkdown) override;

   /**
    * Mattermost interactive buttons require inbound HTTP integration endpoint, so questions
    * are left to the numbered list fallback provided by server core.
    */
   virtual bool sendQuestion(const char *peerId, const char *text, const StringList& options, uint64_t questionId) override
   {
      return false;
   }

   virtual bool checkHealth() override
   {
      return m_connected && CheckServerConnection(m_serverUrl, m_token);
   }

   static MattermostChatBot *createInstance(Config *config);
};

/**
 * Create chat bot driver instance
 */
MattermostChatBot *MattermostChatBot::createInstance(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new Mattermost chat bot driver instance"));

   char url[MAX_SERVER_URL_LEN], token[MAX_TOKEN_LEN];
   if (!ParseConnectionSettings(config, url, token))
      return nullptr;

   MattermostChatBot *bot = new MattermostChatBot(url, token);
   LoadChannelMappings(config, &bot->m_channels);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Mattermost chat bot driver instantiated"));
   return bot;
}

/**
 * Start platform connection
 */
bool MattermostChatBot::start(ChatBotMessageSink *sink)
{
   m_sink = sink;
   m_shutdownFlag = false;
   m_shutdownCondition.reset();
   m_readerThread = ThreadCreateEx(MattermostChatBot::readerThread, this);
   return true;
}

/**
 * Stop platform connection
 */
void MattermostChatBot::stop()
{
   if (m_readerThread == INVALID_THREAD_HANDLE)
      return;

   m_shutdownFlag = true;
   m_shutdownCondition.set();

   m_webSocketLock.lock();
   if (m_webSocket != nullptr)
   {
      m_webSocket->sendClose(WEBSOCKET_CLOSE_GOING_AWAY);
      m_webSocket->disconnect();   // unblocks reader thread waiting in readMessage()
   }
   m_webSocketLock.unlock();

   ThreadJoin(m_readerThread);
   m_readerThread = INVALID_THREAD_HANDLE;
   m_sink = nullptr;
}

/**
 * Reader thread - maintains WebSocket connection with reconnect on failure
 */
void MattermostChatBot::readerThread(MattermostChatBot *bot)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Mattermost chat bot reader thread started"));

   uint32_t reconnectDelay = MIN_RECONNECT_DELAY;
   while(!bot->m_shutdownFlag)
   {
      bool connected = bot->runConnection();
      if (connected)
         reconnectDelay = MIN_RECONNECT_DELAY;   // connection was established, restart backoff
      if (bot->m_shutdownFlag)
         break;

      nxlog_debug_tag(DEBUG_TAG, 5, _T("Next connection attempt in %u seconds"), reconnectDelay / 1000);
      if (bot->m_shutdownCondition.wait(reconnectDelay))
         break;
      if (!connected && (reconnectDelay < MAX_RECONNECT_DELAY))
         reconnectDelay *= 2;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Mattermost chat bot reader thread stopped"));
}

/**
 * Get bot's own user ID (needed to ignore bot's own posts and to create direct message channels)
 */
bool MattermostChatBot::fetchBotIdentity()
{
   char url[MAX_SERVER_URL_LEN + 32];
   strcpy(url, m_serverUrl);
   strcat(url, "api/v4/users/me");

   ByteStream responseData(4096);
   long httpCode = ExecuteRequest(url, m_token, nullptr, &responseData);
   if (httpCode != 200)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot get bot user identity (HTTP response code %d)"), static_cast<int>(httpCode));
      return false;
   }

   responseData.write('\0');
   json_error_t error;
   json_t *user = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
   if (user == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse bot user record (%hs)"), error.text);
      return false;
   }

   const char *id = json_object_get_string_utf8(user, "id", nullptr);
   bool success = (id != nullptr) && (*id != 0);
   if (success)
   {
      strlcpy(m_botUserId, id, sizeof(m_botUserId));
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Bot identity: user ID %hs, username %hs"), m_botUserId, json_object_get_string_utf8(user, "username", ""));
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Bot user record does not contain user ID"));
   }
   json_decref(user);
   return success;
}

/**
 * Establish WebSocket connection and process events until connection is lost or shutdown is requested.
 * Returns true if connection was successfully established (regardless of how it ended).
 */
bool MattermostChatBot::runConnection()
{
   if ((m_botUserId[0] == 0) && !fetchBotIdentity())
      return false;

   char url[MAX_SERVER_URL_LEN + 32];
   strcpy(url, m_serverUrl);
   strcat(url, "api/v4/websocket");

   WebSocketClient webSocket(DEBUG_TAG);
   if (!strncmp(url, "https://", 8))
      webSocket.enablePeerVerification();
   char authHeader[MAX_TOKEN_LEN + 8] = "Bearer ";
   strlcat(authHeader, m_token, sizeof(authHeader));
   webSocket.addHeader("Authorization", authHeader);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Connecting to %hs"), url);
   if (!webSocket.connect(url))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WebSocket connection failed (%s)"), webSocket.getErrorText());
      return false;
   }

   m_webSocketLock.lock();
   m_webSocket = &webSocket;
   m_webSocketLock.unlock();

   // Authenticate session explicitly in addition to Authorization header in handshake
   json_t *challenge = json_object();
   json_object_set_new(challenge, "seq", json_integer(1));
   json_object_set_new(challenge, "action", json_string("authentication_challenge"));
   json_t *data = json_object();
   json_object_set_new(data, "token", json_string(m_token));
   json_object_set_new(challenge, "data", data);
   char *challengeText = json_dumps(challenge, JSON_COMPACT);
   json_decref(challenge);
   bool success = webSocket.sendText(challengeText);
   MemFree(challengeText);

   ByteStream message(16384);
   while(success && !m_shutdownFlag)
   {
      WebSocketMessageType type;
      WebSocketReadResult result = webSocket.readMessage(&message, &type, WS_IDLE_TIMEOUT);
      if (result == WebSocketReadResult::MESSAGE)
      {
         if (type != WebSocketMessageType::TEXT)
            continue;

         message.write('\0');
         json_error_t error;
         json_t *event = json_loads(reinterpret_cast<const char*>(message.buffer()), 0, &error);
         if (event != nullptr)
         {
            processEvent(event);
            json_decref(event);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse WebSocket message (%hs)"), error.text);
         }
      }
      else if (result == WebSocketReadResult::TIMEOUT)
      {
         webSocket.sendPing();
      }
      else
      {
         if (!m_shutdownFlag)
            nxlog_debug_tag(DEBUG_TAG, 4, _T("WebSocket connection closed (code %u, %s)"), webSocket.getCloseCode(),
               (result == WebSocketReadResult::CLOSED) ? _T("close handshake completed") : webSocket.getErrorText());
         break;
      }
   }

   m_connected = false;

   m_webSocketLock.lock();
   m_webSocket = nullptr;
   m_webSocketLock.unlock();
   return true;
}

/**
 * Process event received over WebSocket
 */
void MattermostChatBot::processEvent(json_t *event)
{
   const char *eventName = json_object_get_string_utf8(event, "event", nullptr);
   if (eventName == nullptr)
   {
      // Response to request sent by this client
      const char *status = json_object_get_string_utf8(event, "status", "");
      int64_t seq = json_object_get_int64(event, "seq_reply", 0);
      if (strcmp(status, "OK"))
      {
         json_t *error = json_object_get(event, "error");
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Request %d failed: status %hs, error %hs"), static_cast<int>(seq), status,
            json_is_object(error) ? json_object_get_string_utf8(error, "message", "unknown") : "unknown");
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Request %d completed"), static_cast<int>(seq));
      }
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 8, _T("Received event \"%hs\""), eventName);
   if (!strcmp(eventName, "posted"))
   {
      json_t *data = json_object_get(event, "data");
      if (json_is_object(data))
         processPostedEvent(data);
   }
   else if (!strcmp(eventName, "hello"))
   {
      m_connected = true;
      json_t *data = json_object_get(event, "data");
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Connected to Mattermost server version %hs"), json_is_object(data) ? json_object_get_string_utf8(data, "server_version", "unknown") : "unknown");
   }
}

/**
 * Process "posted" event - dispatch direct messages from other users to message sink
 */
void MattermostChatBot::processPostedEvent(json_t *data)
{
   if (strcmp(json_object_get_string_utf8(data, "channel_type", ""), "D"))
      return;   // Not a direct message

   // Post is delivered as JSON-encoded string
   const char *postText = json_object_get_string_utf8(data, "post", nullptr);
   if (postText == nullptr)
      return;

   json_error_t error;
   json_t *post = json_loads(postText, 0, &error);
   if (post == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse post in \"posted\" event (%hs)"), error.text);
      return;
   }

   const char *userId = json_object_get_string_utf8(post, "user_id", "");
   const char *channelId = json_object_get_string_utf8(post, "channel_id", "");
   const char *messageText = json_object_get_string_utf8(post, "message", "");
   const char *postType = json_object_get_string_utf8(post, "type", "");
   if ((*userId != 0) && strcmp(userId, m_botUserId) && (*postType == 0) && (*messageText != 0))
   {
      // Remember direct message channel for this user - saves channel lookup on reply
      if (*channelId != 0)
      {
         TCHAR *key = TStringFromUTF8String(userId);
         m_directChannelsLock.lock();
         m_directChannels.setPreallocated(key, TStringFromUTF8String(channelId));
         m_directChannelsLock.unlock();
      }

      const char *senderName = json_object_get_string_utf8(data, "sender_name", "");
      if (*senderName == '@')
         senderName++;
      if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 5)
      {
         TCHAR *text = TStringFromUTF8String(messageText);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Direct message from user %hs (%hs): %s"), senderName, userId, text);
         MemFree(text);
      }
      if (m_sink != nullptr)
         m_sink->onMessage(userId, senderName, messageText);
   }
   json_decref(post);
}

/**
 * Resolve Mattermost username to user ID (GET /api/v4/users/username/{name})
 */
bool MattermostChatBot::resolveUserName(const char *userName, char *userId, size_t userIdSize)
{
   char url[MAX_SERVER_URL_LEN + 320];
   strcpy(url, m_serverUrl);
   strcat(url, "api/v4/users/username/");
   char *escapedName = curl_easy_escape(nullptr, userName, 0);
   strlcat(url, escapedName, sizeof(url));
   curl_free(escapedName);

   ByteStream responseData(4096);
   long httpCode = ExecuteRequest(url, m_token, nullptr, &responseData);
   if (httpCode != 200)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot resolve username \"%hs\" (HTTP response code %d)"), userName, static_cast<int>(httpCode));
      return false;
   }

   responseData.write('\0');
   json_t *user = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, nullptr);
   const char *id = (user != nullptr) ? json_object_get_string_utf8(user, "id", nullptr) : nullptr;
   bool success = (id != nullptr) && (*id != 0);
   if (success)
      strlcpy(userId, id, userIdSize);
   else
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse user record for username \"%hs\""), userName);
   json_decref(user);
   return success;
}

/**
 * Get direct message channel between bot and given user, creating it if necessary
 * (POST /api/v4/channels/direct). Result is cached.
 */
bool MattermostChatBot::getDirectChannel(const char *userId, char *channelId, size_t channelIdSize)
{
   TCHAR *key = TStringFromUTF8String(userId);
   m_directChannelsLock.lock();
   const TCHAR *cachedChannelId = m_directChannels.get(key);
   if (cachedChannelId != nullptr)
   {
      tchar_to_utf8(cachedChannelId, -1, channelId, channelIdSize);
      m_directChannelsLock.unlock();
      MemFree(key);
      return true;
   }
   m_directChannelsLock.unlock();

   bool success = false;
   if ((m_botUserId[0] != 0) || fetchBotIdentity())
   {
      char url[MAX_SERVER_URL_LEN + 32];
      strcpy(url, m_serverUrl);
      strcat(url, "api/v4/channels/direct");

      json_t *request = json_array();
      json_array_append_new(request, json_string(m_botUserId));
      json_array_append_new(request, json_string(userId));
      char *requestText = json_dumps(request, JSON_COMPACT);
      json_decref(request);

      ByteStream responseData(4096);
      long httpCode = ExecuteRequest(url, m_token, requestText, &responseData);
      MemFree(requestText);

      if (httpCode == 201)
      {
         responseData.write('\0');
         json_t *channel = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, nullptr);
         const char *id = (channel != nullptr) ? json_object_get_string_utf8(channel, "id", nullptr) : nullptr;
         if ((id != nullptr) && (*id != 0))
         {
            strlcpy(channelId, id, channelIdSize);
            m_directChannelsLock.lock();
            m_directChannels.setPreallocated(key, TStringFromUTF8String(id));
            m_directChannelsLock.unlock();
            key = nullptr;   // ownership transferred to map
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Direct message channel for user %hs: %hs"), userId, id);
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse direct message channel record for user %hs"), userId);
         }
         json_decref(channel);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot create direct message channel for user %hs (HTTP response code %d)"), userId, static_cast<int>(httpCode));
      }
   }
   MemFree(key);
   return success;
}

/**
 * Resolve outbound recipient to channel ID. Recipient can be channel alias from driver configuration,
 * Mattermost username prefixed with '@', or Mattermost user ID (as reported to message sink).
 */
bool MattermostChatBot::resolveRecipient(const char *peerId, char *channelId, size_t channelIdSize)
{
   TCHAR *key = TStringFromUTF8String(peerId);
   const TCHAR *alias = m_channels.get(key);
   MemFree(key);
   if (alias != nullptr)
   {
      tchar_to_utf8(alias, -1, channelId, channelIdSize);
      return true;
   }

   if (*peerId == '@')
   {
      char userId[64];
      return resolveUserName(peerId + 1, userId, sizeof(userId)) && getDirectChannel(userId, channelId, channelIdSize);
   }

   return getDirectChannel(peerId, channelId, channelIdSize);
}

/**
 * Post message to given channel (POST /api/v4/posts). Text is markdown, rendered natively by Mattermost.
 */
bool MattermostChatBot::postMessage(const char *channelId, const char *text)
{
   char url[MAX_SERVER_URL_LEN + 32];
   strcpy(url, m_serverUrl);
   strcat(url, "api/v4/posts");

   json_t *request = json_object();
   json_object_set_new(request, "channel_id", json_string(channelId));
   json_object_set_new(request, "message", json_string(text));
   char *requestText = json_dumps(request, JSON_COMPACT);
   json_decref(request);
   if (requestText == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot encode message for channel %hs (invalid UTF-8?)"), channelId);
      return false;
   }

   ByteStream responseData(4096);
   long httpCode = ExecuteRequest(url, m_token, requestText, &responseData);
   MemFree(requestText);

   if (httpCode == 201)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Message successfully posted to channel %hs"), channelId);
      return true;
   }
   if (httpCode != -1)
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Error response from server: HTTP response code is %d"), static_cast<int>(httpCode));
   return false;
}

/**
 * Send message to given peer. Mattermost renders posts as markdown, so markdown text is posted
 * as is and literal text is escaped to be displayed as written.
 */
bool MattermostChatBot::sendMessage(const char *peerId, const char *text, bool isMarkdown)
{
   char channelId[64];
   if (!resolveRecipient(peerId, channelId, sizeof(channelId)))
      return false;

   if (isMarkdown)
      return postMessage(channelId, text);

   char *escapedText = EscapeStringForMarkdown(text);
   bool success = postMessage(channelId, escapedText);
   MemFree(escapedText);
   return success;
}

/**
 * Chat bot entry point
 */
DECLARE_CHATBOT_ENTRY_POINT
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("cURL initialization failed"));
      return nullptr;
   }
   return MattermostChatBot::createInstance(config);
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
