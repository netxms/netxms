/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
#include <openssl/ssl.h>
#include <nxstat.h>
#include <netxms-version.h>

#define DEBUG_TAG _T("tunnel")

#define REQUEST_TIMEOUT	10000

#ifdef _WITH_ENCRYPTION

/**
 * Check if server address is valid
 */
bool IsValidServerAddress(const InetAddress &addr, bool *pbMasterServer, bool *pbControlServer, bool forceResolve);

/**
 * Register session
 */
bool RegisterSession(const shared_ptr<CommSession>& session);

#ifdef _WIN32

#include <openssl/engine.h>

ENGINE *CreateCNGEngine();
bool MatchWindowsStoreCertificate(PCCERT_CONTEXT context, const TCHAR *id);
void SSLSetCertificateId(SSL_CTX *sslctx, const TCHAR *id);

#endif  /* _WIN32 */

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
   uint32_t m_id;
   bool m_active;
   VolatileCounter m_closed;
   RingBuffer m_buffer;
#ifdef _WIN32
   CRITICAL_SECTION m_bufferLock;
   CONDITION_VARIABLE m_dataCondition;
#else
   pthread_mutex_t m_bufferLock;
   pthread_cond_t m_dataCondition;
#endif

public:
   TunnelCommChannel(Tunnel *tunnel);
   virtual ~TunnelCommChannel();

   virtual ssize_t send(const void *data, size_t size, Mutex *mutex = nullptr) override;
   virtual ssize_t recv(void *buffer, size_t size, uint32_t timeout = INFINITE) override;
   virtual int poll(uint32_t timeout, bool write = false) override;
   virtual void backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*), void *context) override;
   virtual int shutdown() override;
   virtual void close() override;

   uint32_t getId() const { return m_id; }

   void putData(const BYTE *data, size_t size);
};

/**
 * Tunnel class
 */
class Tunnel
{
private:
   TCHAR *m_hostname;
   uint16_t m_port;
   TCHAR *m_certificate;
   char *m_pkeyPassword;
   InetAddress m_address;
   SOCKET m_socket;
   SSL_CTX *m_context;
   SSL *m_ssl;
   Mutex m_sslLock;
   Mutex m_writeLock;
   Mutex m_stateLock;
   bool m_connected;
   bool m_reset;
   bool m_forceResolve;
   VolatileCounter m_requestId;
   THREAD m_recvThread;
   MsgWaitQueue *m_queue;
   SharedHashMap<uint32_t, TunnelCommChannel> m_channels;
   Mutex m_channelLock;
   int m_tlsHandshakeFailures;
   bool m_ignoreClientCertificate;
   BYTE *m_fingerprint; // Server certificate fingerprint

   Tunnel(const TCHAR *hostname, uint16_t port, const TCHAR *certificate, const TCHAR *pkeyPassword, const BYTE *fingerprint);

   bool connectToServer();
   void processHandshakeError(int sslErr, bool certificateLoaded);
   int sslWrite(const void *data, size_t size);
   bool sendMessage(const NXCPMessage& msg);
   NXCPMessage *waitForMessage(uint16_t code, uint32_t id) { return (m_queue != nullptr) ? m_queue->waitForMessage(code, id, REQUEST_TIMEOUT) : nullptr; }

   void processBindRequest(NXCPMessage *request);
   void processChannelCloseRequest(const NXCPMessage& request);
   void createSession(const NXCPMessage& request);

   X509_REQ *createCertificateRequest(const char *country, const char *org, const char *cn, EVP_PKEY **pkey);
   bool saveCertificate(X509 *cert, EVP_PKEY *key);
   bool loadCertificate();
   bool loadCertificateFromFile();
   bool loadCertificateFromStore();
   bool loadCertificateFromStoreWithPK();
   bool loadCertificateFromStoreWithEngine();

   void recvThread();
   static THREAD_RESULT THREAD_CALL recvThreadStarter(void *arg);

   bool verifyServerCertificateFingerprint(STACK_OF(X509) * certChain);

public:
   ~Tunnel();

   void checkConnection();
   void disconnect();

   shared_ptr<TunnelCommChannel> createChannel();
   void closeChannel(TunnelCommChannel *channel);
   ssize_t sendChannelData(uint32_t id, const void *data, size_t len);

   const TCHAR *getHostname() const { return m_hostname; }

   void debugPrintf(int level, const TCHAR *format, ...);

   static Tunnel *createFromConfig(const TCHAR *config);
   static Tunnel *createFromConfig(const ConfigEntry& ce);
};

/**
 * Tunnel constructor
 */
Tunnel::Tunnel(const TCHAR *hostname, uint16_t port, const TCHAR *certificate, const TCHAR *pkeyPassword, const BYTE *fingerprint = nullptr)
   : m_channelLock(MutexType::FAST)
{
   m_hostname = MemCopyString(hostname);
   m_port = port;
   m_certificate = MemCopyString(certificate);
#ifdef UNICODE
   m_pkeyPassword = UTF8StringFromWideString(pkeyPassword);
#else
   m_pkeyPassword = MemCopyStringA(pkeyPassword);
#endif
   m_socket = INVALID_SOCKET;
   m_context = nullptr;
   m_ssl = nullptr;
   m_connected = false;
   m_reset = false;
   m_forceResolve = false;
   m_requestId = 0;
   m_recvThread = INVALID_THREAD_HANDLE;
   m_queue = nullptr;
   m_tlsHandshakeFailures = 0;
   m_ignoreClientCertificate = false;
   m_fingerprint = fingerprint == nullptr ? nullptr : MemCopyBlock(fingerprint, SHA256_DIGEST_SIZE);
}

/**
 * Tunnel destructor
 */
Tunnel::~Tunnel()
{
   disconnect();
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
   if (m_ssl != nullptr)
      SSL_free(m_ssl);
   if (m_context != nullptr)
      SSL_CTX_free(m_context);
   MemFree(m_hostname);
   MemFree(m_certificate);
   MemFree(m_pkeyPassword);
   MemFree(m_fingerprint);
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
   m_stateLock.lock();
   if (m_socket != INVALID_SOCKET)
      shutdown(m_socket, SHUT_RDWR);
   m_connected = false;
   ThreadJoin(m_recvThread);
   m_recvThread = INVALID_THREAD_HANDLE;
   delete_and_null(m_queue);
   m_stateLock.unlock();

   SharedObjectArray<TunnelCommChannel> channels(g_maxCommSessions, 16);
   m_channelLock.lock();
   auto it = m_channels.begin();
   while(it.hasNext())
      channels.add(it.next());
   m_channelLock.unlock();

   for(int i = 0; i < channels.size(); i++)
      channels.get(i)->close();
}

/**
 * Receiver thread starter
 */
THREAD_RESULT THREAD_CALL Tunnel::recvThreadStarter(void *arg)
{
   char name[256];
#ifdef UNICODE
   snprintf(name, 256, "TunnelRecvThread/%S", static_cast<Tunnel*>(arg)->m_hostname);
#else
   snprintf(name, 256, "TunnelRecvThread/%s", static_cast<Tunnel*>(arg)->m_hostname);
#endif
   static_cast<Tunnel*>(arg)->recvThread();
   return THREAD_OK;
}

/**
 * Receiver thread
 */
void Tunnel::recvThread()
{
   TlsMessageReceiver receiver(m_socket, m_ssl, &m_sslLock, 8192, MAX_AGENT_MSG_SIZE);
   while(m_connected)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(1000, &result);
      if (msg != nullptr)
      {
         if (nxlog_get_debug_level() >= 6)
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received message %s (%u)"), NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());
         }

         if (msg->getCode() == CMD_RESET_TUNNEL)
         {
            m_reset = true;
            debugPrintf(4, _T("Receiver thread stopped (tunnel reset)"));
            delete msg;
            break;
         }

         switch(msg->getCode())
         {
            case CMD_BIND_AGENT_TUNNEL:
               ThreadPoolExecute(g_commThreadPool, this, &Tunnel::processBindRequest, msg);
               msg = nullptr; // prevent message deletion
               break;
            case CMD_CREATE_CHANNEL:
               createSession(*msg);
               break;
            case CMD_CHANNEL_DATA:
               if (msg->isBinary())
               {
                  m_channelLock.lock();
                  shared_ptr<TunnelCommChannel> channel = m_channels.getShared(msg->getId());
                  m_channelLock.unlock();
                  if (channel != nullptr)
                  {
                     channel->putData(msg->getBinaryData(), msg->getBinaryDataSize());
                  }
               }
               break;
            case CMD_CLOSE_CHANNEL:
               processChannelCloseRequest(*msg);
               break;
            default:
               m_queue->put(msg);
               msg = nullptr; // prevent message deletion
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
   nxlog_report_event(61, NXLOG_WARNING, 1, _T("Tunnel with %s closed"), m_hostname);
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
   m_writeLock.lock();
   do
   {
      canRetry = false;
      m_sslLock.lock();
      bytes = SSL_write(m_ssl, data, static_cast<int>(size));
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, bytes);
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            m_sslLock.unlock();
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(REQUEST_TIMEOUT) > 0)
               canRetry = true;
            m_sslLock.lock();
         }
         else
         {
            debugPrintf(7, _T("SSL_write error (bytes=%d ssl_err=%d socket_err=%d)"), bytes, err, WSAGetLastError());
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
         }
      }
      m_sslLock.unlock();
   }
   while(canRetry);
   m_writeLock.unlock();
   return bytes;
}

/**
 * Send message
 */
bool Tunnel::sendMessage(const NXCPMessage& msg)
{
   if (!m_connected || m_reset)
      return false;

   if (nxlog_get_debug_level() >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s (%u)"), NXCPMessageCodeName(msg.getCode(), buffer), msg.getId());
   }
   NXCP_MESSAGE *data = msg.serialize(false);
   bool success = (sslWrite(data, ntohl(data->size)) == ntohl(data->size));
   MemFree(data);
   return success;
}

/**
 * Load certificate for this tunnel from file
 */
bool Tunnel::loadCertificate()
{
   return ((m_certificate != nullptr) && (*m_certificate == _T('@'))) ? loadCertificateFromStore() : loadCertificateFromFile();
}

/**
 * Load certificate for this tunnel from file
 */
bool Tunnel::loadCertificateFromFile()
{
   debugPrintf(6, _T("Loading certificate from file"));

   const TCHAR *name;
   TCHAR prefix[48], nameBuffer[MAX_PATH];
   if (m_certificate == nullptr)
   {
      BYTE addressHash[SHA1_DIGEST_SIZE];
#ifdef UNICODE
      char *un = MBStringFromWideString(m_hostname);
      CalculateSHA1Hash((BYTE *)un, strlen(un), addressHash);
      MemFree(un);
#else
      CalculateSHA1Hash((BYTE *)m_hostname, strlen(m_hostname), addressHash);
#endif

      BinToStr(addressHash, SHA1_DIGEST_SIZE, prefix);
      _sntprintf(nameBuffer, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);
      name = nameBuffer;
   }
   else
   {
      name = m_certificate;
      debugPrintf(4, _T("Using externally provisioned certificate \"%s\""), name);
   }

   FILE *f = _tfopen(name, _T("r"));
   if (f == nullptr)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      if ((errno == ENOENT) && (m_certificate == nullptr))
      {
         // Try fallback file
         BYTE addressHash[SHA1_DIGEST_SIZE];
         m_address.buildHashKey(addressHash);
         BinToStr(addressHash, 18, prefix);
         _sntprintf(nameBuffer, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);

         f = _tfopen(nameBuffer, _T("r"));
         if (f == nullptr)
         {
            debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), nameBuffer, _tcserror(errno));
            return false;
         }
      }
      else
      {
         return false;
      }
   }

   X509 *cert = PEM_read_X509(f, nullptr, nullptr, nullptr);
   if (cert == nullptr)
   {
      debugPrintf(4, _T("Cannot load certificate from file \"%s\""), name);
      fclose(f);
      return false;
   }

   // For externally provisioned certificate expect key to follow certificate in same PEM file
   if (m_certificate == nullptr)
   {
      fclose(f);
      _sntprintf(nameBuffer, MAX_PATH, _T("%s%s.key"), g_certificateDirectory, prefix);
      f = _tfopen(nameBuffer, _T("r"));
      if (f == nullptr)
      {
         debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), nameBuffer, _tcserror(errno));
         X509_free(cert);
         return false;
      }
   }

   EVP_PKEY *key = PEM_read_PrivateKey(f, nullptr, nullptr, (void *)((m_pkeyPassword != nullptr) ? m_pkeyPassword : "nxagentd"));
   fclose(f);

   if (key == nullptr)
   {
      debugPrintf(4, _T("Cannot load private key from file \"%s\""), name);
      X509_free(cert);
      return false;
   }

   bool success = false;
   if (SSL_CTX_use_certificate(m_context, cert) == 1)
   {
      if (SSL_CTX_use_PrivateKey(m_context, key) == 1)
      {
         debugPrintf(4, _T("Certificate and private key loaded"));
         success = true;
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
   return success;
}

/**
 * Mutex for engine load
 */
#ifdef _WIN32
static Mutex s_mutexEngineLoad;
#endif

/**
 * Load certificate for this tunnel from Windows store
 */
bool Tunnel::loadCertificateFromStore()
{
#ifdef _WIN32
   debugPrintf(6, _T("Loading certificate from system certificate store"));
   if (loadCertificateFromStoreWithPK())
      return true;
   return loadCertificateFromStoreWithEngine();
#else
   debugPrintf(2, _T("Loading certificate from system store supported only on Windows platform"));
   return false;
#endif
}

/**
 * Load certificate for this tunnel from Windows store with private key
 */
bool Tunnel::loadCertificateFromStoreWithPK()
{
#ifdef _WIN32
   HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, 0,
         CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE, "MY");
   if (hStore == nullptr)
   {
      TCHAR buffer[1024];
      debugPrintf(4, _T("loadCertificateFromStoreWithPK: cannot open certificate store \"MY\" for local system(%s)"),
            GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }

   PCCERT_CONTEXT context = nullptr;
   while ((context = CertEnumCertificatesInStore(hStore, context)) != nullptr)
   {
      if (MatchWindowsStoreCertificate(context, &m_certificate[1]))
      {
         TCHAR certName[1024];
         CertGetNameString(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, certName, 1024);
         debugPrintf(5, _T("loadCertificateFromStoreWithPK: certificate %s matched"), certName);

         HCRYPTPROV_OR_NCRYPT_KEY_HANDLE keyHandle = 0;
         DWORD spec;
         BOOL owned;
         BOOL success = CryptAcquireCertificatePrivateKey(context, CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG, nullptr, &keyHandle, &spec, &owned);
         if (!success || (spec != CERT_NCRYPT_KEY_SPEC) || !owned)
         {
            TCHAR buffer[1024];
            debugPrintf(5, _T("loadCertificateFromStoreWithPK: cannot acquire private key (%s)"),
                  GetSystemErrorText(GetLastError(), buffer, 1024));
            if (success && owned && (spec != CERT_NCRYPT_KEY_SPEC))
               CryptReleaseContext(keyHandle, 0);
            continue;
         }

         DWORD blobSize = 0;
         SECURITY_STATUS status = NCryptExportKey(keyHandle, 0, BCRYPT_RSAFULLPRIVATE_BLOB, NULL, NULL, 0, &blobSize, NCRYPT_SILENT_FLAG);
         if (status != ERROR_SUCCESS)
         {
            TCHAR buffer[1024];
            debugPrintf(5, _T("loadCertificateFromStoreWithPK: key export failed (%s)"), GetSystemErrorText(status, buffer, 1024));
            NCryptFreeObject(keyHandle);
            continue;
         }

         BYTE *blob = MemAllocArray<BYTE>(blobSize);
         status = NCryptExportKey(keyHandle, 0, BCRYPT_RSAFULLPRIVATE_BLOB, NULL, blob, blobSize, &blobSize, NCRYPT_SILENT_FLAG);
         if (status != ERROR_SUCCESS)
         {
            TCHAR buffer[1024];
            debugPrintf(5, _T("loadCertificateFromStoreWithPK: key export failed (%s)"), GetSystemErrorText(status, buffer, 1024));
            NCryptFreeObject(keyHandle);
            MemFree(blob);
            continue;
         }
         NCryptFreeObject(keyHandle);
         BCRYPT_RSAKEY_BLOB *header = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(blob);
         debugPrintf(7, _T("loadCertificateFromStoreWithPK: key exported (%d bits)"), header->BitLength);

         BIGNUM *n = BN_bin2bn(blob + sizeof(BCRYPT_RSAKEY_BLOB) + header->cbPublicExp, header->cbModulus, nullptr);
         BIGNUM *e = BN_bin2bn(blob + sizeof(BCRYPT_RSAKEY_BLOB), header->cbPublicExp, nullptr);
         BIGNUM *d = BN_bin2bn(blob + sizeof(BCRYPT_RSAKEY_BLOB) +
                  header->cbPublicExp + header->cbModulus + header->cbPrime1 * 3 + header->cbPrime2 * 2,
               header->cbModulus, nullptr);
         MemFree(blob);
         if ((n == nullptr) || (e == nullptr) || (d == nullptr))
         {
            debugPrintf(5, _T("loadCertificateFromStoreWithPK: key component conversion failed"));
            BN_free(n);
            BN_free(e);
            BN_free(d);
            continue;
         }

         RSA *rsa = RSA_new();
         if (!RSA_set0_key(rsa, n, e, d))
         {
            debugPrintf(5, _T("loadCertificateFromStoreWithPK: key component conversion failed"));
            BN_free(n);
            BN_free(e);
            BN_free(d);
            RSA_free(rsa);
            continue;
         }

         EVP_PKEY *pkey = EVP_PKEY_new();
         EVP_PKEY_assign_RSA(pkey, rsa);

         const unsigned char *in = context->pbCertEncoded;
         X509 *cert = d2i_X509(nullptr, &in, context->cbCertEncoded);
         if (cert == nullptr)
         {
            debugPrintf(5, _T("loadCertificateFromStoreWithPK: cannot decode certificate"));
            EVP_PKEY_free(pkey);
            continue;
         }

         success = false;
         if (SSL_CTX_use_certificate(m_context, cert) == 1)
         {
            if (SSL_CTX_use_PrivateKey(m_context, pkey) == 1)
            {
               debugPrintf(4, _T("Certificate and private key loaded"));
               success = true;
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
         EVP_PKEY_free(pkey);

         if (success)
         {
            CertFreeCertificateContext(context);
            break;
         }
      }
   }

   CertCloseStore(hStore, 0);

   if (context == nullptr)
   {
      debugPrintf(5, _T("loadCertificateFromStoreWithPK: no matching certificates in store"));
      return false;
   }

   return true;
#else
   return false;
#endif
}

/**
 * Load certificate for this tunnel from Windows store using crypto engine
 */
bool Tunnel::loadCertificateFromStoreWithEngine()
{
#ifdef _WIN32
   debugPrintf(6, _T("Fallback to OpenSSL engine for signing (will switch to TLS 1.1)"));

   s_mutexEngineLoad.lock();

   ENGINE *e = CreateCNGEngine();
   if (e == nullptr)
   {
      debugPrintf(2, _T("Cannot create OpenSSL engine"));
      s_mutexEngineLoad.unlock();
      return false;
   }

   if (ENGINE_init(e))
   {
      debugPrintf(6, _T("OpenSSL engine initialized"));
   }
   else
   {
      debugPrintf(2, _T("Cannot initialize OpenSSL engine"));
      ENGINE_free(e);
      s_mutexEngineLoad.lock();
      return false;
   }

   s_mutexEngineLoad.unlock();

   if (!SSL_CTX_set_client_cert_engine(m_context, e))
   {
      debugPrintf(2, _T("Cannot select engine in SSL context"));
      ENGINE_finish(e);
      ENGINE_free(e);
      return false;
   }

   SSLSetCertificateId(m_context, &m_certificate[1]);

   // Disable TLS 1.2+ because of this OpenSSL issue:
   // https://github.com/openssl/openssl/issues/12859
   SSL_CTX_set_options(m_context, SSL_CTX_get_options(m_context) | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3);

   ENGINE_free(e);
   return true;
#else
   return false;
#endif
}

/**
 * SSL message callback
 */
static void SSLInfoCallback(const SSL *ssl, int where, int ret)
{
   if (where & SSL_CB_ALERT)
   {
      nxlog_debug_tag(_T("ssl"), 4, _T("SSL %s alert: %hs (%hs)"), (where & SSL_CB_READ) ? _T("read") : _T("write"),
               SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
   }
   else if (where & SSL_CB_HANDSHAKE_START)
   {
      nxlog_debug_tag(_T("ssl"), 6, _T("SSL handshake start (%hs)"), SSL_state_string_long(ssl));
   }
   else if (where & SSL_CB_HANDSHAKE_DONE)
   {
      nxlog_debug_tag(_T("ssl"), 6, _T("SSL handshake done (%hs)"), SSL_state_string_long(ssl));
   }
   else
   {
      int method = where & ~SSL_ST_MASK;
      const TCHAR *prefix;
      if (method & SSL_ST_CONNECT)
         prefix = _T("SSL_connect");
      else if (method & SSL_ST_ACCEPT)
         prefix = _T("SSL_accept");
      else
         prefix = _T("undefined");

      if (where & SSL_CB_LOOP)
      {
         nxlog_debug_tag(_T("ssl"), 6, _T("%s: %hs"), prefix, SSL_state_string_long(ssl));
      }
      else if (where & SSL_CB_EXIT)
      {
         if (ret == 0)
            nxlog_debug_tag(_T("ssl"), 3, _T("%s: failed in %hs"), prefix, SSL_state_string_long(ssl));
         else if (ret < 0)
            nxlog_debug_tag(_T("ssl"), 3, _T("%s: error in %hs"), prefix, SSL_state_string_long(ssl));
      }
   }
}

#if HAVE_X509_STORE_CTX_SET_VERIFY_CB

/**
 * Certificate verification callback
 */
static int CertVerifyCallback(int success, X509_STORE_CTX *ctx)
{
   if (!success)
   {
      X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
      int error = X509_STORE_CTX_get_error(ctx);
      int depth = X509_STORE_CTX_get_error_depth(ctx);
      char subjectName[1024];
      X509_NAME_oneline(X509_get_subject_name(cert), subjectName, 1024);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Certificate \"%hs\" verification error %d (%hs) at depth %d"),
             subjectName, error, X509_verify_cert_error_string(error), depth);
   }
   return success;
}

#endif /* HAVE_X509_STORE_SET_VERIFY_CB */

/**
 * Verify server certificate
 */
static bool VerifyServerCertificate(X509 *cert)
{
   X509_STORE *trustedCertificateStore = CreateTrustedCertificatesStore(g_trustedRootCertificates, true);
   if (trustedCertificateStore == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("VerifyServerCertificate: cannot create certificate store"));
      return false;
   }

   bool valid = false;
   X509_STORE_CTX *ctx = X509_STORE_CTX_new();
   if (ctx != nullptr)
   {
      X509_STORE_CTX_init(ctx, trustedCertificateStore, cert, nullptr);
#if HAVE_X509_STORE_CTX_SET_VERIFY_CB
      X509_STORE_CTX_set_verify_cb(ctx, CertVerifyCallback);
#endif
      if (X509_verify_cert(ctx))
      {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
         STACK_OF(X509) *chain = X509_STORE_CTX_get0_chain(ctx);
         if (sk_X509_num(chain) > 1)
         {
            String dp = GetCertificateCRLDistributionPoint(cert);
            if (!dp.isEmpty())
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("VerifyServerCertificate: certificate CRL DP: %s"), dp.cstr());
               char *url = dp.getUTF8String();
               AddRemoteCRL(url, true);
               MemFree(url);
            }

            X509 *issuer = sk_X509_value(chain, 1);
            if (!CheckCertificateRevocation(cert, issuer))
            {
               valid = true;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("VerifyServerCertificate: certificate is revoked"));
            }
         }
#else
         nxlog_debug_tag(DEBUG_TAG, 4, _T("VerifyServerCertificate: CRL check is not implemented for this OpenSSL version"));
         valid = true;
#endif
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("VerifyServerCertificate: verification failed (%hs)"), X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx)));
      }
      X509_STORE_CTX_free(ctx);
   }
   else
   {
      TCHAR buffer[256];
      nxlog_debug_tag(DEBUG_TAG, 3, _T("VerifyServerCertificate: X509_STORE_CTX_new() failed: %s"), _ERR_error_tstring(ERR_get_error(), buffer));
   }
   X509_STORE_free(trustedCertificateStore);
   return valid;
}

/**
 * Verify server certificate
 */
bool Tunnel::verifyServerCertificateFingerprint(STACK_OF(X509) * chain)
{
   bool valid = false;
   if (chain != nullptr)
   {
      for (int i = 0; i < sk_X509_num(chain); i++)
      {
         X509 *cert = sk_X509_value(chain, i);
         BYTE md[SHA256_DIGEST_SIZE];

         if (X509_digest(cert, EVP_sha256(), md, nullptr) && !memcmp(md, m_fingerprint, SHA256_DIGEST_SIZE))
         {
            valid = true;
            break;
         }
      }
   }
   return valid;
}

/**
 * Process handshake error
 */
void Tunnel::processHandshakeError(int sslErr, bool certificateLoaded)
{
   char buffer[128];
   debugPrintf(4, _T("TLS handshake failed (%hs)"), ERR_error_string(sslErr, buffer));

   unsigned long error;
   while((error = ERR_get_error()) != 0)
   {
      ERR_error_string_n(error, buffer, sizeof(buffer));
      debugPrintf(5, _T("Caused by: %hs"), buffer);
   }

   if (certificateLoaded)
   {
      m_tlsHandshakeFailures++;
      if (m_tlsHandshakeFailures >= 10)
      {
         m_ignoreClientCertificate = true;
         m_tlsHandshakeFailures = 0;
         debugPrintf(4, _T("Next connection attempt will ignore agent certificate"));
      }
   }
}

/**
 * Connect to server
 */
bool Tunnel::connectToServer()
{
   m_stateLock.lock();

   // Cleanup from previous connection attempt
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
   if (m_ssl != nullptr)
      SSL_free(m_ssl);
   if (m_context != nullptr)
      SSL_CTX_free(m_context);

   m_socket = INVALID_SOCKET;
   m_context = nullptr;
   m_ssl = nullptr;

   m_address = InetAddress::resolveHostName(m_hostname);
   if (!m_address.isValidUnicast())
   {
      debugPrintf(4, _T("Server address cannot be resolved or is not valid"));
      m_stateLock.unlock();
      return false;
   }

   // Create socket and connect
   m_socket = ConnectToHost(m_address, m_port, REQUEST_TIMEOUT);
   if (m_socket == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      debugPrintf(4, _T("Cannot establish connection (%s)"), GetLastSocketErrorText(buffer, 1024));
      m_stateLock.unlock();
      return false;
   }

   // Setup secure connection
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   if (method == nullptr)
   {
      debugPrintf(4, _T("Cannot obtain TLS method"));
      m_stateLock.unlock();
      return false;
   }

   m_context = SSL_CTX_new((SSL_METHOD *)method);
   if (m_context == nullptr)
   {
      debugPrintf(4, _T("Cannot create TLS context"));
      m_stateLock.unlock();
      return false;
   }
   if (g_dwFlags & AF_ENABLE_SSL_TRACE)
   {
      SSL_CTX_set_info_callback(m_context, SSLInfoCallback);
   }
#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif
   bool certificateLoaded = m_ignoreClientCertificate ? false : loadCertificate();
   m_ignoreClientCertificate = false;  // reset ignore flag for next try

   m_ssl = SSL_new(m_context);
   if (m_ssl == nullptr)
   {
      debugPrintf(4, _T("Cannot create SSL object"));
      m_stateLock.unlock();
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
         if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
         {
            SocketPoller poller(sslErr == SSL_ERROR_WANT_WRITE);
            poller.add(m_socket);
            if (poller.poll(REQUEST_TIMEOUT) > 0)
            {
               debugPrintf(8, _T("TLS handshake: %s wait completed"), (sslErr == SSL_ERROR_WANT_READ) ? _T("read") : _T("write"));
               continue;
            }
            debugPrintf(4, _T("TLS handshake failed (timeout on %s)"), (sslErr == SSL_ERROR_WANT_READ) ? _T("read") : _T("write"));
            m_stateLock.unlock();
            return false;
         }
         else
         {
            processHandshakeError(sslErr, certificateLoaded);
            m_stateLock.unlock();
            return false;
         }
      }
      break;
   }

   // Check if TLS connection still valid. If agent's certificate verification fails on server side,
   // TLS handshake still completes successfully on agent side
   bool canRead = (SSL_pending(m_ssl) != 0);
   if (!canRead)
   {
      SocketPoller sp;
      sp.add(m_socket);
      canRead = (sp.poll(1000) > 0);
   }
   if (canRead)
   {
      // Server will wait for first message from agent, so we can attempt read here without loosing incoming NXCP messages
      char buffer[16];
      int rc = SSL_read(m_ssl, buffer, sizeof(buffer));
      if (rc <= 0)
      {
         int sslErr = SSL_get_error(m_ssl, rc);
         if ((sslErr != SSL_ERROR_WANT_READ) && (sslErr != SSL_ERROR_WANT_WRITE))
         {
            processHandshakeError(sslErr, certificateLoaded);
            m_stateLock.unlock();
            return false;
         }
      }
   }

   debugPrintf(6, _T("TLS handshake completed"));
   m_tlsHandshakeFailures = 0;

   // Check server certificate
   X509 *cert = SSL_get_peer_certificate(m_ssl);
   if (cert == nullptr)
   {
      debugPrintf(4, _T("Server certificate not provided"));
      if (m_socket != INVALID_SOCKET)
      {
         shutdown(m_socket, SHUT_RDWR);
         m_socket = INVALID_SOCKET;
      }
      m_stateLock.unlock();
      return false;
   }

   String subject = GetCertificateSubjectString(cert);
   String issuer = GetCertificateIssuerString(cert);
   debugPrintf(4, _T("Server certificate subject is %s"), subject.cstr());
   debugPrintf(4, _T("Server certificate issuer is %s"), issuer.cstr());

   bool isValid = true;
   if ((g_dwFlags & AF_CHECK_SERVER_CERTIFICATE) || (m_fingerprint != nullptr))
   {
      debugPrintf(3, _T("Verifying server certificate"));
      isValid = VerifyServerCertificate(cert);
      debugPrintf(3, _T("Certificate \"%s\" for issuer %s - verification %s"), subject.cstr(), issuer.cstr(), isValid ? _T("successful") : _T("failed"));
   }
   else
   {
      debugPrintf(3, _T("Server certificate verification is disabled"));
   }

   if (isValid)
   {
      if (m_fingerprint != nullptr)
      {
         debugPrintf(3, _T("Verifying server certificate fingerprint"));
         isValid = verifyServerCertificateFingerprint(SSL_get_peer_cert_chain(m_ssl));
         debugPrintf(3, _T("Certificate \"%s\" for issuer %s - fingerprint verification %s"), subject.cstr(), issuer.cstr(), isValid ? _T("successful") : _T("failed"));
      }
      else
      {
         debugPrintf(3, _T("Server certificate pinning is disabled"));
      }
   }

   X509_free(cert);
   if (!isValid)
   {
      if (m_socket != INVALID_SOCKET)
      {
         shutdown(m_socket, SHUT_RDWR);
         m_socket = INVALID_SOCKET;
      }
      m_stateLock.unlock();
      return false;
   }

   // Setup receiver
   delete m_queue;
   m_queue = new MsgWaitQueue();
   m_connected = true;
   m_recvThread = ThreadCreateEx(Tunnel::recvThreadStarter, 0, this);

   m_stateLock.unlock();

   m_requestId = 0;

   // Do handshake
   NXCPMessage msg(CMD_SETUP_AGENT_TUNNEL, InterlockedIncrement(&m_requestId), 4);  // Use version 4 during setup
   msg.setField(VID_AGENT_VERSION, NETXMS_VERSION_STRING);
   msg.setField(VID_AGENT_BUILD_TAG, NETXMS_BUILD_TAG);
   msg.setField(VID_AGENT_ID, g_agentId);
   msg.setField(VID_SYS_NAME, g_systemName);
   msg.setField(VID_ZONE_UIN, g_zoneUIN);
   msg.setField(VID_USERAGENT_INSTALLED, IsUserAgentInstalled());
   msg.setField(VID_AGENT_PROXY, (g_dwFlags & AF_ENABLE_PROXY) ? true : false);
   msg.setField(VID_SNMP_PROXY, (g_dwFlags & AF_ENABLE_SNMP_PROXY) ? true : false);
   msg.setField(VID_SNMP_TRAP_PROXY, (g_dwFlags & AF_ENABLE_SNMP_TRAP_PROXY) ? true : false);
   msg.setField(VID_SYSLOG_PROXY, (g_dwFlags & AF_ENABLE_SYSLOG_PROXY) ? true : false);
   msg.setField(VID_EXTPROV_CERTIFICATE, m_certificate != nullptr);

   BYTE hwid[HARDWARE_ID_LENGTH];
   if (GetSystemHardwareId(hwid))
      msg.setField(VID_HARDWARE_ID, hwid, sizeof(hwid));

   char serial[256];
   if (GetHardwareSerialNumber(serial, sizeof(serial)))
      msg.setFieldFromMBString(VID_SERIAL_NUMBER, serial);

   TCHAR fqdn[256];
   if (GetLocalHostName(fqdn, 256, true))
      msg.setField(VID_HOSTNAME, fqdn);

   VirtualSession session(0);
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (GetMetricValue(_T("System.PlatformName"), buffer, &session) == ERR_SUCCESS)
      msg.setField(VID_PLATFORM_NAME, buffer);
   if (GetMetricValue(_T("System.UName"), buffer, &session) == ERR_SUCCESS)
      msg.setField(VID_SYS_DESCRIPTION, buffer);

   StringList ifList;
   if (GetListValue(_T("Net.InterfaceList"), &ifList, &session) == ERR_SUCCESS)
   {
      HashSet<MacAddress> macAddressList;
      for(int i = 0; i < ifList.size(); i++)
      {
         TCHAR buffer[MAX_RESULT_LENGTH];
         ExtractWord(ifList.get(i), buffer, 3);
         MacAddress macAddr = MacAddress::parse(buffer);
         if (macAddr.isValid())
            macAddressList.put(macAddr);
      }
      msg.setField(VID_MAC_ADDR_COUNT, macAddressList.size());
      uint32_t fieldId = VID_MAC_ADDR_LIST_BASE;
      for(const MacAddress *m : macAddressList)
         msg.setField(fieldId++, *m);
   }

   sendMessage(msg);

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response == nullptr)
   {
      debugPrintf(4, _T("Cannot configure tunnel (request timeout)"));
      disconnect();
      return false;
   }

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc != ERR_SUCCESS)
   {
      debugPrintf(4, _T("Cannot configure tunnel (error %d)"), rcc);
      disconnect();
      return false;
   }

   // Force master/control server DNS names resolve after new tunnel is established
   // This will fix the following situation:
   //    - server DNS name was not available at agent startup
   //    - DNS name was resolved later and used for establishing tunnel
   //    - first session from server got wrong access because DNS name in server info structure still unresolved
   m_forceResolve = true;

   nxlog_report_event(60, NXLOG_INFO, 1, _T("Tunnel with %s established"), m_hostname);
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
      NXCPMessage msg(CMD_KEEPALIVE, InterlockedIncrement(&m_requestId), 4);
      if (sendMessage(msg))
      {
         NXCPMessage *response = waitForMessage(CMD_KEEPALIVE, msg.getId());
         if (response == nullptr)
         {
            debugPrintf(3, _T("Connection test failed"));
            disconnect();
         }
         else
         {
            delete response;
         }
      }
      else
      {
         debugPrintf(3, _T("Connection test failed"));
         disconnect();
      }
   }
}

/**
 * Create certificate request
 */
X509_REQ *Tunnel::createCertificateRequest(const char *country, const char *org, const char *cn, EVP_PKEY **pkey)
{
   RSA_KEY key = RSAGenerateKey(NETXMS_RSA_KEYLEN);
   if (key == nullptr)
   {
      debugPrintf(4, _T("call to RSAGenerateKey() failed"));
      return nullptr;
   }

   X509_REQ *req = X509_REQ_new();
   if (req != nullptr)
   {
      X509_REQ_set_version(req, 1);
      X509_NAME *subject = X509_REQ_get_subject_name(req);
      if (subject != nullptr)
      {
         if (country != nullptr)
            X509_NAME_add_entry_by_txt(subject, "C", MBSTRING_UTF8, (const BYTE *)country, -1, -1, 0);
         X509_NAME_add_entry_by_txt(subject, "O", MBSTRING_UTF8, (const BYTE *)((org != NULL) ? org : "netxms.org"), -1, -1, 0);
         X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_UTF8, (const BYTE *)cn, -1, -1, 0);

         EVP_PKEY *ekey = EVP_PKEY_from_RSA_KEY(key);
         if (ekey != nullptr)
         {
            key = nullptr; // will be freed by EVP_PKEY_free
            X509_REQ_set_pubkey(req, ekey);
            if (X509_REQ_sign(req, ekey, EVP_sha256()) > 0)
            {
               *pkey = ekey;
            }
            else
            {
               debugPrintf(4, _T("call to X509_REQ_sign() failed"));
               X509_REQ_free(req);
               req = nullptr;
               EVP_PKEY_free(ekey);
            }
         }
         else
         {
            debugPrintf(4, _T("call to EVP_PKEY_from_RSA_KEY() failed"));
            X509_REQ_free(req);
            req = nullptr;
         }
      }
      else
      {
         debugPrintf(4, _T("call to X509_REQ_get_subject_name() failed"));
         X509_REQ_free(req);
         req = nullptr;
      }
   }
   else
   {
      debugPrintf(4, _T("call to X509_REQ_new() failed"));
   }

   RSAFree(key);
   return req;
}

/**
 * Creates certificate and key copy if files exist. Files are copied as NAME.DATE
 */
static void BackupFileIfExist(const TCHAR *name)
{
   if (_taccess(name, 0) != 0)
     return;

   TCHAR formatedTime[256];
   time_t t = time(nullptr);
#if HAVE_LOCALTIME_R
   struct tm tmbuffer;
   struct tm *ltm = localtime_r(&t, &tmbuffer);
#else
   struct tm *ltm = localtime(&t);
#endif
   _tcsftime(formatedTime, 256, _T("%Y.%m.%d.%H.%M.%S"), ltm);

   TCHAR newName[MAX_PATH];
   _sntprintf(newName, MAX_PATH, _T("%s.%s"), name, formatedTime);

   _trename(name, newName);
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
   MemFree(un);
#else
   CalculateSHA1Hash((BYTE *)m_hostname, strlen(m_hostname), addressHash);
#endif

   TCHAR prefix[48];
   BinToStr(addressHash, SHA1_DIGEST_SIZE, prefix);

   TCHAR name[MAX_PATH];
   _sntprintf(name, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);
   BackupFileIfExist(name);
   FILE *f = _tfopen(name, _T("w"));
   if (f == nullptr)
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
   BackupFileIfExist(name);
   f = _tfopen(name, _T("w"));
   if (f == nullptr)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      return false;
   }
   rc = PEM_write_PrivateKey(f, key, EVP_des_ede3_cbc(), nullptr, 0, 0, (void *)"nxagentd");
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
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId(), 4);

   uuid guid = request->getFieldAsGUID(VID_GUID);
   char *cn = guid.toString().getUTF8String();

   char *country = request->getFieldAsUtf8String(VID_COUNTRY);
   char *org = request->getFieldAsUtf8String(VID_ORGANIZATION);

   EVP_PKEY *key = nullptr;
   X509_REQ *req = createCertificateRequest(country, org, cn, &key);

   MemFree(country);
   MemFree(org);
   MemFree(cn);

   if (req != nullptr)
   {
      BYTE *buffer = nullptr;
      int len = i2d_X509_REQ(req, &buffer);
      if (len > 0)
      {
         NXCPMessage certRequest(CMD_REQUEST_CERTIFICATE, request->getId(), 4);
         certRequest.setField(VID_CERTIFICATE, buffer, len);
         sendMessage(certRequest);
         OPENSSL_free(buffer);

         NXCPMessage *certResponse = waitForMessage(CMD_NEW_CERTIFICATE, request->getId());
         if (certResponse != nullptr)
         {
            uint32_t rcc = certResponse->getFieldAsUInt32(VID_RCC);
            if (rcc == ERR_SUCCESS)
            {
               size_t certLen;
               const BYTE *certData = certResponse->getBinaryFieldPtr(VID_CERTIFICATE, &certLen);
               if (certData != nullptr)
               {
                  X509 *cert = d2i_X509(nullptr, &certData, (long)certLen);
                  if (cert != nullptr)
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

   sendMessage(response);
   delete request;
}

/**
 * Create new session
 */
void Tunnel::createSession(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId(), 4);

   // Assume that peer always have minimal access, so don't check return value
   bool masterServer, controlServer;
   IsValidServerAddress(m_address, &masterServer, &controlServer, m_forceResolve);
   m_forceResolve = false;

   shared_ptr<TunnelCommChannel> channel = createChannel();
   if (channel != nullptr)
   {
      shared_ptr<CommSession> session = MakeSharedCommSession<CommSession>(channel, m_address, masterServer, controlServer);
      if (RegisterSession(session))
      {
         response.setField(VID_RCC, ERR_SUCCESS);
         response.setField(VID_CHANNEL_ID, channel->getId());
         debugPrintf(9, _T("New session registered"));
         session->run();
      }
      else
      {
         response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
      }
   }
   else
   {
      response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
   }

   sendMessage(response);
}

/**
 * Create channel
 */
shared_ptr<TunnelCommChannel> Tunnel::createChannel()
{
   shared_ptr<TunnelCommChannel> channel;
   m_channelLock.lock();
   if (m_channels.size() < (int)g_maxCommSessions)
   {
      channel = make_shared<TunnelCommChannel>(this);
      m_channels.set(channel->getId(), channel);
      debugPrintf(5, _T("New channel created (ID=%d)"), channel->getId());
   }
   m_channelLock.unlock();
   return channel;
}

/**
 * Process server's channel close request
 */
void Tunnel::processChannelCloseRequest(const NXCPMessage& request)
{
   uint32_t id = request.getFieldAsUInt32(VID_CHANNEL_ID);
   debugPrintf(5, _T("Close request for channel %d"), id);
   m_channelLock.lock();
   shared_ptr<TunnelCommChannel> channel = m_channels.getShared(id);
   m_channelLock.unlock();
   if (channel != nullptr)
   {
      channel->close();
   }
}

/**
 * Close channel
 */
void Tunnel::closeChannel(TunnelCommChannel *channel)
{
   uint32_t id = 0;
   m_channelLock.lock();
   if (m_channels.contains(channel->getId()))
   {
      id = channel->getId();
      debugPrintf(5, _T("Channel %d closed"), id);
      m_channels.remove(id);
   }
   m_channelLock.unlock();

   if (id != 0)
   {
      NXCPMessage msg(CMD_CLOSE_CHANNEL, 0, 4);
      msg.setField(VID_CHANNEL_ID, id);
      sendMessage(msg);
   }
}

/**
 * Send channel data
 */
ssize_t Tunnel::sendChannelData(uint32_t id, const void *data, size_t len)
{
   NXCP_MESSAGE *msg = CreateRawNXCPMessage(CMD_CHANNEL_DATA, id, 0, data, len, NULL, false);
   int rc = sslWrite(msg, ntohl(msg->size));
   if (rc == ntohl(msg->size))
      rc = (int)len;  // adjust number of bytes to exclude tunnel overhead
   MemFree(msg);
   return rc;
}

/**
 * Create tunnel object from configuration record
 * Record format is address[:port][,certificate[%password]]
 */
Tunnel *Tunnel::createFromConfig(const TCHAR *config)
{
   StringBuffer sb(config);

   // Get certificate option
   TCHAR *certificate = nullptr, *password = nullptr;
   TCHAR *p = _tcschr(sb.getBuffer(), _T(','));
   if (p != nullptr)
   {
      *p = 0;
      certificate = p + 1;
      p = _tcschr(certificate, _T('%'));
      if (p != nullptr)
      {
         *p = 0;
         password = p + 1;
         Trim(password);
      }
      Trim(certificate);
   }

   // Get port option
   int port = AGENT_TUNNEL_PORT;
   p = _tcschr(sb.getBuffer(), _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;

      TCHAR *eptr;
      port = _tcstol(p, &eptr, 10);
      if ((port < 1) || (port > 65535))
         return nullptr;
   }
   return new Tunnel(sb.cstr(), port, certificate, password);
}

/**
 * Create tunnel object from ConfigEntry
 */
Tunnel *Tunnel::createFromConfig(const ConfigEntry& ce)
{
   const TCHAR *hostname = ce.getSubEntryValue(_T("Hostname"), 0, nullptr);
   if (hostname == nullptr)
      hostname = ce.getName();

   uint16_t port = ce.getSubEntryValueAsUInt(_T("Port"), 0, AGENT_TUNNEL_PORT);

   TCHAR certificate[MAX_PATH];
   const TCHAR *certificateId = ce.getSubEntryValue(_T("CertificateId"), 0, nullptr);
   if (certificateId != nullptr)
   {
      certificate[0] = '@';
      _tcslcpy(&certificate[1], certificateId, MAX_PATH - 1);
   }
   else
   {
      const TCHAR *certificateFile = ce.getSubEntryValue(_T("CertificateFile"), 0, nullptr);
      if (certificateFile != nullptr)
      {
         _tcslcpy(certificate, certificateFile, MAX_PATH);
      }
      else
      {
         certificate[0] = 0;
      }
   }
   const TCHAR *password = nullptr;
   if (certificate[0] != 0)
      password = ce.getSubEntryValue(_T("Password"), 0, nullptr);

   StringBuffer fingerprintString = ce.getSubEntryValue(_T("ServerCertificateFingerprint"), 0, nullptr);
   if (fingerprintString.isEmpty())
      return new Tunnel(hostname, port, certificate, password);

   fingerprintString.replace(_T(":"), _T(""));
   BYTE fingerprint[SHA256_DIGEST_SIZE];
   StrToBin(fingerprintString, fingerprint, SHA256_DIGEST_SIZE);
   return new Tunnel(hostname, port, certificate, password, fingerprint);
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
   InitializeConditionVariable(&m_dataCondition);
#else
   pthread_mutex_init(&m_bufferLock, nullptr);
   pthread_cond_init(&m_dataCondition, nullptr);
#endif
}

/**
 * Channel destructor
 */
TunnelCommChannel::~TunnelCommChannel()
{
#ifdef _WIN32
   DeleteCriticalSection(&m_bufferLock);
#else
   pthread_mutex_destroy(&m_bufferLock);
   pthread_cond_destroy(&m_dataCondition);
#endif
}

/**
 * Send data
 */
ssize_t TunnelCommChannel::send(const void *data, size_t size, Mutex *mutex)
{
   return m_active ? m_tunnel->sendChannelData(m_id, data, size) : -1;
}

/**
 * Receive data
 */
ssize_t TunnelCommChannel::recv(void *buffer, size_t size, uint32_t timeout)
{
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   if (!m_active && m_buffer.isEmpty())
   {
      LeaveCriticalSection(&m_bufferLock);
      return 0;   // closed
   }
   while (m_buffer.isEmpty())
   {
      if (!SleepConditionVariableCS(&m_dataCondition, &m_bufferLock, timeout))
      {
         LeaveCriticalSection(&m_bufferLock);
         return -2;
      }

      if (!m_active)
      {
         LeaveCriticalSection(&m_bufferLock);
         return 0;   // closed while waiting
      }
   }
#else
   pthread_mutex_lock(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
      if (!m_active)
      {
         pthread_mutex_unlock(&m_bufferLock);
         return 0;   // closed
      }
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

   ssize_t bytes = m_buffer.read((BYTE *)buffer, size);
#ifdef _WIN32
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_unlock(&m_bufferLock);
#endif
   return bytes;
}

/**
 * Poll for data
 */
int TunnelCommChannel::poll(uint32_t timeout, bool write)
{
   if (write)
      return 1;

   if (!m_active)
      return -1;

#ifdef _WIN32
   int rc = 1;
   EnterCriticalSection(&m_bufferLock);
   while (m_buffer.isEmpty())
   {
      if (!SleepConditionVariableCS(&m_dataCondition, &m_bufferLock, timeout))
      {
         rc = 0;  // Timeout
         break;
      }

      if (!m_active)
      {
         rc = -1;
         break;
      }
   }
   LeaveCriticalSection(&m_bufferLock);
   return rc;
#else
   int rc = 0;
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
   return (rc == 0) ? 1 : 0;
#endif
}

/**
 * Background poll (dummy implementation)
 */
void TunnelCommChannel::backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*), void *context)
{
}

/**
 * Shutdown channel
 */
int TunnelCommChannel::shutdown()
{
   m_active = false;
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   WakeAllConditionVariable(&m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
   pthread_cond_broadcast(&m_dataCondition);
   pthread_mutex_unlock(&m_bufferLock);
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
   WakeAllConditionVariable(&m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
   pthread_mutex_unlock(&m_bufferLock);
#endif
}

/**
 * Configured tunnels
 */
static ObjectArray<Tunnel> s_tunnels(0, 8, Ownership::True);

#endif	/* _WITH_ENCRYPTION */

/**
 * Parser server connection (tunnel) list
 */
void ParseTunnelList(const StringSet& tunnels)
{
#ifdef _WITH_ENCRYPTION
   auto it = tunnels.begin();
   while(it.hasNext())
   {
      const TCHAR *config = it.next();
      Tunnel *t = Tunnel::createFromConfig(config);
      if (t != nullptr)
      {
         s_tunnels.add(t);
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Added server tunnel %s"), t->getHostname());
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Invalid server connection configuration record \"%s\""), config);
      }
   }
#endif
}

/**
 * Parser server connection (tunnel) list from separate configuration sections
 */
void ParseTunnelList(const ObjectArray<ConfigEntry>& config)
{
#ifdef _WITH_ENCRYPTION
   for (ConfigEntry *ce : config)
   {
      Tunnel *t = Tunnel::createFromConfig(*ce);
      if (t != nullptr)
      {
         s_tunnels.add(t);
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Added server tunnel %s"), t->getHostname());
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Invalid server connection configuration record \"%s\""), ce->getName());
      }
   }
#endif
}

/**
 * Tunnel manager
 */
void TunnelManager()
{
#ifdef _WITH_ENCRYPTION
   if (s_tunnels.size() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("No tunnels configured, tunnel manager will not start"));
      return;
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

   // Shutdown all running tunnels
   for(int i = 0; i < s_tunnels.size(); i++)
      s_tunnels.get(i)->disconnect();

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Tunnel manager stopped"));
#else
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Agent built without encryption support, tunnel manager will not start"));
#endif
}
