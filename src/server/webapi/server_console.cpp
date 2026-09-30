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
** File: server_console.cpp
**
**/

#include "webapi.h"
#include <nms_users.h>

#define DEBUG_TAG _T("webapi.console")

/**
 * Token validity period in seconds
 */
static const int CONSOLE_SESSION_TOKEN_VALIDITY = 30;

/**
 * Maximum console command length. Console command parser copies command words
 * into fixed size buffers, so longer commands must be rejected.
 */
#define MAX_CONSOLE_COMMAND_LEN  255

/**
 * Validate console command. Returns error message or nullptr if command is valid.
 */
static const wchar_t *ValidateConsoleCommand(const wchar_t *command)
{
   if ((command == nullptr) || (command[0] == 0))
      return L"Command cannot be empty";
   if (wcslen(command) > MAX_CONSOLE_COMMAND_LEN)
      return L"Command is too long";
   return nullptr;
}

/**
 * Handler for POST /v1/server-console/execute
 */
int H_ServerConsoleExecute(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONSOLE))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing or invalid JSON body");
      return 400;
   }

   String command = json_object_get_string(request, "command", L"");
   const wchar_t *error = ValidateConsoleCommand(command);
   if (error != nullptr)
   {
      context->setErrorResponse(error);
      return 400;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"User %s [%u] executing console command \"%s\"", context->getLoginName(), context->getUserId(), command.cstr());

   StringBufferConsole console;
   bool shutdown = (ProcessConsoleCommand(command, &console) == CMD_EXIT_SHUTDOWN);
   if (shutdown)
      ThreadCreate(InitiateShutdown, ShutdownReason::FROM_REMOTE_CONSOLE);

   json_t *response = json_object();
   json_object_set_new(response, "output", json_string_w(console.getOutput()));
   json_object_set_new(response, "shutdownInitiated", json_boolean(shutdown));
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * WebSocket server console session
 */
class ServerConsoleWebSocketSession : public ServerConsole
{
private:
   MHD_UpgradeResponseHandle *m_responseHandle;
   SOCKET m_socket;
   Mutex m_socketMutex;
   bool m_closed;
   Mutex m_closeMutex;
   uint32_t m_userId;
   wchar_t m_loginName[MAX_USER_NAME];
   time_t m_creationTime;

   void sendTextFrame(const char *text);
   void sendMessage(const char *type, const char *key, const wchar_t *value);
   bool processCommand(const wchar_t *command);

protected:
   virtual void write(const wchar_t *text) override;

public:
   ServerConsoleWebSocketSession(uint32_t userId, const wchar_t *loginName);

   void start(MHD_UpgradeResponseHandle *responseHandle, SOCKET s);
   void run();
   void close();

   time_t getCreationTime() const { return m_creationTime; }
};

/**
 * Session constructor
 */
ServerConsoleWebSocketSession::ServerConsoleWebSocketSession(uint32_t userId, const wchar_t *loginName) :
         m_socketMutex(MutexType::FAST), m_closeMutex(MutexType::FAST)
{
   m_responseHandle = nullptr;
   m_socket = INVALID_SOCKET;
   m_closed = false;
   m_userId = userId;
   wcslcpy(m_loginName, loginName, MAX_USER_NAME);
   m_creationTime = time(nullptr);
}

/**
 * Send text frame to WebSocket client
 */
void ServerConsoleWebSocketSession::sendTextFrame(const char *text)
{
   m_socketMutex.lock();
   if ((m_socket != INVALID_SOCKET) && !m_closed)
      SendWebsocketFrame(m_socket, text, strlen(text));
   m_socketMutex.unlock();
}

/**
 * Send JSON message with given type and optional string field to WebSocket client
 */
void ServerConsoleWebSocketSession::sendMessage(const char *type, const char *key, const wchar_t *value)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string(type));
   if (key != nullptr)
      json_object_set_new(msg, key, json_string_w(value));
   char *encoded = json_dumps(msg, 0);
   sendTextFrame(encoded);
   MemFree(encoded);
   json_decref(msg);
}

/**
 * Write console output to WebSocket client
 */
void ServerConsoleWebSocketSession::write(const wchar_t *text)
{
   sendMessage("output", "data", text);
}

/**
 * Start session after WebSocket upgrade
 */
void ServerConsoleWebSocketSession::start(MHD_UpgradeResponseHandle *responseHandle, SOCKET s)
{
   m_socketMutex.lock();
   m_responseHandle = responseHandle;
   m_socket = s;
   m_socketMutex.unlock();
   sendTextFrame("{\"type\":\"ready\"}");
   nxlog_debug_tag(DEBUG_TAG, 3, L"Server console WebSocket session started for user %s [%u]", m_loginName, m_userId);
}

/**
 * Process command received from client. Returns false if session should be closed.
 */
bool ServerConsoleWebSocketSession::processCommand(const wchar_t *command)
{
   // User can be disabled, deleted, or lose access right while session is open
   wchar_t loginName[MAX_USER_NAME];
   uint64_t systemAccessRights;
   uint32_t rcc;
   if (!ValidateUserId(m_userId, loginName, &systemAccessRights, &rcc) || !(systemAccessRights & SYSTEM_ACCESS_SERVER_CONSOLE))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Server console WebSocket session for user %s [%u] closed: access denied", m_loginName, m_userId);
      sendMessage("error", "message", L"Access denied");
      return false;
   }

   const wchar_t *error = ValidateConsoleCommand(command);
   if (error != nullptr)
   {
      sendMessage("error", "message", error);
      sendTextFrame("{\"type\":\"ready\"}");
      return true;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"User %s [%u] executing console command \"%s\"", m_loginName, m_userId, command);

   switch(ProcessConsoleCommand(command, this))
   {
      case CMD_EXIT_CLOSE_SESSION:
         sendTextFrame("{\"type\":\"closed\",\"reason\":\"exit\"}");
         return false;
      case CMD_EXIT_SHUTDOWN:
         sendTextFrame("{\"type\":\"closed\",\"reason\":\"shutdown\"}");
         close();
         ThreadCreate(InitiateShutdown, ShutdownReason::FROM_REMOTE_CONSOLE);
         return false;
      default:
         sendTextFrame("{\"type\":\"ready\"}");
         return true;
   }
}

/**
 * Main loop - read commands from WebSocket client and execute them
 */
void ServerConsoleWebSocketSession::run()
{
   while(!m_closed && !IsShutdownInProgress())
   {
      ByteStream buffer;
      BYTE frameType;
      if (!ReadWebsocketFrame(m_socket, &buffer, &frameType))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Server console WebSocket session for user %s [%u]: read error", m_loginName, m_userId);
         break;
      }

      if (frameType == 0x08)  // Close frame
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Server console WebSocket session for user %s [%u]: received close frame", m_loginName, m_userId);
         break;
      }
      else if (frameType == 0x09)  // Ping frame
      {
         m_socketMutex.lock();
         BYTE pong[2] = { 0x8A, 0x00 };  // FIN + pong opcode, no payload
         SendEx(m_socket, pong, 2, 0, nullptr);
         m_socketMutex.unlock();
      }
      else if (frameType == 0x01)  // Text frame
      {
         json_error_t error;
         json_t *msg = json_loadb(reinterpret_cast<const char*>(buffer.buffer()), buffer.size(), 0, &error);
         const char *type = json_object_get_string_utf8(msg, "type", "");
         if (!strcmp(type, "command"))
         {
            String command = json_object_get_string(msg, "command", L"");
            json_decref(msg);
            if (!processCommand(command))
               break;
         }
         else
         {
            json_decref(msg);
            sendTextFrame("{\"type\":\"error\",\"message\":\"Invalid message\"}");
            sendTextFrame("{\"type\":\"ready\"}");
         }
      }
      // Ignore other frame types
   }
   close();
}

/**
 * Close the session
 */
void ServerConsoleWebSocketSession::close()
{
   m_closeMutex.lock();
   if (m_closed)
   {
      m_closeMutex.unlock();
      return;
   }
   m_closed = true;
   m_closeMutex.unlock();

   m_socketMutex.lock();
   if (m_socket != INVALID_SOCKET)
      SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
   m_socketMutex.unlock();

   if (m_responseHandle != nullptr)
      MHD_upgrade_action(m_responseHandle, MHD_UPGRADE_ACTION_CLOSE);

   nxlog_debug_tag(DEBUG_TAG, 5, L"Server console WebSocket session for user %s [%u] closed", m_loginName, m_userId);
}

/**
 * Pending sessions storage
 */
static HashMap<uuid, ServerConsoleWebSocketSession> s_pendingSessions(Ownership::True);
static Mutex s_pendingSessionsLock;

/**
 * Cleanup expired pending sessions
 */
void CleanupExpiredServerConsoleSessions()
{
   time_t now = time(nullptr);

   s_pendingSessionsLock.lock();

   StructArray<uuid> expiredTokens;
   s_pendingSessions.forEach(
      [now, &expiredTokens](const uuid& token, ServerConsoleWebSocketSession *session) -> EnumerationCallbackResult
      {
         if (now - session->getCreationTime() > CONSOLE_SESSION_TOKEN_VALIDITY * 2)
            expiredTokens.add(token);
         return _CONTINUE;
      });

   for(int i = 0; i < expiredTokens.size(); i++)
      s_pendingSessions.remove(*expiredTokens.get(i));

   s_pendingSessionsLock.unlock();

   if (!expiredTokens.isEmpty())
      nxlog_debug_tag(DEBUG_TAG, 5, L"%d expired server console session tokens removed", expiredTokens.size());

   ThreadPoolScheduleRelative(g_mainThreadPool, 300000, CleanupExpiredServerConsoleSessions);  // Reschedule itself to run in 5 minutes
}

/**
 * Handler for POST /v1/server-console/session - create console session and return token
 */
int H_ServerConsoleSessionCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONSOLE))
      return 403;

   uuid token = uuid::generate();
   s_pendingSessionsLock.lock();
   s_pendingSessions.set(token, new ServerConsoleWebSocketSession(context->getUserId(), context->getLoginName()));
   s_pendingSessionsLock.unlock();

   context->writeAuditLog(AUDIT_CONSOLE, true, 0, L"Server console session opened (WebSocket pending)");
   nxlog_debug_tag(DEBUG_TAG, 4, L"Server console session token created for user %s [%u]", context->getLoginName(), context->getUserId());

   json_t *response = json_object();
   char tokenStr[64];
   token.toStringA(tokenStr);
   json_object_set_new(response, "token", json_string(tokenStr));
   json_object_set_new(response, "expiresIn", json_integer(CONSOLE_SESSION_TOKEN_VALIDITY));

   char wsUrl[128];
   snprintf(wsUrl, sizeof(wsUrl), "/v1/server-console/session/%s", tokenStr);
   json_object_set_new(response, "wsUrl", json_string(wsUrl));

   context->setResponseData(response);
   json_decref(response);
   return 201;
}

/**
 * Reject WebSocket connection
 */
static void RejectConnection(MHD_socket sock, MHD_UpgradeResponseHandle *responseHandle, const wchar_t *reason)
{
   nxlog_debug_tag(DEBUG_TAG, 4, L"Server console WebSocket connection rejected: %s", reason);
   SendWebsocketCloseFrame(static_cast<SOCKET>(sock), WS_CLOSE_POLICY_VIOLATION);
   MHD_upgrade_action(responseHandle, MHD_UPGRADE_ACTION_CLOSE);
}

/**
 * WebSocket upgrade handler for server console session
 */
void WS_ServerConsoleConnect(void *cls, MHD_Connection *connection, void *con_cls,
                             const char *extra_in, size_t extra_in_size, MHD_socket sock,
                             MHD_UpgradeResponseHandle *responseHandle)
{
   Context *context = static_cast<Context*>(cls);

   const wchar_t *tokenStr = context->getPlaceholderValue(L"token");
   if (tokenStr == nullptr)
   {
      RejectConnection(sock, responseHandle, L"no token provided");
      return;
   }

   uuid token = uuid::parse(tokenStr);
   if (token.isNull())
   {
      RejectConnection(sock, responseHandle, L"invalid token format");
      return;
   }

   // Look up and remove pending session (token is single-use)
   s_pendingSessionsLock.lock();
   ServerConsoleWebSocketSession *session = s_pendingSessions.get(token);
   if (session != nullptr)
      s_pendingSessions.unlink(token);
   s_pendingSessionsLock.unlock();

   if (session == nullptr)
   {
      RejectConnection(sock, responseHandle, L"token not found");
      return;
   }

   if (time(nullptr) - session->getCreationTime() > CONSOLE_SESSION_TOKEN_VALIDITY)
   {
      delete session;
      RejectConnection(sock, responseHandle, L"token expired");
      return;
   }

   session->start(responseHandle, static_cast<SOCKET>(sock));

   ThreadCreate(
      [session] () -> void
      {
         session->run();
         delete session;
      });
}
