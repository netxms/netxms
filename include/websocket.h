/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: websocket.h
**
**/

#ifndef _websocket_h_
#define _websocket_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <tls_conn.h>

/**
 * WebSocket close status codes (RFC 6455 section 7.4.1)
 */
#define WEBSOCKET_CLOSE_NORMAL            1000
#define WEBSOCKET_CLOSE_GOING_AWAY        1001
#define WEBSOCKET_CLOSE_PROTOCOL_ERROR    1002
#define WEBSOCKET_CLOSE_UNSUPPORTED_DATA  1003
#define WEBSOCKET_CLOSE_NO_STATUS         1005
#define WEBSOCKET_CLOSE_ABNORMAL          1006
#define WEBSOCKET_CLOSE_INVALID_PAYLOAD   1007
#define WEBSOCKET_CLOSE_POLICY_VIOLATION  1008
#define WEBSOCKET_CLOSE_MESSAGE_TOO_BIG   1009
#define WEBSOCKET_CLOSE_INTERNAL_ERROR    1011

/**
 * Default limit for size of reassembled incoming message
 */
#define WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE (16 * 1024 * 1024)

/**
 * WebSocket message type
 */
enum class WebSocketMessageType
{
   TEXT = 1,
   BINARY = 2
};

/**
 * Result of WebSocketClient::readMessage()
 */
enum class WebSocketReadResult
{
   MESSAGE,    // complete message received
   TIMEOUT,    // no complete message within given timeout, connection still open
   CLOSED,     // connection closed (close handshake completed or peer closed transport after close frame)
   FAILURE     // connection lost or protocol error, connection is no longer usable
};

/**
 * RFC 6455 WebSocket client over plain TCP or TLS.
 *
 * Threading model: exactly one thread may call readMessage() and close() (the reader thread). Any thread may
 * call sendText(), sendBinary(), sendPing(), sendClose(), and disconnect() concurrently with the reader;
 * disconnect() shuts the socket down and causes a readMessage() in progress on another thread to return.
 */
class LIBNETXMS_EXPORTABLE WebSocketClient
{
private:
   TCHAR m_debugTag[20];
   Mutex m_lock;
   TLSConnection *m_connection;  // protected by m_lock
   SOCKET m_socket;              // cached copy of connection socket for polling without lock
   bool m_verifyPeer;
   size_t m_maxMessageSize;
   char *m_extraHeaders;
   char m_host[256];
   uint16_t m_port;
   bool m_secure;
   char m_path[2048];
   ByteStream m_readBuffer;      // raw bytes received and not yet parsed
   ByteStream m_messageBuffer;   // fragments of message being reassembled
   WebSocketMessageType m_messageType;
   bool m_messageInProgress;
   bool m_closeSent;
   bool m_closeReceived;
   uint16_t m_closeCode;
   char m_closeReason[124];
   TCHAR m_errorText[256];

   bool performHandshake(int64_t deadline);
   bool sendFrame(uint8_t opcode, const void *payload, size_t size);
   bool sendCloseFrame(uint16_t code, const char *reason);
   ssize_t readChunk(uint32_t timeout);
   WebSocketReadResult processFrame(ByteStream *message, WebSocketMessageType *type);
   void failConnection(uint16_t closeCode, const TCHAR *errorText);
   void dropConnection();

public:
   WebSocketClient(const TCHAR *debugTag = _T("websocket"));
   ~WebSocketClient();

   /**
    * Enable TLS peer certificate verification against system CA store. Must be called before connect().
    */
   void enablePeerVerification() { m_verifyPeer = true; }

   /**
    * Set limit for size of reassembled incoming message. Larger messages fail the connection with status 1009.
    */
   void setMaxMessageSize(size_t size) { m_maxMessageSize = size; }

   void addHeader(const char *name, const char *value);

   bool connect(const char *url, uint32_t timeout = 10000);
   bool isConnected();
   WebSocketReadResult readMessage(ByteStream *message, WebSocketMessageType *type, uint32_t timeout);

   bool sendText(const char *text, ssize_t length = -1);
   bool sendBinary(const void *data, size_t size);
   bool sendPing(const void *data = nullptr, size_t size = 0);
   bool sendClose(uint16_t code = WEBSOCKET_CLOSE_NORMAL, const char *reason = nullptr);
   void close(uint16_t code = WEBSOCKET_CLOSE_NORMAL, const char *reason = nullptr, uint32_t timeout = 5000);
   void disconnect();

   /**
    * Get close status code received from peer (WEBSOCKET_CLOSE_NO_STATUS if close frame had no code,
    * WEBSOCKET_CLOSE_ABNORMAL if connection was lost without close frame)
    */
   uint16_t getCloseCode() const { return m_closeCode; }

   /**
    * Get close reason received from peer (empty string if not provided)
    */
   const char *getCloseReason() const { return m_closeReason; }

   /**
    * Get description of last connection error
    */
   const TCHAR *getErrorText() const { return m_errorText; }

   static bool parseURL(const char *url, char *host, size_t hostSize, uint16_t *port, bool *secure, char *path, size_t pathSize);
};

#endif   /* _websocket_h_ */
