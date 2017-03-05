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

/**
 * Tunnel class
 */
class Tunnel
{
private:
   InetAddress m_address;
   UINT16 m_port;
   SOCKET m_socket;
   SSL_CTX *m_context;
   SSL *m_ssl;
   MUTEX m_sslLock;
   bool m_connected;
   VolatileCounter m_requestId;
   THREAD m_recvThread;
   MsgWaitQueue *m_queue;
   TCHAR m_debugId[64];

   Tunnel(const InetAddress& addr, UINT16 port);

   bool connectToServer();
   bool sendMessage(const NXCPMessage *msg);
   NXCPMessage *waitForMessage(UINT16 code, UINT32 id) { return (m_queue != NULL) ? m_queue->waitForMessage(code, id, 5000) : NULL; }

   void processBindRequest(NXCPMessage *request);

   X509_REQ *createCertificateRequest(const char *cn, EVP_PKEY **pkey);
   bool saveCertificate(X509 *cert, EVP_PKEY *key);
   void loadCertificate();

   void debugPrintf(int level, const TCHAR *format, ...);

   void recvThread();
   static THREAD_RESULT THREAD_CALL recvThreadStarter(void *arg);

public:
   ~Tunnel();

   void checkConnection();
   void disconnect();

   const InetAddress& getAddress() const { return m_address; }

   static Tunnel *createFromConfig(TCHAR *config);
};

/**
 * Tunnel constructor
 */
Tunnel::Tunnel(const InetAddress& addr, UINT16 port) : m_address(addr)
{
   m_port = port;
   m_socket = INVALID_SOCKET;
   m_context = NULL;
   m_ssl = NULL;
   m_sslLock = MutexCreate();
   m_connected = false;
   m_requestId = 0;
   m_recvThread = INVALID_THREAD_HANDLE;
   m_queue = NULL;
   _sntprintf(m_debugId, 64, _T("TUN-%s"), (const TCHAR *)addr.toString());
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
   nxlog_debug(level, _T("[%s] %s"), m_debugId, buffer);
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
         switch(msg->getCode())
         {
            case CMD_BIND_AGENT_TUNNEL:
               ThreadPoolExecute(g_commThreadPool, this, &Tunnel::processBindRequest, msg);
               msg = NULL; // prevent message deletion
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
         break;
      }
   }
}

/**
 * Send message
 */
bool Tunnel::sendMessage(const NXCPMessage *msg)
{
   if (m_socket == INVALID_SOCKET)
      return false;

   if (nxlog_get_debug_level() >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
   }
   NXCP_MESSAGE *data = msg->createMessage(true);
   MutexLock(m_sslLock);
   bool success = (SSL_write(m_ssl, data, ntohl(data->size)) == ntohl(data->size));
   MutexUnlock(m_sslLock);
   free(data);
   return success;
}

/**
 * Load certificate for this tunnel
 */
void Tunnel::loadCertificate()
{
   BYTE addressHash[18];
   m_address.buildHashKey(addressHash);

   TCHAR prefix[48];
   BinToStr(addressHash, 18, prefix);

   TCHAR name[MAX_PATH];
   _sntprintf(name, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);
   FILE *f = _tfopen(name, _T("r"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      return;
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

   m_context = SSL_CTX_new(method);
   if (m_context == NULL)
   {
      debugPrintf(4, _T("Cannot create TLS context"));
      return false;
   }
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
   loadCertificate();

   m_ssl = SSL_new(m_context);
   if (m_ssl == NULL)
   {
      debugPrintf(4, _T("Cannot create SSL object"));
      return false;
   }

   SSL_set_connect_state(m_ssl);
   SSL_set_fd(m_ssl, m_socket);

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
   if (!m_connected)
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
X509_REQ *Tunnel::createCertificateRequest(const char *cn, EVP_PKEY **pkey)
{
   RSA *key = RSA_generate_key(NETXMS_RSA_KEYLEN, 17, NULL, NULL);
   if (key == NULL)
   {
      debugPrintf(4, _T("call to RSA_generate_key() failed"));
      return NULL;
   }

   X509_REQ *req = X509_REQ_new();
   if (req != NULL)
   {
      X509_REQ_set_version(req, 1);
      X509_NAME *subject = X509_REQ_get_subject_name(req);
      if (subject != NULL)
      {
         X509_NAME_add_entry_by_txt(subject,"O", MBSTRING_UTF8, (const BYTE *)"netxms.org", -1, -1, 0);
         X509_NAME_add_entry_by_txt(subject,"CN", MBSTRING_UTF8, (const BYTE *)cn, -1, -1, 0);

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
   BYTE addressHash[18];
   m_address.buildHashKey(addressHash);

   TCHAR prefix[48];
   BinToStr(addressHash, 18, prefix);

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

   EVP_PKEY *key = NULL;
   X509_REQ *req = createCertificateRequest(cn, &key);
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
                  X509 *cert = d2i_X509(NULL, &certData, certLen);
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

   InetAddress addr = InetAddress::resolveHostName(config);
   if (!addr.isValidUnicast())
      return NULL;

   return new Tunnel(addr, port);
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
         nxlog_debug(1, _T("Added server tunnel %s"), (const TCHAR *)t->getAddress().toString());
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
      nxlog_debug(3, _T("No tunnels configured, tunnel manager will not start"));
      return THREAD_OK;
   }

   nxlog_debug(3, _T("Tunnel manager started"));
   while(!AgentSleepAndCheckForShutdown(30000))
   {
      for(int i = 0; i < s_tunnels.size(); i++)
      {
         Tunnel *t = s_tunnels.get(i);
         t->checkConnection();
      }
   }
   nxlog_debug(3, _T("Tunnel manager stopped"));
   return THREAD_OK;
}
