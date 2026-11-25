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
** File: ws_output.cpp
**
**/

#include "webapi.h"

#define DEBUG_TAG L"webapi.ws"

/**
 * Timeout for sending single frame to client (in milliseconds). Producers can send while holding locks
 * (poll output is sent under poller lock), so client that stopped reading should be detected quickly.
 */
#define SEND_TIMEOUT 5000

/**
 * Output sessions waiting for WebSocket connection, indexed by connection token
 */
static SharedHashMap<uuid, WebSocketOutputSession> s_pendingSessions;
static Mutex s_pendingSessionsLock(MutexType::FAST);

/**
 * Session constructor
 */
WebSocketOutputSession::WebSocketOutputSession() : m_token(uuid::generate()), m_socketLock(MutexType::FAST), m_connected(true)
{
   m_responseHandle = nullptr;
   m_socket = INVALID_SOCKET;
   m_closeFrameSent = false;
   m_disconnected = false;
}

/**
 * Register session as waiting for client connection. Returns connection token.
 */
uuid RegisterWebSocketOutputSession(const shared_ptr<WebSocketOutputSession>& session)
{
   s_pendingSessionsLock.lock();
   s_pendingSessions.set(session->getToken(), session);
   s_pendingSessionsLock.unlock();
   return session->getToken();
}

/**
 * Create response document for request that started streaming output. URL prefix is the WebSocket endpoint
 * path without token (for example "/v1/object-tools/output").
 */
json_t *CreateWebSocketOutputResponse(const uuid& token, const char *urlPrefix)
{
   json_t *response = json_object();
   char tokenStr[64];
   token.toStringA(tokenStr);
   json_object_set_new(response, "token", json_string(tokenStr));
   json_object_set_new(response, "expiresIn", json_integer(WS_OUTPUT_TOKEN_VALIDITY));

   char wsUrl[256];
   snprintf(wsUrl, sizeof(wsUrl), "%s/%s", urlPrefix, tokenStr);
   json_object_set_new(response, "wsUrl", json_string(wsUrl));
   return response;
}

/**
 * Wait for WebSocket client connection (called from producer thread). Returns false if client did not
 * connect within token validity period - in that case token is invalidated and no output should be produced.
 */
bool WebSocketOutputSession::waitForConnection()
{
   if (m_connected.wait(WS_OUTPUT_TOKEN_VALIDITY * 1000))
      return true;

   // Token may be consumed by connection handler right at timeout, in that case connection will be established shortly
   s_pendingSessionsLock.lock();
   bool expired = s_pendingSessions.contains(m_token);
   if (expired)
      s_pendingSessions.remove(m_token);
   s_pendingSessionsLock.unlock();

   if (expired)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Output session cancelled: client did not connect within %d seconds", WS_OUTPUT_TOKEN_VALIDITY);
      return false;
   }

   m_connected.wait(INFINITE);
   return true;
}

/**
 * Attach WebSocket connection to session
 */
void WebSocketOutputSession::connect(MHD_UpgradeResponseHandle *responseHandle, SOCKET s)
{
   m_socketLock.lock();
   m_responseHandle = responseHandle;
   m_socket = s;
   m_socketLock.unlock();
   m_connected.set();
}

/**
 * Mark session as disconnected after send failure and shut down socket so that reader thread will finish.
 * Should be called with socket lock held.
 */
void WebSocketOutputSession::disconnectOnSendFailure()
{
   nxlog_debug_tag(DEBUG_TAG, 5, L"Output session: send failed or timed out, closing connection");
   m_disconnected = true;
   shutdown(m_socket, SHUT_RDWR);
}

/**
 * Send JSON message to WebSocket client. Message is silently dropped if client already disconnected.
 * Client is disconnected on first failed send, so all subsequent messages are dropped without delay.
 */
void WebSocketOutputSession::sendMessage(json_t *msg)
{
   char *encoded = json_dumps(msg, 0);
   m_socketLock.lock();
   if ((m_socket != INVALID_SOCKET) && !m_closeFrameSent && !m_disconnected)
   {
      if (!SendWebsocketFrame(m_socket, encoded, strlen(encoded), SEND_TIMEOUT))
         disconnectOnSendFailure();
   }
   m_socketLock.unlock();
   MemFree(encoded);
}

/**
 * Send final message and close frame, then shut down socket so that reader thread will finish
 */
void WebSocketOutputSession::finish(json_t *msg)
{
   sendMessage(msg);
   m_socketLock.lock();
   if ((m_socket != INVALID_SOCKET) && !m_closeFrameSent && !m_disconnected)
   {
      SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
      shutdown(m_socket, SHUT_RDWR);
   }
   m_closeFrameSent = true;
   m_socketLock.unlock();
}

/**
 * Send output chunk to WebSocket client
 */
void WebSocketOutputSession::sendOutput(const wchar_t *text)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("output"));
   json_object_set_new(msg, "data", json_string_w(text));
   sendMessage(msg);
   json_decref(msg);
}

/**
 * Send output chunk from UTF-8 source
 */
void WebSocketOutputSession::sendOutputUtf8(const char *text, size_t length)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("output"));
   json_object_set_new(msg, "data", json_stringn(text, length));
   sendMessage(msg);
   json_decref(msg);
}

/**
 * Send completion message and close connection
 */
void WebSocketOutputSession::sendCompleted()
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("completed"));
   finish(msg);
   json_decref(msg);
}

/**
 * Send error message and close connection
 */
void WebSocketOutputSession::sendError(const char *message)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("error"));
   json_object_set_new(msg, "message", json_string(message));
   finish(msg);
   json_decref(msg);
}

/**
 * Handle text frame received from client. Default implementation ignores it.
 */
void WebSocketOutputSession::onTextFrame(json_t *msg)
{
}

/**
 * Handle client disconnect. Default implementation does nothing.
 */
void WebSocketOutputSession::onDisconnect()
{
}

/**
 * Read frames from WebSocket client until connection is closed by either side
 */
void WebSocketOutputSession::readFrames()
{
   while(true)
   {
      ByteStream buffer;
      BYTE frameType;
      if (!ReadWebsocketFrame(m_socket, &buffer, &frameType))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Output session: WebSocket read error or connection closed");
         break;
      }

      if (frameType == 0x08)  // Close frame
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Output session: received close frame");
         break;
      }
      else if (frameType == 0x09)  // Ping frame
      {
         m_socketLock.lock();
         if (!m_closeFrameSent && !m_disconnected)
         {
            BYTE pong[2] = { 0x8A, 0x00 };  // FIN + pong opcode, no payload
            if (SendEx(m_socket, pong, 2, 0, nullptr, SEND_TIMEOUT) != 2)
               disconnectOnSendFailure();
         }
         m_socketLock.unlock();
      }
      else if (frameType == 0x01)  // Text frame
      {
         json_error_t error;
         json_t *msg = json_loadb(reinterpret_cast<const char*>(buffer.buffer()), buffer.size(), 0, &error);
         onTextFrame(msg);
         json_decref(msg);
      }
      // Ignore other frame types
   }

   onDisconnect();

   m_socketLock.lock();
   if (!m_closeFrameSent && !m_disconnected)
      SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
   m_disconnected = true;
   m_socketLock.unlock();

   MHD_upgrade_action(m_responseHandle, MHD_UPGRADE_ACTION_CLOSE);
   nxlog_debug_tag(DEBUG_TAG, 5, L"Output session: WebSocket connection closed");
}

/**
 * Connect WebSocket client to output session identified by token in request path (placeholder "token").
 * Token is single-use; it is removed by producer thread when not used within validity period.
 */
void ConnectWebSocketOutputSession(Context *context, MHD_socket sock, MHD_UpgradeResponseHandle *responseHandle)
{
   const wchar_t *tokenStr = context->getPlaceholderValue(L"token");
   if (tokenStr == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Output WebSocket connection rejected: no token provided");
      RejectWebsocketConnection(sock, responseHandle);
      return;
   }

   uuid token = uuid::parse(tokenStr);
   if (token.isNull())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Output WebSocket connection rejected: invalid token format");
      RejectWebsocketConnection(sock, responseHandle);
      return;
   }

   s_pendingSessionsLock.lock();
   shared_ptr<WebSocketOutputSession> session = s_pendingSessions.getShared(token);
   if (session != nullptr)
      s_pendingSessions.remove(token);
   s_pendingSessionsLock.unlock();

   if (session == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Output WebSocket connection rejected: token not found");
      RejectWebsocketConnection(sock, responseHandle);
      return;
   }

   session->connect(responseHandle, static_cast<SOCKET>(sock));
   nxlog_debug_tag(DEBUG_TAG, 4, L"Output WebSocket connection established");

   ThreadCreate([session]() -> void { session->readFrames(); });
}
