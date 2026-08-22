#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxsocket.h>
#include <websocket.h>
#include <testtools.h>

/**
 * Loopback test server behavior modes
 */
enum class ServerMode
{
   ECHO,             // normal RFC 6455 echo server
   REJECT,           // respond with HTTP 401 to handshake
   BAD_ACCEPT_KEY    // respond with 101 but wrong Sec-WebSocket-Accept
};

/**
 * Loopback test server state
 */
struct TestServer
{
   SOCKET listener;
   uint16_t port;
   THREAD thread;
   volatile bool stop;
   volatile ServerMode mode;
   volatile bool pongReceived;
   volatile bool closeReceived;
   volatile uint16_t clientCloseCode;
   volatile bool extraHeaderSeen;
};

/**
 * Read frame sent by client (must be masked)
 */
static bool ReadClientFrame(SOCKET s, BYTE *opcode, bool *fin, ByteStream *payload)
{
   BYTE header[2];
   if (!RecvAll(s, header, 2, 5000))
      return false;
   *fin = (header[0] & 0x80) != 0;
   *opcode = header[0] & 0x0F;
   if (!(header[1] & 0x80))
      return false;   // client frames must be masked

   size_t size = header[1] & 0x7F;
   if (size == 126)
   {
      BYTE ext[2];
      if (!RecvAll(s, ext, 2, 5000))
         return false;
      size = (static_cast<size_t>(ext[0]) << 8) | ext[1];
   }
   else if (size == 127)
   {
      BYTE ext[8];
      if (!RecvAll(s, ext, 8, 5000))
         return false;
      uint64_t n;
      memcpy(&n, ext, 8);
      size = static_cast<size_t>(BigEndianToHost64(n));
   }

   BYTE mask[4];
   if (!RecvAll(s, mask, 4, 5000))
      return false;

   BYTE *data = MemAllocArrayNoInit<BYTE>(size + 1);
   if (!RecvAll(s, data, size, 5000))
   {
      MemFree(data);
      return false;
   }
   for(size_t i = 0; i < size; i++)
      data[i] ^= mask[i & 3];
   payload->clear();
   payload->write(data, size);
   MemFree(data);
   return true;
}

/**
 * Send unmasked frame to client
 */
static void SendServerFrame(SOCKET s, BYTE opcode, bool fin, const void *data, size_t size)
{
   ByteStream frame(size + 16);
   frame.write(static_cast<BYTE>((fin ? 0x80 : 0) | opcode));
   if (size < 126)
   {
      frame.write(static_cast<BYTE>(size));
   }
   else if (size <= 0xFFFF)
   {
      frame.write(static_cast<BYTE>(126));
      frame.writeB(static_cast<uint16_t>(size));
   }
   else
   {
      frame.write(static_cast<BYTE>(127));
      frame.writeB(static_cast<uint64_t>(size));
   }
   frame.write(data, size);
   SendEx(s, frame.buffer(), frame.size(), 0, nullptr);
}

/**
 * Send close frame to client
 */
static void SendServerClose(SOCKET s, uint16_t code, const char *reason)
{
   BYTE payload[128];
   payload[0] = static_cast<BYTE>(code >> 8);
   payload[1] = static_cast<BYTE>(code & 0xFF);
   size_t len = strlen(reason);
   memcpy(&payload[2], reason, len);
   SendServerFrame(s, 0x8, true, payload, len + 2);
}

/**
 * Handle single client connection
 */
static void HandleConnection(TestServer *server, SOCKET s)
{
   // Read handshake request
   char request[8192];
   size_t total = 0;
   while(true)
   {
      ssize_t bytes = RecvEx(s, request + total, sizeof(request) - total - 1, 0, 5000);
      if (bytes <= 0)
         return;
      total += bytes;
      request[total] = 0;
      if (strstr(request, "\r\n\r\n") != nullptr)
         break;
      if (total >= sizeof(request) - 1)
         return;
   }

   if (strncmp(request, "GET /test/socket?v=1 HTTP/1.1\r\n", 31) || (strstr(request, "\r\nUpgrade: websocket\r\n") == nullptr) ||
       (strstr(request, "\r\nSec-WebSocket-Version: 13\r\n") == nullptr) || (strstr(request, "\r\nHost: 127.0.0.1:") == nullptr))
   {
      static const char response[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
      SendEx(s, response, sizeof(response) - 1, 0, nullptr);
      return;
   }
   server->extraHeaderSeen = (strstr(request, "\r\nAuthorization: Bearer test-token\r\n") != nullptr);

   if (server->mode == ServerMode::REJECT)
   {
      static const char response[] = "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\n\r\n";
      SendEx(s, response, sizeof(response) - 1, 0, nullptr);
      return;
   }

   const char *key = strstr(request, "\r\nSec-WebSocket-Key: ");
   if (key == nullptr)
      return;
   key += 21;
   char keyAndGuid[128];
   size_t keyLen = strstr(key, "\r\n") - key;
   memcpy(keyAndGuid, key, keyLen);
   strcpy(&keyAndGuid[keyLen], "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
   BYTE hash[SHA1_DIGEST_SIZE];
   CalculateSHA1Hash(keyAndGuid, strlen(keyAndGuid), hash);
   char accept[BASE64_LENGTH(SHA1_DIGEST_SIZE) + 1];
   base64_encode(reinterpret_cast<char*>(hash), SHA1_DIGEST_SIZE, accept, sizeof(accept));
   if (server->mode == ServerMode::BAD_ACCEPT_KEY)
      accept[0] = (accept[0] == 'A') ? 'B' : 'A';

   char response[256];
   snprintf(response, sizeof(response), "HTTP/1.1 101 Switching Protocols\r\nupgrade: WebSocket\r\nConnection: keep-alive, Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", accept);
   SendEx(s, response, strlen(response), 0, nullptr);

   // Ping client right away; client must respond with pong carrying the same payload
   SendServerFrame(s, 0x9, true, "srv-ping", 8);

   ByteStream payload;
   while(true)
   {
      BYTE opcode;
      bool fin;
      if (!ReadClientFrame(s, &opcode, &fin, &payload))
         break;

      if ((opcode == 0x1) || (opcode == 0x2))
      {
         size_t size;
         const BYTE *data = payload.buffer(&size);
         if ((opcode == 0x1) && (size == 4) && !memcmp(data, "FRAG", 4))
         {
            SendServerFrame(s, 0x1, false, "part1", 5);
            SendServerFrame(s, 0x0, false, "part2", 5);
            SendServerFrame(s, 0x9, true, "mid", 3);   // control frame interleaved with fragments
            SendServerFrame(s, 0x0, true, "part3", 5);
         }
         else if ((opcode == 0x1) && (size == 3) && !memcmp(data, "BYE", 3))
         {
            SendServerClose(s, WEBSOCKET_CLOSE_GOING_AWAY, "going away");
         }
         else if ((opcode == 0x1) && (size == 3) && !memcmp(data, "BIG", 3))
         {
            BYTE big[2000];
            memset(big, 'x', sizeof(big));
            SendServerFrame(s, 0x2, true, big, sizeof(big));
         }
         else if ((opcode == 0x1) && (size == 4) && !memcmp(data, "MASK", 4))
         {
            BYTE frame[] = { 0x81, 0x82, 0x01, 0x02, 0x03, 0x04 };   // masked frame from server is a protocol violation
            SendEx(s, frame, sizeof(frame), 0, nullptr);
         }
         else
         {
            SendServerFrame(s, opcode, true, data, size);
         }
      }
      else if (opcode == 0xA)
      {
         size_t size;
         const BYTE *data = payload.buffer(&size);
         if ((size == 8) && !memcmp(data, "srv-ping", 8))
            server->pongReceived = true;
      }
      else if (opcode == 0x9)
      {
         SendServerFrame(s, 0xA, true, payload.buffer(), payload.size());
      }
      else if (opcode == 0x8)
      {
         size_t size;
         const BYTE *data = payload.buffer(&size);
         server->clientCloseCode = (size >= 2) ? ((static_cast<uint16_t>(data[0]) << 8) | data[1]) : 0;
         server->closeReceived = true;
         SendServerFrame(s, 0x8, true, data, size);
         break;
      }
   }
}

/**
 * Server thread
 */
static THREAD_RESULT THREAD_CALL ServerThread(void *arg)
{
   TestServer *server = static_cast<TestServer*>(arg);
   while(!server->stop)
   {
      SocketPoller sp;
      sp.add(server->listener);
      if (sp.poll(100) <= 0)
         continue;
      SOCKET s = accept(server->listener, nullptr, nullptr);
      if (s == INVALID_SOCKET)
         continue;
      HandleConnection(server, s);
      shutdown(s, SHUT_RDWR);
      closesocket(s);
   }
   return THREAD_OK;
}

/**
 * Start loopback server on ephemeral port
 */
static void StartTestServer(TestServer *server)
{
   server->listener = CreateSocket(AF_INET, SOCK_STREAM, 0);
   AssertTrue(server->listener != INVALID_SOCKET);
   SetSocketReuseFlag(server->listener);

   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   sa.sin_port = 0;
   AssertEquals(bind(server->listener, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa)), 0);
   AssertEquals(listen(server->listener, 4), 0);

   socklen_t len = sizeof(sa);
   AssertEquals(getsockname(server->listener, reinterpret_cast<struct sockaddr*>(&sa), &len), 0);
   server->port = ntohs(sa.sin_port);

   server->stop = false;
   server->mode = ServerMode::ECHO;
   server->pongReceived = false;
   server->closeReceived = false;
   server->clientCloseCode = 0;
   server->extraHeaderSeen = false;
   server->thread = ThreadCreateEx(ServerThread, 0, server);
}

/**
 * Stop loopback server
 */
static void StopTestServer(TestServer *server)
{
   server->stop = true;
   ThreadJoin(server->thread);
   closesocket(server->listener);
}

/**
 * Wait for server-side flag with timeout
 */
static bool WaitForFlag(volatile bool *flag, uint32_t timeout)
{
   int64_t deadline = GetMonotonicClockTime() + timeout;
   while(!*flag && (GetMonotonicClockTime() < deadline))
      ThreadSleepMs(10);
   return *flag;
}

/**
 * URL parsing test vector
 */
struct URLTestVector
{
   const char *url;
   bool valid;
   const char *host;
   uint16_t port;
   bool secure;
   const char *path;
};

static URLTestVector s_urlTestVectors[] =
{
   { "ws://example.com/path", true, "example.com", 80, false, "/path" },
   { "wss://example.com", true, "example.com", 443, true, "/" },
   { "WSS://Example.com:8443/", true, "Example.com", 8443, true, "/" },
   { "http://mattermost.local:8065/api/v4/websocket", true, "mattermost.local", 8065, false, "/api/v4/websocket" },
   { "https://[::1]:9443/x?y=1&z=2", true, "::1", 9443, true, "/x?y=1&z=2" },
   { "ws://10.0.0.1?q=1", true, "10.0.0.1", 80, false, "/?q=1" },
   { "ftp://example.com/", false, nullptr, 0, false, nullptr },
   { "ws://", false, nullptr, 0, false, nullptr },
   { "ws://example.com:0/", false, nullptr, 0, false, nullptr },
   { "ws://example.com:70000/", false, nullptr, 0, false, nullptr },
   { "ws://example.com:abc/", false, nullptr, 0, false, nullptr },
   { "ws://[::1/", false, nullptr, 0, false, nullptr },
   { "ws://example.com:80x/", false, nullptr, 0, false, nullptr },
   { nullptr, false, nullptr, 0, false, nullptr }
};

/**
 * Test WebSocket URL parsing
 */
void TestWebSocketURLParsing()
{
   StartTest(_T("WebSocket URL parsing"));
   for(int i = 0; s_urlTestVectors[i].url != nullptr; i++)
   {
      char host[256], path[2048];
      uint16_t port;
      bool secure;
      bool valid = WebSocketClient::parseURL(s_urlTestVectors[i].url, host, sizeof(host), &port, &secure, path, sizeof(path));
      AssertEquals(valid, s_urlTestVectors[i].valid);
      if (valid)
      {
         AssertEquals(host, s_urlTestVectors[i].host);
         AssertEquals(port, s_urlTestVectors[i].port);
         AssertEquals(secure, s_urlTestVectors[i].secure);
         AssertEquals(path, s_urlTestVectors[i].path);
      }
   }
   EndTest();
}

/**
 * Context for disconnect thread
 */
struct DisconnectContext
{
   WebSocketClient *client;
   uint32_t delay;
};

/**
 * Thread calling disconnect() on client after delay
 */
static THREAD_RESULT THREAD_CALL DisconnectThread(void *arg)
{
   DisconnectContext *context = static_cast<DisconnectContext*>(arg);
   ThreadSleepMs(context->delay);
   context->client->disconnect();
   return THREAD_OK;
}

/**
 * Test WebSocket client against loopback server
 */
void TestWebSocketClient()
{
   TestServer server;
   StartTestServer(&server);

   char url[128];
   snprintf(url, sizeof(url), "ws://127.0.0.1:%u/test/socket?v=1", server.port);

   StartTest(_T("WebSocket handshake rejected by server"));
   server.mode = ServerMode::REJECT;
   {
      WebSocketClient client;
      client.addHeader("Authorization", "Bearer test-token");
      AssertFalse(client.connect(url, 5000));
      AssertFalse(client.isConnected());
      AssertNotNull(_tcsstr(client.getErrorText(), _T("401")));
      AssertTrue(server.extraHeaderSeen);
   }
   EndTest();

   StartTest(_T("WebSocket handshake with invalid accept key"));
   server.mode = ServerMode::BAD_ACCEPT_KEY;
   {
      WebSocketClient client;
      AssertFalse(client.connect(url, 5000));
      AssertNotNull(_tcsstr(client.getErrorText(), _T("Sec-WebSocket-Accept")));
   }
   EndTest();

   StartTest(_T("WebSocket connection refused"));
   {
      char badUrl[128];
      snprintf(badUrl, sizeof(badUrl), "ws://127.0.0.1:%u/", static_cast<unsigned int>(server.port + 1));
      WebSocketClient client;
      int64_t start = GetMonotonicClockTime();
      AssertFalse(client.connect(badUrl, 5000));
      AssertTrue(GetMonotonicClockTime() - start < 4000);
   }
   EndTest();

   server.mode = ServerMode::ECHO;

   StartTest(_T("WebSocket echo (short, medium, and large frames)"));
   {
      WebSocketClient client;
      AssertTrue(client.connect(url, 5000));
      AssertTrue(client.isConnected());

      ByteStream message;
      WebSocketMessageType type;

      AssertTrue(client.sendText("hello"));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::MESSAGE);
      AssertTrue(type == WebSocketMessageType::TEXT);
      AssertEquals(message.size(), static_cast<size_t>(5));
      AssertTrue(!memcmp(message.buffer(), "hello", 5));

      // Server pinged right after handshake; pong must have been sent automatically
      AssertTrue(WaitForFlag(&server.pongReceived, 2000));

      BYTE medium[1000];
      for(size_t i = 0; i < sizeof(medium); i++)
         medium[i] = static_cast<BYTE>(i * 7);
      AssertTrue(client.sendBinary(medium, sizeof(medium)));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::MESSAGE);
      AssertTrue(type == WebSocketMessageType::BINARY);
      AssertEquals(message.size(), sizeof(medium));
      AssertTrue(!memcmp(message.buffer(), medium, sizeof(medium)));

      size_t largeSize = 200000;
      char *large = MemAllocArrayNoInit<char>(largeSize);
      for(size_t i = 0; i < largeSize; i++)
         large[i] = static_cast<char>('a' + (i % 26));
      AssertTrue(client.sendText(large, largeSize));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::MESSAGE);
      AssertTrue(type == WebSocketMessageType::TEXT);
      AssertEquals(message.size(), largeSize);
      AssertTrue(!memcmp(message.buffer(), large, largeSize));
      MemFree(large);

      // Client-initiated ping is answered by server with pong, which is consumed silently
      AssertTrue(client.sendPing("cli-ping", 8));
      AssertTrue(client.sendText("after-ping"));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::MESSAGE);
      AssertEquals(message.size(), static_cast<size_t>(10));

      // No data pending: read must time out and leave connection intact
      int64_t start = GetMonotonicClockTime();
      AssertTrue(client.readMessage(&message, &type, 200) == WebSocketReadResult::TIMEOUT);
      AssertTrue(GetMonotonicClockTime() - start >= 190);
      AssertTrue(client.isConnected());

      // Client-initiated close handshake
      server.closeReceived = false;
      client.close(WEBSOCKET_CLOSE_NORMAL, "done");
      AssertFalse(client.isConnected());
      AssertTrue(WaitForFlag(&server.closeReceived, 2000));
      AssertEquals(server.clientCloseCode, static_cast<uint16_t>(WEBSOCKET_CLOSE_NORMAL));
      AssertEquals(client.getCloseCode(), static_cast<uint16_t>(WEBSOCKET_CLOSE_NORMAL));
   }
   EndTest();

   StartTest(_T("WebSocket fragmented message with interleaved control frame"));
   {
      WebSocketClient client;
      AssertTrue(client.connect(url, 5000));
      ByteStream message;
      WebSocketMessageType type;
      AssertTrue(client.sendText("FRAG"));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::MESSAGE);
      AssertTrue(type == WebSocketMessageType::TEXT);
      AssertEquals(message.size(), static_cast<size_t>(15));
      AssertTrue(!memcmp(message.buffer(), "part1part2part3", 15));
      client.close();
   }
   EndTest();

   StartTest(_T("WebSocket server-initiated close"));
   server.closeReceived = false;
   {
      WebSocketClient client;
      AssertTrue(client.connect(url, 5000));
      ByteStream message;
      WebSocketMessageType type;
      AssertTrue(client.sendText("BYE"));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::CLOSED);
      AssertFalse(client.isConnected());
      AssertEquals(client.getCloseCode(), static_cast<uint16_t>(WEBSOCKET_CLOSE_GOING_AWAY));
      AssertEquals(client.getCloseReason(), "going away");
      AssertTrue(WaitForFlag(&server.closeReceived, 2000));
      AssertEquals(server.clientCloseCode, static_cast<uint16_t>(WEBSOCKET_CLOSE_GOING_AWAY));
   }
   EndTest();

   StartTest(_T("WebSocket message size limit"));
   server.closeReceived = false;
   {
      WebSocketClient client;
      client.setMaxMessageSize(1000);
      AssertTrue(client.connect(url, 5000));
      ByteStream message;
      WebSocketMessageType type;
      AssertTrue(client.sendText("BIG"));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::FAILURE);
      AssertFalse(client.isConnected());
      AssertTrue(WaitForFlag(&server.closeReceived, 2000));
      AssertEquals(server.clientCloseCode, static_cast<uint16_t>(WEBSOCKET_CLOSE_MESSAGE_TOO_BIG));
   }
   EndTest();

   StartTest(_T("WebSocket protocol violation by server"));
   server.closeReceived = false;
   {
      WebSocketClient client;
      AssertTrue(client.connect(url, 5000));
      ByteStream message;
      WebSocketMessageType type;
      AssertTrue(client.sendText("MASK"));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::FAILURE);
      AssertFalse(client.isConnected());
      AssertTrue(WaitForFlag(&server.closeReceived, 2000));
      AssertEquals(server.clientCloseCode, static_cast<uint16_t>(WEBSOCKET_CLOSE_PROTOCOL_ERROR));
   }
   EndTest();

   StartTest(_T("WebSocket disconnect from another thread"));
   {
      WebSocketClient client;
      AssertTrue(client.connect(url, 5000));
      DisconnectContext context = { &client, 300 };
      THREAD thread = ThreadCreateEx(DisconnectThread, 0, &context);
      ByteStream message;
      WebSocketMessageType type;
      int64_t start = GetMonotonicClockTime();
      WebSocketReadResult rc = client.readMessage(&message, &type, 10000);
      AssertTrue(rc == WebSocketReadResult::FAILURE);
      AssertTrue(GetMonotonicClockTime() - start < 3000);
      AssertFalse(client.isConnected());
      ThreadJoin(thread);
   }
   EndTest();

   StartTest(_T("WebSocket client reconnect"));
   {
      WebSocketClient client;
      AssertTrue(client.connect(url, 5000));
      client.close();
      AssertFalse(client.isConnected());
      AssertTrue(client.connect(url, 5000));
      ByteStream message;
      WebSocketMessageType type;
      AssertTrue(client.sendText("again"));
      AssertTrue(client.readMessage(&message, &type, 5000) == WebSocketReadResult::MESSAGE);
      AssertEquals(message.size(), static_cast<size_t>(5));
      client.close();
   }
   EndTest();

   StopTestServer(&server);
}
