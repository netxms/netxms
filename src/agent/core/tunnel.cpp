/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: tunnel.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("tunnel")

/**
 * Check if server address is valid
 */
bool IsValidServerAddress(const InetAddress &addr, bool *pbMasterServer, bool *pbControlServer);

/**
 * Register session
 */
bool RegisterSession(CommSession *session);

class Tunnel;

/**
 * Unique channel ID
 */
static VolatileCounter s_nextChannelId = 0;

/**
 * Tunnel communication channel
 */
class TunnelCommChannel : public AbstractCommChannel
{
private:
   Tunnel *m_tunnel;
   UINT32 m_id;
   bool m_active;
   VolatileCounter m_closed;
   RingBuffer m_buffer;
#ifdef _WIN32
   CRITICAL_SECTION m_bufferLock;
   HANDLE m_dataCondition;
#else
   pthread_mutex_t m_bufferLock;
   pthread_cond_t m_dataCondition;
#endif

protected:
   virtual ~TunnelCommChannel();

public:
   TunnelCommChannel(Tunnel *tunnel);

   virtual int send(const void *data, size_t size, MUTEX mutex = INVALID_MUTEX_HANDLE);
   virtual int recv(void *buffer, size_t size, UINT32 timeout = INFINITE);
   virtual int poll(UINT32 timeout, bool write = false);
   virtual int shutdown();
   virtual void close();

   UINT32 getId() const { return m_id; }

   void putData(const BYTE *data, size_t size);
};

/**
 * Tunnel class
 */
class Tunnel
{
private:
   TCHAR *m_hostname;
   UINT16 m_port;
   InetAddress m_address;
   SOCKET m_socket;
   SSL_CTX *m_context;
   SSL *m_ssl;
   MUTEX m_sslLock;
   bool m_connected;
   bool m_reset;
   VolatileCounter m_requestId;
   THREAD m_recvThread;
   MsgWaitQueue *m_queue;
   RefCountHashMap<UINT32, TunnelCommChannel> m_channels;
   MUTEX m_channelLock;

   Tunnel(const TCHAR *hostname, UINT16 port);

   bool connectToServer();
   int sslWrite(const void *data, size_t size);
   bool sendMessage(const NXCPMessage *msg);
   NXCPMessage *waitForMessage(UINT16 code, UINT32 id) { return (m_queue != NULL) ? m_queue->waitForMessage(code, id, 5000) : NULL; }

   void processBindRequest(NXCPMessage *request);
   void processChannelCloseRequest(NXCPMessage *request);
   void createSession(NXCPMessage *request);

   X509_REQ *createCertificateRequest(const char *country, const char *org, const char *cn, EVP_PKEY **pkey);
   bool saveCertificate(X509 *cert, EVP_PKEY *key);
   void loadCertificate();

   void recvThread();
   static THREAD_RESULT THREAD_CALL recvThreadStarter(void *arg);

public:
   ~Tunnel();

   void checkConnection();
   void disconnect();

   TunnelCommChannel *createChannel();
   void closeChannel(TunnelCommChannel *channel);
   int sendChannelData(UINT32 id, const void *data, size_t len);

   const TCHAR *getHostname() const { return m_hostname; }

   void debugPrintf(int level, const TCHAR *format, ...);

   static Tunnel *createFromConfig(TCHAR *config);
};

/**
 * Tunnel constructor
 */
Tunnel::Tunnel(const TCHAR *hostname, UINT16 port) : m_channels(true)
{
   m_hostname = _tcsdup(hostname);
   m_port = port;
   m_socket = INVALID_SOCKET;
   m_context = NULL;
   m_ssl = NULL;
   m_sslLock = MutexCreate();
   m_connected = false;
   m_reset = false;
   m_requestId = 0;
   m_recvThread = INVALID_THREAD_HANDLE;
   m_queue = NULL;
   m_channelLock = MutexCreate();
}

/**
 * Tunnel destructor
 */
Tunnel::~Tunnel()
{
   disconnect();
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
   if (m_ssl != NULL)
      SSL_free(m_ssl);
   if (m_context != NULL)
      SSL_CTX_free(m_context);
   MutexDestroy(m_sslLock);
   MutexDestroy(m_channelLock);
   free(m_hostname);
}

/**
 * Debug output
 */
void Tunnel::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   TCHAR buffer[8192];
   _vsntprintf(buffer, 8192, format, args);
   va_end(args);
   nxlog_debug_tag(DEBUG_TAG, level, _T("%s: %s"), m_hostname, buffer);
}

/**
 * Force disconnect
 */
void Tunnel::disconnect()
{
   if (m_socket != INVALID_SOCKET)
      shutdown(m_socket, SHUT_RDWR);
   m_connected = false;
   ThreadJoin(m_recvThread);
   delete_and_null(m_queue);

   Array channels(g_dwMaxSessions, 16, false);
   MutexLock(m_channelLock);
   Iterator<TunnelCommChannel> *it = m_channels.iterator();
   while(it->hasNext())
   {
      TunnelCommChannel *c = it->next();
      channels.add(c);
      c->incRefCount();
   }
   delete it;
   MutexUnlock(m_channelLock);

   for(int i = 0; i < channels.size(); i++)
   {
      AbstractCommChannel *c = (AbstractCommChannel *)channels.get(i);
      c->close();
      c->decRefCount();
   }
}

/**
 * Receiver thread starter
 */
THREAD_RESULT THREAD_CALL Tunnel::recvThreadStarter(void *arg)
{
   ((Tunnel *)arg)->recvThread();
   return THREAD_OK;
}

/**
 * Receiver thread
 */
void Tunnel::recvThread()
{
   TlsMessageReceiver receiver(m_socket, m_ssl, m_sslLock, 8192, MAX_AGENT_MSG_SIZE);
   while(m_connected)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(1000, &result);
      if (msg != NULL)
      {
         if (nxlog_get_debug_level() >= 6)
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
         }

         if (msg->getCode() == CMD_RESET_TUNNEL)
         {
            m_reset = true;
            debugPrintf(4, _T("Receiver thread stopped (tunnel reset)"));
            break;
         }

         switch(msg->getCode())
         {
            case CMD_BIND_AGENT_TUNNEL:
               ThreadPoolExecute(g_commThreadPool, this, &Tunnel::processBindRequest, msg);
               msg = NULL; // prevent message deletion
               break;
            case CMD_CREATE_CHANNEL:
               createSession(msg);
               break;
            case CMD_CHANNEL_DATA:
               if (msg->isBinary())
               {
                  MutexLock(m_channelLock);
                  TunnelCommChannel *channel = m_channels.get(msg->getId());
                  MutexUnlock(m_channelLock);
                  if (channel != NULL)
                  {
                     channel->putData(msg->getBinaryData(), msg->getBinaryDataSize());
                     channel->decRefCount();
                  }
               }
               break;
            case CMD_CLOSE_CHANNEL:
               processChannelCloseRequest(msg);
               break;
            default:
               m_queue->put(msg);
               msg = NULL; // prevent message deletion
               break;
         }
         delete msg;
      }
      else if (result != MSGRECV_TIMEOUT)
      {
         debugPrintf(4, _T("Receiver thread stopped (%s)"), AbstractMessageReceiver::resultToText(result));
         m_reset = true;
         break;
      }
   }
}

/**
 * Write to SSL
 */
int Tunnel::sslWrite(const void *data, size_t size)
{
   if (!m_connected || m_reset)
      return -1;

   bool canRetry;
   int bytes;
   MutexLock(m_sslLock);
   do
   {
      canRetry = false;
      bytes = SSL_write(m_ssl, data, (int)size);
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, bytes);
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(5000) > 0)
               canRetry = true;
         }
         else
         {
            debugPrintf(7, _T("SSL_write error (bytes=%d ssl_err=%d errno=%d)"), bytes, err, errno);
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
         }
      }
   }
   while(canRetry);
   MutexUnlock(m_sslLock);
   return bytes;
}

/**
 * Send message
 */
bool Tunnel::sendMessage(const NXCPMessage *msg)
{
   if (!m_connected || m_reset)
      return false;

   if (nxlog_get_debug_level() >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
   }
   NXCP_MESSAGE *data = msg->serialize(false);
   bool success = (sslWrite(data, ntohl(data->size)) == ntohl(data->size));
   free(data);
   return success;
}

/**
 * Load certificate for this tunnel
 */
void Tunnel::loadCertificate()
{
   BYTE addressHash[SHA1_DIGEST_SIZE];
#ifdef UNICODE
   char *un = MBStringFromWideString(m_hostname);
   CalculateSHA1Hash((BYTE *)un, strlen(un), addressHash);
   free(un);
#else
   CalculateSHA1Hash((BYTE *)m_hostname, strlen(m_hostname), addressHash);
#endif

   TCHAR prefix[48];
   BinToStr(addressHash, SHA1_DIGEST_SIZE, prefix);

   TCHAR name[MAX_PATH];
   _sntprintf(name, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);
   FILE *f = _tfopen(name, _T("r"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      if (errno == ENOENT)
      {
         // Try fallback file
         m_address.buildHashKey(addressHash);
         BinToStr(addressHash, 18, prefix);
         _sntprintf(name, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);

         f = _tfopen(name, _T("r"));
         if (f == NULL)
         {
            debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
            return;
         }
      }
      else
      {
         return;
      }
   }

   X509 *cert = PEM_read_X509(f, NULL, NULL, NULL);
   fclose(f);

   if (cert == NULL)
   {
      debugPrintf(4, _T("Cannot load certificate from file \"%s\""), name);
      return;
   }

   _sntprintf(name, MAX_PATH, _T("%s%s.key"), g_certificateDirectory, prefix);
   f = _tfopen(name, _T("r"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      X509_free(cert);
      return;
   }

   EVP_PKEY *key = PEM_read_PrivateKey(f, NULL, NULL, (void *)"nxagentd");
   fclose(f);

   if (key == NULL)
   {
      debugPrintf(4, _T("Cannot load private key from file \"%s\""), name);
      X509_free(cert);
      return;
   }

   if (SSL_CTX_use_certificate(m_context, cert) == 1)
   {
      if (SSL_CTX_use_PrivateKey(m_context, key) == 1)
      {
         debugPrintf(4, _T("Certificate and private key loaded"));
      }
      else
      {
         debugPrintf(4, _T("Cannot set private key"));
      }
   }
   else
   {
      debugPrintf(4, _T("Cannot set certificate"));
   }

   X509_free(cert);
   EVP_PKEY_free(key);
}

/**
 * Connect to server
 */
bool Tunnel::connectToServer()
{
   // Cleanup from previous connection attempt
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
   if (m_ssl != NULL)
      SSL_free(m_ssl);
   if (m_context != NULL)
      SSL_CTX_free(m_context);

   m_socket = INVALID_SOCKET;
   m_context = NULL;
   m_ssl = NULL;

   m_address = InetAddress::resolveHostName(m_hostname);
   if (!m_address.isValidUnicast())
   {
      debugPrintf(4, _T("Server address cannot be resolved or is not valid"));
      return false;
   }

   // Create socket and connect
   m_socket = socket(m_address.getFamily(), SOCK_STREAM, 0);
   if (m_socket == INVALID_SOCKET)
   {
      debugPrintf(4, _T("Cannot create socket (%s)"), _tcserror(WSAGetLastError()));
      return false;
   }

   SockAddrBuffer sa;
   m_address.fillSockAddr(&sa, m_port);
   if (ConnectEx(m_socket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa), 5000) == -1)
   {
      debugPrintf(4, _T("Cannot establish connection (%s)"), _tcserror(WSAGetLastError()));
      return false;
   }

   // Setup secure connection
   const SSL_METHOD *method = SSLv23_method();
   if (method == NULL)
   {
      debugPrintf(4, _T("Cannot obtain TLS method"));
      return false;
   }

   m_context = SSL_CTX_new((SSL_METHOD *)method);
   if (m_context == NULL)
   {
      debugPrintf(4, _T("Cannot create TLS context"));
      return false;
   }
#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif
   loadCertificate();

   m_ssl = SSL_new(m_context);
   if (m_ssl == NULL)
   {
      debugPrintf(4, _T("Cannot create SSL object"));
      return false;
   }

   SSL_set_connect_state(m_ssl);
   SSL_set_fd(m_ssl, (int)m_socket);

   while(true)
   {
      int rc = SSL_do_handshake(m_ssl);
      if (rc != 1)
      {
         int sslErr = SSL_get_error(m_ssl, rc);
         if (sslErr == SSL_ERROR_WANT_READ)
         {
            SocketPoller poller;
            poller.add(m_socket);
            if (poller.poll(5000) > 0)
               continue;
            debugPrintf(4, _T("TLS handshake failed (timeout)"));
            return false;
         }
         else
         {
            char buffer[128];
            debugPrintf(4, _T("TLS handshake failed (%hs)"), ERR_error_string(sslErr, buffer));
            return false;
         }
      }
      break;
   }

   // Check server certificate
   X509 *cert = SSL_get_peer_certificate(m_ssl);
   if (cert == NULL)
   {
      debugPrintf(4, _T("Server certificate not provided"));
      return false;
   }

   char *subj = X509_NAME_oneline(X509_get_subject_name(cert), NULL ,0);
   char *issuer = X509_NAME_oneline(X509_get_issuer_name(cert), NULL ,0);
   debugPrintf(4, _T("Server certificate subject is %hs"), subj);
   debugPrintf(4, _T("Server certificate issuer is %hs"), issuer);
   OPENSSL_free(subj);
   OPENSSL_free(issuer);

   X509_free(cert);

   // Setup receiver
   delete m_queue;
   m_queue = new MsgWaitQueue();
   m_connected = true;
   m_recvThread = ThreadCreateEx(Tunnel::recvThreadStarter, 0, this);

   m_requestId = 0;

   // Do handshake
   NXCPMessage msg;
   msg.setCode(CMD_SETUP_AGENT_TUNNEL);
   msg.setId(InterlockedIncrement(&m_requestId));
   msg.setField(VID_AGENT_VERSION, NETXMS_BUILD_TAG);
   msg.setField(VID_SYS_NAME, g_systemName);
   msg.setField(VID_ZONE_UIN, g_zoneUIN);

   TCHAR fqdn[256];
   if (GetLocalHostName(fqdn, 256, true))
      msg.setField(VID_HOSTNAME, fqdn);

   VirtualSession session(0);
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (GetParameterValue(_T("System.PlatformName"), buffer, &session) == ERR_SUCCESS)
      msg.setField(VID_PLATFORM_NAME, buffer);
   if (GetParameterValue(_T("System.UName"), buffer, &session) == ERR_SUCCESS)
      msg.setField(VID_SYS_DESCRIPTION, buffer);

   sendMessage(&msg);

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response == NULL)
   {
      debugPrintf(4, _T("Cannot configure tunnel (request timeout)"));
      disconnect();
      return false;
   }

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc != ERR_SUCCESS)
   {
      debugPrintf(4, _T("Cannot configure tunnel (error %d)"), rcc);
      disconnect();
      return false;
   }

   return true;
}

/**
 * Check tunnel connection and connect as needed
 */
void Tunnel::checkConnection()
{
   if (m_reset)
   {
      m_reset = false;
      disconnect();
      closesocket(m_socket);
      m_socket = INVALID_SOCKET;
      debugPrintf(3, _T("Resetting tunnel"));
      if (connectToServer())
         debugPrintf(3, _T("Tunnel is active"));
   }
   else if (!m_connected)
   {
      if (connectToServer())
         debugPrintf(3, _T("Tunnel is active"));
   }
   else
   {
      NXCPMessage msg;
      msg.setCode(CMD_KEEPALIVE);
      msg.setId(InterlockedIncrement(&m_requestId));
      if (sendMessage(&msg))
      {
         NXCPMessage *response = waitForMessage(CMD_KEEPALIVE, msg.getId());
         if (response == NULL)
         {
            disconnect();
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            debugPrintf(3, _T("Connection test failed"));
         }
         else
         {
            delete response;
         }
      }
      else
      {
         disconnect();
         closesocket(m_socket);
         m_socket = INVALID_SOCKET;
         debugPrintf(3, _T("Connection test failed"));
      }
   }
}

/**
 * Create certificate request
 */
X509_REQ *Tunnel::createCertificateRequest(const char *country, const char *org, const char *cn, EVP_PKEY **pkey)
{
   RSA *key = RSA_new();
   if (key == NULL)
   {
      debugPrintf(4, _T("call to RSA_new() failed"));
      return NULL;
   }

   BIGNUM *bn = BN_new();
   if (bn == NULL)
   {
      debugPrintf(4, _T("call to BN_new() failed"));
      RSA_free(key);
      return NULL;
   }

   BN_set_word(bn, RSA_F4);
   if (RSA_generate_key_ex(key, NETXMS_RSA_KEYLEN, bn, NULL) == -1)
   {
      debugPrintf(4, _T("call to RSA_generate_key_ex() failed"));
      RSA_free(key);
      BN_free(bn);
      return NULL;
   }
   BN_free(bn);

   X509_REQ *req = X509_REQ_new();
   if (req != NULL)
   {
      X509_REQ_set_version(req, 1);
      X509_NAME *subject = X509_REQ_get_subject_name(req);
      if (subject != NULL)
      {
         if (country != NULL)
            X509_NAME_add_entry_by_txt(subject, "C", MBSTRING_UTF8, (const BYTE *)country, -1, -1, 0);
         X509_NAME_add_entry_by_txt(subject, "O", MBSTRING_UTF8, (const BYTE *)((org != NULL) ? org : "netxms.org"), -1, -1, 0);
         X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_UTF8, (const BYTE *)cn, -1, -1, 0);

         EVP_PKEY *ekey = EVP_PKEY_new();
         if (ekey != NULL)
         {
            EVP_PKEY_assign_RSA(ekey, key);
            key = NULL; // will be freed by EVP_PKEY_free
            X509_REQ_set_pubkey(req, ekey);
            if (X509_REQ_sign(req, ekey, EVP_sha256()) > 0)
            {
               *pkey = ekey;
            }
            else
            {
               debugPrintf(4, _T("call to X509_REQ_sign() failed"));
               X509_REQ_free(req);
               req = NULL;
               EVP_PKEY_free(ekey);
            }
         }
         else
         {
            debugPrintf(4, _T("call to EVP_PKEY_new() failed"));
            X509_REQ_free(req);
            req = NULL;
         }
      }
      else
      {
         debugPrintf(4, _T("call to X509_REQ_get_subject_name() failed"));
         X509_REQ_free(req);
         req = NULL;
      }
   }
   else
   {
      debugPrintf(4, _T("call to X509_REQ_new() failed"));
   }

   if (key != NULL)
      RSA_free(key);
   return req;
}

/**
 * Save certificate
 */
bool Tunnel::saveCertificate(X509 *cert, EVP_PKEY *key)
{
   BYTE addressHash[SHA1_DIGEST_SIZE];
#ifdef UNICODE
   char *un = MBStringFromWideString(m_hostname);
   CalculateSHA1Hash((BYTE *)un, strlen(un), addressHash);
   free(un);
#else
   CalculateSHA1Hash((BYTE *)m_hostname, strlen(m_hostname), addressHash);
#endif

   TCHAR prefix[48];
   BinToStr(addressHash, SHA1_DIGEST_SIZE, prefix);

   TCHAR name[MAX_PATH];
   _sntprintf(name, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);
   FILE *f = _tfopen(name, _T("w"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      return false;
   }
   int rc = PEM_write_X509(f, cert);
   fclose(f);
   if (rc != 1)
   {
      debugPrintf(4, _T("PEM_write_X509(\"%s\") failed"), name);
      return false;
   }

   _sntprintf(name, MAX_PATH, _T("%s%s.key"), g_certificateDirectory, prefix);
   f = _tfopen(name, _T("w"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      return false;
   }
   rc = PEM_write_PrivateKey(f, key, EVP_des_ede3_cbc(), NULL, 0, 0, (void *)"nxagentd");
   fclose(f);
   if (rc != 1)
   {
      debugPrintf(4, _T("PEM_write_PrivateKey(\"%s\") failed"), name);
      return false;
   }

   debugPrintf(4, _T("Certificate and private key saved"));
   return true;
}

/**
 * Process tunnel bind request
 */
void Tunnel::processBindRequest(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   uuid guid = request->getFieldAsGUID(VID_GUID);
   char *cn = guid.toString().getUTF8String();

   char *country = request->getFieldAsUtf8String(VID_COUNTRY);
   char *org = request->getFieldAsUtf8String(VID_ORGANIZATION);

   EVP_PKEY *key = NULL;
   X509_REQ *req = createCertificateRequest(country, org, cn, &key);
   free(cn);

   if (req != NULL)
   {
      BYTE *buffer = NULL;
      int len = i2d_X509_REQ(req, &buffer);
      if (len > 0)
      {
         NXCPMessage certRequest(CMD_REQUEST_CERTIFICATE, request->getId());
         certRequest.setField(VID_CERTIFICATE, buffer, len);
         sendMessage(&certRequest);
         OPENSSL_free(buffer);

         NXCPMessage *certResponse = waitForMessage(CMD_NEW_CERTIFICATE, request->getId());
         if (certResponse != NULL)
         {
            UINT32 rcc = certResponse->getFieldAsUInt32(VID_RCC);
            if (rcc == ERR_SUCCESS)
            {
               size_t certLen;
               const BYTE *certData = certResponse->getBinaryFieldPtr(VID_CERTIFICATE, &certLen);
               if (certData != NULL)
               {
                  X509 *cert = d2i_X509(NULL, &certData, (long)certLen);
                  if (cert != NULL)
                  {
                     if (saveCertificate(cert, key))
                     {
                        response.setField(VID_RCC, ERR_SUCCESS);
                     }
                     else
                     {
                        response.setField(VID_RCC, ERR_IO_FAILURE);
                     }
                     X509_free(cert);
                  }
                  else
                  {
                     debugPrintf(4, _T("certificate data is invalid"));
                     response.setField(VID_RCC, ERR_ENCRYPTION_ERROR);
                  }
               }
               else
               {
                  debugPrintf(4, _T("certificate data missing in server response"));
                  response.setField(VID_RCC, ERR_INTERNAL_ERROR);
               }
            }
            else
            {
               debugPrintf(4, _T("certificate request failed (%d)"), rcc);
               response.setField(VID_RCC, rcc);
            }
            delete certResponse;
         }
         else
         {
            debugPrintf(4, _T("timeout waiting for certificate request completion"));
            response.setField(VID_RCC, ERR_REQUEST_TIMEOUT);
         }
      }
      else
      {
         debugPrintf(4, _T("call to i2d_X509_REQ() failed"));
         response.setField(VID_RCC, ERR_ENCRYPTION_ERROR);
      }
      X509_REQ_free(req);
      EVP_PKEY_free(key);
   }
   else
   {
      response.setField(VID_RCC, ERR_ENCRYPTION_ERROR);
   }

   sendMessage(&response);
   delete request;
}

/**
 * Create new session
 */
void Tunnel::createSession(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   // Assume that peer always have minimal access, so don't check return value
   bool masterServer, controlServer;
   IsValidServerAddress(m_address, &masterServer, &controlServer);

   TunnelCommChannel *channel = createChannel();
   if (channel != NULL)
   {
      CommSession *session = new CommSession(channel, m_address, masterServer, controlServer);
      if (RegisterSession(session))
      {
         response.setField(VID_RCC, ERR_SUCCESS);
         response.setField(VID_CHANNEL_ID, channel->getId());
         debugPrintf(9, _T("New session registered"));
         session->run();
      }
      else
      {
         delete session;
         response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
      }
      channel->decRefCount();
   }
   else
   {
      response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
   }

   sendMessage(&response);
}

/**
 * Create channel
 */
TunnelCommChannel *Tunnel::createChannel()
{
   TunnelCommChannel *channel = NULL;
   MutexLock(m_channelLock);
   if (m_channels.size() < (int)g_dwMaxSessions)
   {
      channel = new TunnelCommChannel(this);
      m_channels.set(channel->getId(), channel);
      debugPrintf(5, _T("New channel created (ID=%d)"), channel->getId());
   }
   MutexUnlock(m_channelLock);
   return channel;
}

/**
 * Process server's channel close request
 */
void Tunnel::processChannelCloseRequest(NXCPMessage *request)
{
   UINT32 id = request->getFieldAsUInt32(VID_CHANNEL_ID);
   debugPrintf(5, _T("Close request for channel %d"), id);
   MutexLock(m_channelLock);
   TunnelCommChannel *channel = m_channels.get(id);
   MutexUnlock(m_channelLock);
   if (channel != NULL)
   {
      channel->close();
      channel->decRefCount();
   }
}

/**
 * Close channel
 */
void Tunnel::closeChannel(TunnelCommChannel *channel)
{
   UINT32 id = 0;
   MutexLock(m_channelLock);
   if (m_channels.contains(channel->getId()))
   {
      id = channel->getId();
      debugPrintf(5, _T("Channel %d closed"), id);
      m_channels.remove(id);
   }
   MutexUnlock(m_channelLock);

   if (id != 0)
   {
      NXCPMessage msg(CMD_CLOSE_CHANNEL, 0);
      msg.setField(VID_CHANNEL_ID, id);
      sendMessage(&msg);
   }
}

/**
 * Send channel data
 */
int Tunnel::sendChannelData(UINT32 id, const void *data, size_t len)
{
   NXCP_MESSAGE *msg = CreateRawNXCPMessage(CMD_CHANNEL_DATA, id, 0, data, len, NULL, false);
   int rc = sslWrite(msg, ntohl(msg->size));
   if (rc == ntohl(msg->size))
      rc = (int)len;  // adjust number of bytes to exclude tunnel overhead
   free(msg);
   return rc;
}

/**
 * Create tunnel object from configuration record
 */
Tunnel *Tunnel::createFromConfig(TCHAR *config)
{
   int port = AGENT_TUNNEL_PORT;
   TCHAR *p = _tcschr(config, _T(':'));
   if (p != NULL)
   {
      *p = 0;
      p++;

      TCHAR *eptr;
      int port = _tcstol(p, &eptr, 10);
      if ((port < 1) || (port > 65535))
         return NULL;
   }
   return new Tunnel(config, port);
}

/**
 * Channel constructor
 */
TunnelCommChannel::TunnelCommChannel(Tunnel *tunnel) : AbstractCommChannel(), m_buffer(32768, 32768)
{
   m_id = InterlockedIncrement(&s_nextChannelId);
   m_tunnel = tunnel;
   m_active = true;
   m_closed = 0;
#ifdef _WIN32
   InitializeCriticalSectionAndSpinCount(&m_bufferLock, 4000);
   m_dataCondition = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
   pthread_mutex_init(&m_bufferLock, NULL);
   pthread_cond_init(&m_dataCondition, NULL);
#endif
}

/**
 * Channel destructor
 */
TunnelCommChannel::~TunnelCommChannel()
{
#ifdef _WIN32
   DeleteCriticalSection(&m_bufferLock);
   CloseHandle(m_dataCondition);
#else
   pthread_mutex_destroy(&m_bufferLock);
   pthread_cond_destroy(&m_dataCondition);
#endif
}

/**
 * Send data
 */
int TunnelCommChannel::send(const void *data, size_t size, MUTEX mutex)
{
   return m_active ? m_tunnel->sendChannelData(m_id, data, size) : -1;
}

/**
 * Receive data
 */
int TunnelCommChannel::recv(void *buffer, size_t size, UINT32 timeout)
{
   if (!m_active)
      return 0;

#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
retry_wait:
      LeaveCriticalSection(&m_bufferLock);
      if (WaitForSingleObject(m_dataCondition, timeout) == WAIT_TIMEOUT)
         return -2;

      if (!m_active)
         return 0;   // closed while waiting

      EnterCriticalSection(&m_bufferLock);
      if (m_buffer.isEmpty())
      {
         ResetEvent(m_dataCondition);
         goto retry_wait;
      }
   }
#else
   pthread_mutex_lock(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      int rc = pthread_cond_reltimedwait_np(&m_dataCondition, &m_bufferLock, &ts);
#else
      struct timeval now;
      struct timespec ts;
      gettimeofday(&now, NULL);
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      now.tv_usec += (timeout % 1000) * 1000;
      ts.tv_sec += now.tv_usec / 1000000;
      ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
      int rc = pthread_cond_timedwait(&m_dataCondition, &m_bufferLock, &ts);
#endif
      if (rc != 0)
      {
         pthread_mutex_unlock(&m_bufferLock);
         return -2;  // timeout
      }

      if (!m_active) // closed while waiting
      {
         pthread_mutex_unlock(&m_bufferLock);
         return 0;
      }
   }
#endif

   size_t bytes = m_buffer.read((BYTE *)buffer, size);
#ifdef _WIN32
   if (m_buffer.isEmpty())
      ResetEvent(m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_unlock(&m_bufferLock);
#endif
   return (int)bytes;
}

/**
 * Poll for data
 */
int TunnelCommChannel::poll(UINT32 timeout, bool write)
{
   if (write)
      return 1;

   if (!m_active)
      return -1;

   int rc = 0;

#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
retry_wait:
      LeaveCriticalSection(&m_bufferLock);
      if (WaitForSingleObject(m_dataCondition, timeout) == WAIT_TIMEOUT)
         return 0;

      if (!m_active)
         return -1;

      EnterCriticalSection(&m_bufferLock);
      if (m_buffer.isEmpty())
      {
         ResetEvent(m_dataCondition);
         goto retry_wait;
      }
   }
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      rc = pthread_cond_reltimedwait_np(&m_dataCondition, &m_bufferLock, &ts);
#else
      struct timeval now;
      struct timespec ts;
      gettimeofday(&now, NULL);
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      now.tv_usec += (timeout % 1000) * 1000;
      ts.tv_sec += now.tv_usec / 1000000;
      ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
      rc = pthread_cond_timedwait(&m_dataCondition, &m_bufferLock, &ts);
#endif
   }
   pthread_mutex_unlock(&m_bufferLock);
#endif

   return (rc == 0) ? 1 : 0;
}

/**
 * Shutdown channel
 */
int TunnelCommChannel::shutdown()
{
   m_active = false;
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   SetEvent(m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
#endif
   return 0;
}

/**
 * Close channel
 */
void TunnelCommChannel::close()
{
   if (InterlockedIncrement(&m_closed) > 1)
      return;  // already closed or close in progress
   shutdown();
   m_tunnel->closeChannel(this);
}

/**
 * Put data into buffer
 */
void TunnelCommChannel::putData(const BYTE *data, size_t size)
{
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
#endif
   m_buffer.write(data, size);
#ifdef _WIN32
   SetEvent(m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
   pthread_mutex_unlock(&m_bufferLock);
#endif
}

/**
 * Configured tunnels
 */
static ObjectArray<Tunnel> s_tunnels;

/**
 * Parser server connection (tunnel) list
 */
void ParseTunnelList(TCHAR *list)
{
   TCHAR *curr, *next;
   for(curr = next = list; curr != NULL && *curr != 0; curr = next + 1)
   {
      next = _tcschr(curr, _T('\n'));
      if (next != NULL)
         *next = 0;
      StrStrip(curr);

      Tunnel *t = Tunnel::createFromConfig(curr);
      if (t != NULL)
      {
         s_tunnels.add(t);
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Added server tunnel %s"), t->getHostname());
      }
      else
      {
         nxlog_write(MSG_INVALID_TUNNEL_CONFIG, NXLOG_ERROR, "s", curr);
      }
   }
   free(list);
}

/**
 * Tunnel manager
 */
THREAD_RESULT THREAD_CALL TunnelManager(void *)
{
   if (s_tunnels.size() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("No tunnels configured, tunnel manager will not start"));
      return THREAD_OK;
   }

   g_tunnelKeepaliveInterval *= 1000;  // convert to milliseconds
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Tunnel manager started"));
   do
   {
      for(int i = 0; i < s_tunnels.size(); i++)
      {
         Tunnel *t = s_tunnels.get(i);
         t->checkConnection();
      }
   }
   while(!AgentSleepAndCheckForShutdown(g_tunnelKeepaliveInterval));
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Tunnel manager stopped"));
   return THREAD_OK;
}
