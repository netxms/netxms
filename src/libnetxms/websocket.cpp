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
** File: websocket.cpp
**
**/

#include "libnetxms.h"
#include <websocket.h>
#include <nxsocket.h>
#include <netxms-version.h>

/**
 * Frame opcodes (RFC 6455 section 5.2)
 */
#define WS_OPCODE_CONTINUATION   0x0
#define WS_OPCODE_TEXT           0x1
#define WS_OPCODE_BINARY         0x2
#define WS_OPCODE_CLOSE          0x8
#define WS_OPCODE_PING           0x9
#define WS_OPCODE_PONG           0xA

/**
 * GUID appended to client key when computing Sec-WebSocket-Accept (RFC 6455 section 4.2.2)
 */
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/**
 * Limits
 */
#define MAX_HANDSHAKE_RESPONSE_SIZE    65536
#define MAX_FRAME_HEADER_SIZE          14
#define MAX_CONTROL_PAYLOAD_SIZE       125
#define RECEIVE_CHUNK_SIZE             16384
#define RECEIVE_CALL_TIMEOUT           1000

/**
 * Constructor
 */
WebSocketClient::WebSocketClient(const TCHAR *debugTag) : m_readBuffer(RECEIVE_CHUNK_SIZE * 2), m_messageBuffer(RECEIVE_CHUNK_SIZE)
{
   _tcslcpy(m_debugTag, CHECK_NULL_EX(debugTag), sizeof(m_debugTag) / sizeof(TCHAR));
   m_connection = nullptr;
   m_socket = INVALID_SOCKET;
   m_verifyPeer = false;
   m_maxMessageSize = WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE;
   m_extraHeaders = nullptr;
   m_host[0] = 0;
   m_port = 0;
   m_secure = false;
   m_path[0] = 0;
   m_messageType = WebSocketMessageType::TEXT;
   m_messageInProgress = false;
   m_closeSent = false;
   m_closeReceived = false;
   m_closeCode = WEBSOCKET_CLOSE_ABNORMAL;
   m_closeReason[0] = 0;
   m_errorText[0] = 0;
}

/**
 * Destructor
 */
WebSocketClient::~WebSocketClient()
{
   dropConnection();
   MemFree(m_extraHeaders);
}

/**
 * Add header to be sent with handshake request (for example, "Authorization: Bearer ..."). Must be called before connect().
 */
void WebSocketClient::addHeader(const char *name, const char *value)
{
   size_t currLen = (m_extraHeaders != nullptr) ? strlen(m_extraHeaders) : 0;
   size_t addLen = strlen(name) + strlen(value) + 4;
   m_extraHeaders = MemRealloc(m_extraHeaders, currLen + addLen + 1);
   snprintf(m_extraHeaders + currLen, addLen + 1, "%s: %s\r\n", name, value);
}

/**
 * Parse WebSocket URL. Accepted schemes are ws, wss, http, and https (http and https map to ws and wss respectively,
 * so server URLs from existing HTTP based configuration can be used directly). IPv6 literals must be enclosed in brackets.
 * Path is always written with leading slash and includes query string if present.
 */
bool WebSocketClient::parseURL(const char *url, char *host, size_t hostSize, uint16_t *port, bool *secure, char *path, size_t pathSize)
{
   const char *p;
   if (!strnicmp(url, "wss://", 6))
   {
      *secure = true;
      p = url + 6;
   }
   else if (!strnicmp(url, "ws://", 5))
   {
      *secure = false;
      p = url + 5;
   }
   else if (!strnicmp(url, "https://", 8))
   {
      *secure = true;
      p = url + 8;
   }
   else if (!strnicmp(url, "http://", 7))
   {
      *secure = false;
      p = url + 7;
   }
   else
   {
      return false;
   }

   const char *hostStart, *hostEnd;
   if (*p == '[')
   {
      hostStart = p + 1;
      hostEnd = strchr(hostStart, ']');
      if (hostEnd == nullptr)
         return false;
      p = hostEnd + 1;
   }
   else
   {
      hostStart = p;
      while((*p != 0) && (*p != ':') && (*p != '/') && (*p != '?'))
         p++;
      hostEnd = p;
   }

   size_t hostLen = hostEnd - hostStart;
   if ((hostLen == 0) || (hostLen >= hostSize))
      return false;
   memcpy(host, hostStart, hostLen);
   host[hostLen] = 0;

   *port = *secure ? 443 : 80;
   if (*p == ':')
   {
      p++;
      char *eptr;
      unsigned long value = strtoul(p, &eptr, 10);
      if ((eptr == p) || (value == 0) || (value > 65535) || ((*eptr != 0) && (*eptr != '/') && (*eptr != '?')))
         return false;
      *port = static_cast<uint16_t>(value);
      p = eptr;
   }

   if (*p == 0)
   {
      strlcpy(path, "/", pathSize);
   }
   else if (*p == '/')
   {
      if (strlen(p) >= pathSize)
         return false;
      strcpy(path, p);
   }
   else if (*p == '?')
   {
      if (strlen(p) + 1 >= pathSize)
         return false;
      path[0] = '/';
      strcpy(path + 1, p);
   }
   else
   {
      return false;
   }
   return true;
}

/**
 * Check if connection is established
 */
bool WebSocketClient::isConnected()
{
   LockGuard lockGuard(m_lock);
   return m_connection != nullptr;
}

/**
 * Connect to server and perform WebSocket opening handshake.
 * Timeout applies separately to TCP connect, TLS handshake, and WebSocket handshake phases.
 */
bool WebSocketClient::connect(const char *url, uint32_t timeout)
{
   if (isConnected())
   {
      _tcslcpy(m_errorText, _T("Already connected"), sizeof(m_errorText) / sizeof(TCHAR));
      return false;
   }

   if (!parseURL(url, m_host, sizeof(m_host), &m_port, &m_secure, m_path, sizeof(m_path)))
   {
      _sntprintf(m_errorText, sizeof(m_errorText) / sizeof(TCHAR), _T("Invalid WebSocket URL \"%hs\""), url);
      nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
      return false;
   }

   if (timeout == 0)
      timeout = TLS_CONN_DEFAULT_TIMEOUT;

   InetAddress addr = InetAddress::resolveHostName(m_host);
   if (!addr.isValidUnicast() && !addr.isLoopback())
   {
      _sntprintf(m_errorText, sizeof(m_errorText) / sizeof(TCHAR), _T("Cannot resolve host name \"%hs\""), m_host);
      nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
      return false;
   }

   TLSConnection *connection = new TLSConnection(m_debugTag);
   if (m_verifyPeer)
      connection->enablePeerVerification();
   if (!connection->connect(addr, m_port, m_secure, timeout, m_host))
   {
      TCHAR ipAddrText[64];
      _sntprintf(m_errorText, sizeof(m_errorText) / sizeof(TCHAR), _T("Cannot establish %s connection to %hs:%u (%s)"),
            m_secure ? _T("TLS") : _T("TCP"), m_host, m_port, addr.toString(ipAddrText));
      nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
      delete connection;
      return false;
   }

   m_readBuffer.clear();
   m_messageBuffer.clear();
   m_messageInProgress = false;
   m_closeSent = false;
   m_closeReceived = false;
   m_closeCode = WEBSOCKET_CLOSE_ABNORMAL;
   m_closeReason[0] = 0;
   m_errorText[0] = 0;

   m_lock.lock();
   m_connection = connection;
   m_socket = connection->getSocket();
   m_lock.unlock();

   if (!performHandshake(GetMonotonicClockTime() + timeout))
   {
      dropConnection();
      return false;
   }

   nxlog_debug_tag(m_debugTag, 5, _T("WebSocket connection to %hs established"), url);
   return true;
}

/**
 * Find header in handshake response and copy its trimmed value to given buffer.
 * @return true if header is present
 */
static bool FindHeader(const char *headers, const char *name, char *value, size_t valueSize)
{
   size_t nameLen = strlen(name);
   for(const char *line = headers; *line != 0; )
   {
      const char *end = strstr(line, "\r\n");
      size_t lineLen = (end != nullptr) ? static_cast<size_t>(end - line) : strlen(line);
      if ((lineLen > nameLen) && !strnicmp(line, name, nameLen) && (line[nameLen] == ':'))
      {
         const char *start = line + nameLen + 1;
         while((start < line + lineLen) && isspace(*start))
            start++;
         size_t copyLen = std::min(static_cast<size_t>(line + lineLen - start), valueSize - 1);
         memcpy(value, start, copyLen);
         value[copyLen] = 0;
         TrimA(value);
         return true;
      }
      if (end == nullptr)
         break;
      line = end + 2;
   }
   return false;
}

/**
 * Perform WebSocket opening handshake (RFC 6455 section 4)
 */
bool WebSocketClient::performHandshake(int64_t deadline)
{
   BYTE nonce[16];
   GenerateRandomBytes(nonce, sizeof(nonce));
   char key[BASE64_LENGTH(sizeof(nonce)) + 1];
   base64_encode(reinterpret_cast<char*>(nonce), sizeof(nonce), key, sizeof(key));

   char keyAndGuid[BASE64_LENGTH(sizeof(nonce)) + sizeof(WS_GUID)];
   strcpy(keyAndGuid, key);
   strcat(keyAndGuid, WS_GUID);
   BYTE hash[SHA1_DIGEST_SIZE];
   CalculateSHA1Hash(keyAndGuid, strlen(keyAndGuid), hash);
   char expectedAccept[BASE64_LENGTH(SHA1_DIGEST_SIZE) + 1];
   base64_encode(reinterpret_cast<char*>(hash), SHA1_DIGEST_SIZE, expectedAccept, sizeof(expectedAccept));

   char hostHeader[sizeof(m_host) + 16];
   bool ipv6Literal = (strchr(m_host, ':') != nullptr);
   if (m_port == (m_secure ? 443 : 80))
      snprintf(hostHeader, sizeof(hostHeader), ipv6Literal ? "[%s]" : "%s", m_host);
   else
      snprintf(hostHeader, sizeof(hostHeader), ipv6Literal ? "[%s]:%u" : "%s:%u", m_host, m_port);

   ByteStream request(4096);
   char line[sizeof(m_path) + 64];
   snprintf(line, sizeof(line), "GET %s HTTP/1.1\r\n", m_path);
   request.write(line, strlen(line));
   snprintf(line, sizeof(line), "Host: %s\r\n", hostHeader);
   request.write(line, strlen(line));
   static const char fixedHeaders[] = "Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nUser-Agent: NetXMS/" NETXMS_VERSION_STRING_A "\r\n";
   request.write(fixedHeaders, sizeof(fixedHeaders) - 1);
   snprintf(line, sizeof(line), "Sec-WebSocket-Key: %s\r\n", key);
   request.write(line, strlen(line));
   if (m_extraHeaders != nullptr)
      request.write(m_extraHeaders, strlen(m_extraHeaders));
   request.write("\r\n", 2);

   int64_t remaining = deadline - GetMonotonicClockTime();
   if (remaining <= 0)
   {
      _tcslcpy(m_errorText, _T("Handshake timeout"), sizeof(m_errorText) / sizeof(TCHAR));
      return false;
   }

   m_lock.lock();
   ssize_t sent = m_connection->send(request.buffer(), request.size(), static_cast<uint32_t>(remaining));
   m_lock.unlock();
   if (sent != static_cast<ssize_t>(request.size()))
   {
      _tcslcpy(m_errorText, _T("Cannot send handshake request"), sizeof(m_errorText) / sizeof(TCHAR));
      nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
      return false;
   }

   // Read response headers; anything received past end of headers is the start of frame data
   size_t headersEnd;
   while(true)
   {
      size_t size;
      const BYTE *data = m_readBuffer.buffer(&size);
      const BYTE *end = (size >= 4) ? reinterpret_cast<const BYTE*>(memmem(data, size, "\r\n\r\n", 4)) : nullptr;
      if (end != nullptr)
      {
         headersEnd = end - data;
         break;
      }

      if (size > MAX_HANDSHAKE_RESPONSE_SIZE)
      {
         _tcslcpy(m_errorText, _T("Handshake response too large"), sizeof(m_errorText) / sizeof(TCHAR));
         nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
         return false;
      }

      remaining = deadline - GetMonotonicClockTime();
      if (remaining <= 0)
      {
         _tcslcpy(m_errorText, _T("Handshake timeout"), sizeof(m_errorText) / sizeof(TCHAR));
         nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
         return false;
      }

      ssize_t bytes = readChunk(static_cast<uint32_t>(remaining));
      if (bytes == -2)
         continue;
      if (bytes <= 0)
      {
         _tcslcpy(m_errorText, (bytes == 0) ? _T("Connection closed by server during handshake") : _T("Communication failure during handshake"), sizeof(m_errorText) / sizeof(TCHAR));
         nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
         return false;
      }
   }

   Buffer<char, 4096> headers(headersEnd + 1);
   memcpy(headers, m_readBuffer.buffer(), headersEnd);
   headers[headersEnd] = 0;
   m_readBuffer.truncateLeft(headersEnd + 4);

   bool success = false;
   char *statusLine = headers;
   char *headerLines = strstr(headers, "\r\n");
   if (headerLines != nullptr)
   {
      *headerLines = 0;
      headerLines += 2;
   }
   nxlog_debug_tag(m_debugTag, 7, _T("Handshake response: %hs"), statusLine);

   if (strncmp(statusLine, "HTTP/1.", 7) || (strlen(statusLine) < 12) || (statusLine[8] != ' '))
   {
      _tcslcpy(m_errorText, _T("Malformed handshake response"), sizeof(m_errorText) / sizeof(TCHAR));
   }
   else if (strncmp(&statusLine[9], "101", 3))
   {
      _sntprintf(m_errorText, sizeof(m_errorText) / sizeof(TCHAR), _T("Server rejected WebSocket upgrade (%hs)"), &statusLine[9]);
   }
   else if (headerLines == nullptr)
   {
      _tcslcpy(m_errorText, _T("Missing handshake response headers"), sizeof(m_errorText) / sizeof(TCHAR));
   }
   else
   {
      char upgrade[64], connection[256], accept[64];
      if (!FindHeader(headerLines, "Upgrade", upgrade, sizeof(upgrade)) || stricmp(upgrade, "websocket"))
         _tcslcpy(m_errorText, _T("Invalid or missing Upgrade header in handshake response"), sizeof(m_errorText) / sizeof(TCHAR));
      else if (!FindHeader(headerLines, "Connection", connection, sizeof(connection)) || (strcasestr(connection, "upgrade") == nullptr))
         _tcslcpy(m_errorText, _T("Invalid or missing Connection header in handshake response"), sizeof(m_errorText) / sizeof(TCHAR));
      else if (!FindHeader(headerLines, "Sec-WebSocket-Accept", accept, sizeof(accept)) || strcmp(accept, expectedAccept))
         _tcslcpy(m_errorText, _T("Invalid or missing Sec-WebSocket-Accept header in handshake response"), sizeof(m_errorText) / sizeof(TCHAR));
      else
         success = true;
   }

   if (!success)
      nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
   return success;
}

/**
 * Read next chunk of data from connection into read buffer. Waits for data without holding connection lock,
 * so that other threads can send while reader is waiting.
 * @return number of bytes read on success, 0 if connection was closed, -1 on error, -2 on timeout
 */
ssize_t WebSocketClient::readChunk(uint32_t timeout)
{
   m_lock.lock();
   if (m_connection == nullptr)
   {
      m_lock.unlock();
      return 0;
   }
   bool pending = m_connection->hasPendingData();
   m_lock.unlock();

   if (!pending)
   {
      SocketPoller sp;
      sp.add(m_socket);
      int rc = sp.poll(timeout);
      if (rc == 0)
         return -2;
      if (rc < 0)
         return -1;
   }

   BYTE buffer[RECEIVE_CHUNK_SIZE];
   m_lock.lock();
   if (m_connection == nullptr)
   {
      m_lock.unlock();
      return 0;
   }
   ssize_t bytes = m_connection->recv(buffer, sizeof(buffer), RECEIVE_CALL_TIMEOUT);
   m_lock.unlock();

   if (bytes > 0)
      m_readBuffer.write(buffer, bytes);
   else if ((bytes == -1) && ((WSAGetLastError() == WSAEWOULDBLOCK) || (errno == EAGAIN)))
      bytes = -2;  // spurious poll wakeup on non-blocking socket
   return bytes;
}

/**
 * Send single frame with given opcode and payload. Payload is masked as required for client-to-server frames.
 */
bool WebSocketClient::sendFrame(uint8_t opcode, const void *payload, size_t size)
{
   Buffer<BYTE, 4096> frame(size + MAX_FRAME_HEADER_SIZE);
   frame[0] = static_cast<BYTE>(0x80 | opcode);  // FIN + opcode
   size_t headerSize;
   if (size < 126)
   {
      frame[1] = static_cast<BYTE>(0x80 | size);
      headerSize = 2;
   }
   else if (size <= 0xFFFF)
   {
      frame[1] = 0x80 | 126;
      frame[2] = static_cast<BYTE>(size >> 8);
      frame[3] = static_cast<BYTE>(size & 0xFF);
      headerSize = 4;
   }
   else
   {
      frame[1] = 0x80 | 127;
      uint64_t n = HostToBigEndian64(static_cast<uint64_t>(size));
      memcpy(&frame[2], &n, 8);
      headerSize = 10;
   }

   BYTE *mask = &frame[headerSize];
   GenerateRandomBytes(mask, 4);
   headerSize += 4;

   const BYTE *src = static_cast<const BYTE*>(payload);
   BYTE *dst = &frame[headerSize];
   for(size_t i = 0; i < size; i++)
      dst[i] = src[i] ^ mask[i & 3];

   size_t frameSize = headerSize + size;
   bool success;
   m_lock.lock();
   if ((m_connection != nullptr) && (!m_closeSent || (opcode >= WS_OPCODE_CLOSE)))
   {
      ssize_t sent = m_connection->send(frame, frameSize);
      success = (sent == static_cast<ssize_t>(frameSize));
      if (!success)
         nxlog_debug_tag(m_debugTag, 6, _T("Cannot send frame (opcode=%u size=%u)"), opcode, static_cast<unsigned int>(size));
   }
   else
   {
      success = false;
   }
   m_lock.unlock();
   return success;
}

/**
 * Send close frame with given status code and optional reason
 */
bool WebSocketClient::sendCloseFrame(uint16_t code, const char *reason)
{
   BYTE payload[MAX_CONTROL_PAYLOAD_SIZE];
   payload[0] = static_cast<BYTE>(code >> 8);
   payload[1] = static_cast<BYTE>(code & 0xFF);
   size_t size = 2;
   if (reason != nullptr)
   {
      size_t reasonLen = std::min(strlen(reason), static_cast<size_t>(MAX_CONTROL_PAYLOAD_SIZE - 2));
      memcpy(&payload[2], reason, reasonLen);
      size += reasonLen;
   }
   return sendFrame(WS_OPCODE_CLOSE, payload, size);
}

/**
 * Send text message
 */
bool WebSocketClient::sendText(const char *text, ssize_t length)
{
   return sendFrame(WS_OPCODE_TEXT, text, (length < 0) ? strlen(text) : static_cast<size_t>(length));
}

/**
 * Send binary message
 */
bool WebSocketClient::sendBinary(const void *data, size_t size)
{
   return sendFrame(WS_OPCODE_BINARY, data, size);
}

/**
 * Send ping frame with optional application data (up to 125 bytes)
 */
bool WebSocketClient::sendPing(const void *data, size_t size)
{
   return sendFrame(WS_OPCODE_PING, data, std::min(size, static_cast<size_t>(MAX_CONTROL_PAYLOAD_SIZE)));
}

/**
 * Initiate close handshake without waiting for completion. Subsequent readMessage() on the reader thread
 * returns WebSocketReadResult::CLOSED when peer's close frame arrives.
 */
bool WebSocketClient::sendClose(uint16_t code, const char *reason)
{
   if (m_closeSent)
      return true;
   nxlog_debug_tag(m_debugTag, 6, _T("Sending close frame (code=%u)"), code);
   m_closeSent = true;
   return sendCloseFrame(code, reason);
}

/**
 * Perform close handshake: send close frame, wait for peer's close frame up to given timeout, and close connection.
 * Must be called from the reader thread.
 */
void WebSocketClient::close(uint16_t code, const char *reason, uint32_t timeout)
{
   if (!isConnected())
      return;

   sendClose(code, reason);

   int64_t deadline = GetMonotonicClockTime() + timeout;
   ByteStream message;
   WebSocketMessageType type;
   while(isConnected())
   {
      int64_t remaining = deadline - GetMonotonicClockTime();
      if (remaining <= 0)
      {
         nxlog_debug_tag(m_debugTag, 6, _T("Timeout waiting for close frame from peer"));
         break;
      }
      WebSocketReadResult rc = readMessage(&message, &type, static_cast<uint32_t>(remaining));
      if ((rc == WebSocketReadResult::CLOSED) || (rc == WebSocketReadResult::FAILURE))
         break;
   }
   dropConnection();
}

/**
 * Shut down underlying socket without close handshake. Safe to call from any thread; causes readMessage()
 * in progress on the reader thread to return.
 */
void WebSocketClient::disconnect()
{
   LockGuard lockGuard(m_lock);
   if (m_connection != nullptr)
   {
      nxlog_debug_tag(m_debugTag, 6, _T("Shutting down connection socket"));
      shutdown(m_connection->getSocket(), SHUT_RDWR);
   }
}

/**
 * Close and delete underlying connection
 */
void WebSocketClient::dropConnection()
{
   LockGuard lockGuard(m_lock);
   delete_and_null(m_connection);
   m_socket = INVALID_SOCKET;
}

/**
 * Fail connection due to protocol violation or local limit: record error, send close frame with given
 * status code (best effort), and drop connection.
 */
void WebSocketClient::failConnection(uint16_t closeCode, const TCHAR *errorText)
{
   _tcslcpy(m_errorText, errorText, sizeof(m_errorText) / sizeof(TCHAR));
   nxlog_debug_tag(m_debugTag, 5, _T("Connection failed: %s"), errorText);
   if (!m_closeSent)
   {
      m_closeSent = true;
      sendCloseFrame(closeCode, nullptr);
   }
   dropConnection();
}

/**
 * Process frames accumulated in read buffer.
 * @return WebSocketReadResult::MESSAGE when complete message was assembled, CLOSED or FAILURE when connection
 * was closed, and TIMEOUT when more data is needed
 */
WebSocketReadResult WebSocketClient::processFrame(ByteStream *message, WebSocketMessageType *type)
{
   while(true)
   {
      size_t available;
      const BYTE *data = m_readBuffer.buffer(&available);
      if (available < 2)
         return WebSocketReadResult::TIMEOUT;

      if (data[0] & 0x70)
      {
         failConnection(WEBSOCKET_CLOSE_PROTOCOL_ERROR, _T("Reserved bits set in frame header"));
         return WebSocketReadResult::FAILURE;
      }

      bool fin = (data[0] & 0x80) != 0;
      uint8_t opcode = data[0] & 0x0F;
      bool masked = (data[1] & 0x80) != 0;
      uint64_t payloadSize = data[1] & 0x7F;
      size_t headerSize = 2;
      if (payloadSize == 126)
      {
         if (available < 4)
            return WebSocketReadResult::TIMEOUT;
         payloadSize = (static_cast<uint64_t>(data[2]) << 8) | data[3];
         headerSize = 4;
      }
      else if (payloadSize == 127)
      {
         if (available < 10)
            return WebSocketReadResult::TIMEOUT;
         uint64_t n;
         memcpy(&n, &data[2], 8);
         payloadSize = BigEndianToHost64(n);
         headerSize = 10;
         if (payloadSize & 0x8000000000000000ULL)
         {
            failConnection(WEBSOCKET_CLOSE_PROTOCOL_ERROR, _T("Invalid payload length"));
            return WebSocketReadResult::FAILURE;
         }
      }

      if (masked)
      {
         failConnection(WEBSOCKET_CLOSE_PROTOCOL_ERROR, _T("Received masked frame from server"));
         return WebSocketReadResult::FAILURE;
      }

      if (opcode >= WS_OPCODE_CLOSE)
      {
         if (!fin || (payloadSize > MAX_CONTROL_PAYLOAD_SIZE))
         {
            failConnection(WEBSOCKET_CLOSE_PROTOCOL_ERROR, _T("Invalid control frame"));
            return WebSocketReadResult::FAILURE;
         }
      }
      else if ((payloadSize > m_maxMessageSize) || (m_messageBuffer.size() + payloadSize > m_maxMessageSize))
      {
         failConnection(WEBSOCKET_CLOSE_MESSAGE_TOO_BIG, _T("Incoming message exceeds size limit"));
         return WebSocketReadResult::FAILURE;
      }

      if (available < headerSize + payloadSize)
         return WebSocketReadResult::TIMEOUT;

      const BYTE *payload = data + headerSize;
      size_t size = static_cast<size_t>(payloadSize);
      WebSocketReadResult result = WebSocketReadResult::TIMEOUT;
      switch(opcode)
      {
         case WS_OPCODE_CONTINUATION:
            if (!m_messageInProgress)
            {
               failConnection(WEBSOCKET_CLOSE_PROTOCOL_ERROR, _T("Unexpected continuation frame"));
               return WebSocketReadResult::FAILURE;
            }
            m_messageBuffer.write(payload, size);
            break;
         case WS_OPCODE_TEXT:
         case WS_OPCODE_BINARY:
            if (m_messageInProgress)
            {
               failConnection(WEBSOCKET_CLOSE_PROTOCOL_ERROR, _T("New data frame received while fragmented message is in progress"));
               return WebSocketReadResult::FAILURE;
            }
            m_messageType = (opcode == WS_OPCODE_TEXT) ? WebSocketMessageType::TEXT : WebSocketMessageType::BINARY;
            m_messageBuffer.clear();
            m_messageBuffer.write(payload, size);
            m_messageInProgress = true;
            break;
         case WS_OPCODE_CLOSE:
            m_closeReceived = true;
            if (size >= 2)
            {
               m_closeCode = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
               size_t reasonLen = std::min(size - 2, sizeof(m_closeReason) - 1);
               memcpy(m_closeReason, payload + 2, reasonLen);
               m_closeReason[reasonLen] = 0;
            }
            else
            {
               m_closeCode = WEBSOCKET_CLOSE_NO_STATUS;
               m_closeReason[0] = 0;
            }
            nxlog_debug_tag(m_debugTag, 6, _T("Close frame received (code=%u reason=\"%hs\")"), m_closeCode, m_closeReason);
            if (!m_closeSent)
            {
               m_closeSent = true;
               sendCloseFrame((size >= 2) ? m_closeCode : WEBSOCKET_CLOSE_NORMAL, nullptr);
            }
            result = WebSocketReadResult::CLOSED;
            break;
         case WS_OPCODE_PING:
            nxlog_debug_tag(m_debugTag, 8, _T("Ping received (%u bytes)"), static_cast<unsigned int>(size));
            sendFrame(WS_OPCODE_PONG, payload, size);
            break;
         case WS_OPCODE_PONG:
            nxlog_debug_tag(m_debugTag, 8, _T("Pong received (%u bytes)"), static_cast<unsigned int>(size));
            break;
         default:
            failConnection(WEBSOCKET_CLOSE_PROTOCOL_ERROR, _T("Unknown frame opcode"));
            return WebSocketReadResult::FAILURE;
      }

      if ((opcode < WS_OPCODE_CLOSE) && fin)
      {
         message->clear();
         message->write(m_messageBuffer.buffer(), m_messageBuffer.size());
         *type = m_messageType;
         m_messageBuffer.clear();
         m_messageInProgress = false;
         result = WebSocketReadResult::MESSAGE;
      }

      m_readBuffer.truncateLeft(headerSize + size);

      if (result == WebSocketReadResult::CLOSED)
      {
         dropConnection();
         return result;
      }
      if (result != WebSocketReadResult::TIMEOUT)
         return result;
   }
}

/**
 * Read next complete message. Control frames (ping, pong, close) are handled internally.
 * @param message buffer to receive message payload (cleared before writing)
 * @param type receives message type
 * @param timeout wait timeout in milliseconds or INFINITE
 */
WebSocketReadResult WebSocketClient::readMessage(ByteStream *message, WebSocketMessageType *type, uint32_t timeout)
{
   int64_t deadline = (timeout == INFINITE) ? INT64_MAX : GetMonotonicClockTime() + timeout;
   while(true)
   {
      WebSocketReadResult result = processFrame(message, type);
      if (result != WebSocketReadResult::TIMEOUT)
         return result;

      if (!isConnected())
         return (m_closeSent || m_closeReceived) ? WebSocketReadResult::CLOSED : WebSocketReadResult::FAILURE;

      int64_t remaining = deadline - GetMonotonicClockTime();
      if (remaining <= 0)
         return WebSocketReadResult::TIMEOUT;

      ssize_t bytes = readChunk((remaining > static_cast<int64_t>(0xFFFFFFFE)) ? 0xFFFFFFFE : static_cast<uint32_t>(remaining));
      if (bytes == -2)
         continue;
      if (bytes <= 0)
      {
         if (m_closeSent || m_closeReceived)
         {
            nxlog_debug_tag(m_debugTag, 6, _T("Connection closed by peer after close handshake"));
            dropConnection();
            return WebSocketReadResult::CLOSED;
         }
         _tcslcpy(m_errorText, (bytes == 0) ? _T("Connection closed by peer") : _T("Communication failure"), sizeof(m_errorText) / sizeof(TCHAR));
         nxlog_debug_tag(m_debugTag, 5, _T("%s"), m_errorText);
         m_closeCode = WEBSOCKET_CLOSE_ABNORMAL;
         dropConnection();
         return WebSocketReadResult::FAILURE;
      }
   }
}
