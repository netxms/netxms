/*
** NetXMS - Network Management System
** Copyright (C) 2023-2025 Raden Solutions
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
** File: webapi.h
**
**/

#ifndef _webapi_h_
#define _webapi_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <netxms-webapi.h>

/**
 * Create object summary JSON document
 */
json_t *CreateObjectSummary(const NetObj& object);

/**
 * Create object summary JSON document with effective access rights for given user
 */
json_t *CreateObjectSummary(const NetObj& object, uint32_t userId);

/**
 * Add already evaluated effective access rights to object's JSON document
 */
void AddEffectiveRights(json_t *json, uint64_t rights);

/**
 * Add effective access rights of given user on given object to object's JSON document
 */
void AddEffectiveRights(json_t *json, const NetObj& object, uint32_t userId);

/**
 * Validity period in seconds for WebSocket output connection token
 */
#define WS_OUTPUT_TOKEN_VALIDITY 30

/**
 * Output stream to WebSocket client. Producer registers session with RegisterWebSocketOutputSession() to obtain
 * connection token, waits for client connection with waitForConnection(), and only then starts producing output,
 * so output is never buffered. Messages are JSON objects with "type" field. Final message is sent with finish(),
 * which also closes connection.
 */
class WebSocketOutputSession
{
   friend void ConnectWebSocketOutputSession(Context *context, MHD_socket sock, MHD_UpgradeResponseHandle *responseHandle);

private:
   uuid m_token;
   MHD_UpgradeResponseHandle *m_responseHandle;
   SOCKET m_socket;
   bool m_closeFrameSent;
   bool m_disconnected;
   Mutex m_socketLock;
   Condition m_connected;

   void connect(MHD_UpgradeResponseHandle *responseHandle, SOCKET s);
   void readFrames();
   void disconnectOnSendFailure();

protected:
   virtual void onTextFrame(json_t *msg);
   virtual void onDisconnect();

public:
   WebSocketOutputSession();
   virtual ~WebSocketOutputSession() = default;

   const uuid& getToken() const { return m_token; }
   bool waitForConnection();
   bool isDisconnected() const { return m_disconnected; }

   void sendMessage(json_t *msg);
   void sendOutput(const wchar_t *text);
   void sendOutputUtf8(const char *text, size_t length);
   void finish(json_t *msg);
   void sendCompleted();
   void sendError(const char *message);
};

uuid RegisterWebSocketOutputSession(const shared_ptr<WebSocketOutputSession>& session);
void ConnectWebSocketOutputSession(Context *context, MHD_socket sock, MHD_UpgradeResponseHandle *responseHandle);
json_t *CreateWebSocketOutputResponse(const uuid& token, const char *urlPrefix);

#endif
