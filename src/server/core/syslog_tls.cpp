/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: syslog_tls.cpp
**
** Syslog over TLS (RFC 5425) listener. Accepts TLS connections on a
** dedicated port and feeds received messages into the same processing
** queue as the UDP syslog receiver. Both RFC 5425 octet-counting and
** RFC 6587 non-transparent (LF-delimited) framing are supported; the
** framing mode is auto-detected from the first byte of the stream.
**
**/

#include "nxcore.h"
#include <nxcore_syslog.h>
#include <socket_listener.h>

#define DEBUG_TAG L"syslog.tls"

/**
 * MSG-LEN in octet-counting mode is limited to this many digits (9,999,999);
 * value itself is further limited by MAX_ACCEPTED_MSG_LEN
 */
#define MAX_LENGTH_DIGITS     7

/**
 * Frames longer than MAX_SYSLOG_MSG_LEN but not exceeding this limit are read
 * and truncated (stream stays in sync); anything longer indicates a broken or
 * hostile peer and causes connection drop
 */
#define MAX_ACCEPTED_MSG_LEN  1048576

/**
 * Processing queue (syslogd.cpp)
 */
extern ObjectQueue<SyslogMessage> g_syslogProcessingQueue;

/**
 * Shutdown flag
 */
static std::atomic<bool> s_shutdown(false);

/**
 * Cached configuration
 */
static int s_tlsMinVersion = 2;
static bool s_requireClientCertificate = false;

/**
 * Convert internal TLS version code to OpenSSL version code
 */
static long DecodeTLSVersion(int version)
{
   long protoVersion = 0;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   switch(version)
   {
      case 0:
         protoVersion = TLS1_VERSION;
         break;
      case 1:
         protoVersion = TLS1_1_VERSION;
         break;
      case 2:
         protoVersion = TLS1_2_VERSION;
         break;
      case 3:
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
         protoVersion = TLS1_3_VERSION;
#endif
         break;
   }
#else
   switch(version)
   {
      case 3:
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
         protoVersion |= SSL_OP_NO_TLSv1_2;
#endif
         /* no break */
      case 2:
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
         protoVersion |= SSL_OP_NO_TLSv1_1;
#endif
         /* no break */
      case 1:
         protoVersion |= SSL_OP_NO_TLSv1;
         /* no break */
   }
#endif
   return protoVersion;
}

/**
 * Run TLS handshake with WANT_READ/WANT_WRITE retry on a non-blocking socket
 */
static bool DoTlsHandshake(SSL *ssl, SOCKET s, const wchar_t *peer)
{
   SetSocketNonBlocking(s);
   while(true)
   {
      int rc = SSL_do_handshake(ssl);
      if (rc == 1)
         return true;
      int sslErr = SSL_get_error(ssl, rc);
      if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
      {
         SocketPoller poller(sslErr == SSL_ERROR_WANT_WRITE);
         poller.add(s);
         if (poller.poll(10000) > 0)
            continue;
         nxlog_debug_tag(DEBUG_TAG, 4, L"TLS handshake timeout on syslog connection from %s", peer);
      }
      else
      {
         char buffer[128];
         nxlog_debug_tag(DEBUG_TAG, 4, L"TLS handshake failed on syslog connection from %s (%hs)", peer, ERR_error_string(ERR_get_error(), buffer));
         LogOpenSSLErrorStack(6);
      }
      return false;
   }
}

/**
 * Session - one TLS connection from a syslog sender. Sessions never delete
 * themselves; stopped sessions are reaped by the listener on next accept and
 * drained by StopSyslogTlsListener().
 */
class SyslogTlsSession
{
private:
   enum class FramingMode
   {
      UNKNOWN,
      OCTET_COUNTING,
      LF_DELIMITED
   };
   enum class OctetState
   {
      LENGTH,
      MESSAGE
   };

   SOCKET m_socket;
   SSL_CTX *m_context;
   SSL *m_ssl;
   InetAddress m_peer;
   wchar_t m_peerText[64];
   THREAD m_thread;
   std::atomic<bool> m_stopped;

   // Framing state (persists across SSL_read calls)
   FramingMode m_framing;
   OctetState m_octetState;
   size_t m_expectedLen;      // MSG-LEN of current frame
   size_t m_frameLen;         // bytes accumulated in m_frame
   size_t m_frameTotal;       // bytes consumed of current frame including truncated tail
   bool m_copyStopped;        // LF mode: line exceeds buffer, stop copying until LF
   char m_lengthDigits[MAX_LENGTH_DIGITS + 1];
   size_t m_lengthDigitCount;
   char m_frame[MAX_SYSLOG_MSG_LEN + 1];

   void readLoop();
   bool processData(const char *data, size_t len);
   void queueMessage();

public:
   SyslogTlsSession(SOCKET s, SSL_CTX *context, SSL *ssl, const InetAddress& peer);
   ~SyslogTlsSession();

   void start()
   {
      m_thread = ThreadCreateEx(this, &SyslogTlsSession::readLoop);
   }
   void terminate()
   {
      m_stopped = true;
      if (m_socket != INVALID_SOCKET)
         shutdown(m_socket, SHUT_RDWR);
   }
   void join()
   {
      ThreadJoin(m_thread);
      m_thread = INVALID_THREAD_HANDLE;
   }
   bool isStopped() const
   {
      return m_stopped.load();
   }
};

/**
 * Active sessions
 */
static Mutex s_sessionLock(MutexType::FAST);
static ObjectArray<SyslogTlsSession> s_sessions(0, 16, Ownership::False);

/**
 * Create session
 */
SyslogTlsSession::SyslogTlsSession(SOCKET s, SSL_CTX *context, SSL *ssl, const InetAddress& peer) : m_peer(peer)
{
   m_socket = s;
   m_context = context;
   m_ssl = ssl;
   peer.toString(m_peerText);
   m_thread = INVALID_THREAD_HANDLE;
   m_stopped = false;
   m_framing = FramingMode::UNKNOWN;
   m_octetState = OctetState::LENGTH;
   m_expectedLen = 0;
   m_frameLen = 0;
   m_frameTotal = 0;
   m_copyStopped = false;
   m_lengthDigitCount = 0;
}

/**
 * Destroy session
 */
SyslogTlsSession::~SyslogTlsSession()
{
   if (m_ssl != nullptr)
      SSL_free(m_ssl);
   if (m_context != nullptr)
      SSL_CTX_free(m_context);
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
}

/**
 * Session read loop (socket is non-blocking after handshake)
 */
void SyslogTlsSession::readLoop()
{
   ThreadSetName("SyslogTLS");
   char chunk[4096];
   while(!m_stopped.load() && !s_shutdown.load())
   {
      if (SSL_pending(m_ssl) == 0)
      {
         SocketPoller poller;
         poller.add(m_socket);
         int rc = poller.poll(1000);   // 1 second tick to re-check stop flags
         if (rc == 0)
            continue;
         if (rc < 0)
            break;
      }
      int bytes = SSL_read(m_ssl, chunk, sizeof(chunk));
      if (bytes <= 0)
      {
         int sslErr = SSL_get_error(m_ssl, bytes);
         if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
            continue;
         if (sslErr == SSL_ERROR_ZERO_RETURN)
            nxlog_debug_tag(DEBUG_TAG, 5, L"Syslog TLS connection from %s closed by peer", m_peerText);
         else
            nxlog_debug_tag(DEBUG_TAG, 5, L"Read error on syslog TLS connection from %s", m_peerText);
         break;
      }
      if (!processData(chunk, bytes))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Protocol error on syslog TLS connection from %s, closing connection", m_peerText);
         break;
      }
   }

   // Actively close the connection so the peer sees it immediately; resources
   // are released later when the session object is reaped
   SSL_shutdown(m_ssl);
   shutdown(m_socket, SHUT_RDWR);
   m_stopped = true;
}

/**
 * Process next portion of data from TLS stream. Handles partial frames and
 * multiple frames per read. Returns false on unrecoverable protocol error
 * (connection should be dropped).
 */
bool SyslogTlsSession::processData(const char *data, size_t len)
{
   if (m_framing == FramingMode::UNKNOWN)
   {
      // RFC 5425 frame starts with MSG-LEN (NONZERO-DIGIT *DIGIT); anything
      // else (normally '<' of PRI) indicates LF-delimited framing
      m_framing = ((data[0] >= '1') && (data[0] <= '9')) ? FramingMode::OCTET_COUNTING : FramingMode::LF_DELIMITED;
      nxlog_debug_tag(DEBUG_TAG, 5, L"Using %s framing for syslog TLS connection from %s",
            (m_framing == FramingMode::OCTET_COUNTING) ? L"octet-counting" : L"LF-delimited", m_peerText);
   }

   size_t i = 0;
   while(i < len)
   {
      if (m_framing == FramingMode::OCTET_COUNTING)
      {
         if (m_octetState == OctetState::LENGTH)
         {
            char c = data[i++];
            if ((c >= '0') && (c <= '9'))
            {
               if (m_lengthDigitCount >= MAX_LENGTH_DIGITS)
                  return false;
               m_lengthDigits[m_lengthDigitCount++] = c;
            }
            else if (c == ' ')
            {
               if (m_lengthDigitCount == 0)
                  return false;
               m_lengthDigits[m_lengthDigitCount] = 0;
               m_expectedLen = strtoul(m_lengthDigits, nullptr, 10);
               m_lengthDigitCount = 0;
               if ((m_expectedLen == 0) || (m_expectedLen > MAX_ACCEPTED_MSG_LEN))
                  return false;
               m_frameLen = 0;
               m_frameTotal = 0;
               m_octetState = OctetState::MESSAGE;
            }
            else
            {
               return false;
            }
         }
         else
         {
            // Frames longer than the buffer are truncated but consumed in full
            // to keep the stream in sync
            size_t take = std::min(len - i, m_expectedLen - m_frameTotal);
            size_t copy = std::min(take, MAX_SYSLOG_MSG_LEN - m_frameLen);
            memcpy(&m_frame[m_frameLen], &data[i], copy);
            m_frameLen += copy;
            m_frameTotal += take;
            i += take;
            if (m_frameTotal == m_expectedLen)
            {
               queueMessage();
               m_octetState = OctetState::LENGTH;
            }
         }
      }
      else
      {
         char c = data[i++];
         if (c == '\n')
         {
            if ((m_frameLen > 0) && (m_frame[m_frameLen - 1] == '\r'))
               m_frameLen--;
            if (m_frameLen > 0)
               queueMessage();
            m_frameLen = 0;
            m_copyStopped = false;
         }
         else if (!m_copyStopped)
         {
            if (m_frameLen < MAX_SYSLOG_MSG_LEN)
               m_frame[m_frameLen++] = c;
            else
               m_copyStopped = true;   // oversized line: keep truncated part, sync recovers at next LF
         }
      }
   }
   return true;
}

/**
 * Queue accumulated message for processing
 */
void SyslogTlsSession::queueMessage()
{
   m_frame[m_frameLen] = 0;
   g_syslogProcessingQueue.put(new SyslogMessage(m_peer, m_frame, m_frameLen));
   m_frameLen = 0;
}

/**
 * Listener for syslog over TLS connections
 */
class SyslogTlsListener : public StreamSocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer) override;
   virtual bool isStopConditionReached() override
   {
      return s_shutdown.load();
   }

public:
   SyslogTlsListener(uint16_t port) : StreamSocketListener(port)
   {
      setName(L"SyslogTLS");
   }
};

/**
 * Process incoming connection. TLS handshake runs inline on the listener
 * thread (senders are few long-lived relays, so a slow handshaker blocking
 * accepts for up to the 10 second handshake timeout is acceptable).
 */
ConnectionProcessingResult SyslogTlsListener::processConnection(SOCKET s, const InetAddress& peer)
{
   // Reap stopped sessions
   s_sessionLock.lock();
   for(int i = s_sessions.size() - 1; i >= 0; i--)
   {
      SyslogTlsSession *session = s_sessions.get(i);
      if (session->isStopped())
      {
         session->join();
         s_sessions.remove(i);
         delete session;
      }
   }
   s_sessionLock.unlock();

   wchar_t peerText[64];
   peer.toString(peerText);

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   if (method == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot obtain TLS method for syslog connection from %s", peerText);
      return CPR_COMPLETED;
   }

   SSL_CTX *context = SSL_CTX_new((SSL_METHOD *)method);
   if (context == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot create TLS context for syslog connection from %s", peerText);
      return CPR_COMPLETED;
   }
#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif

#if OPENSSL_VERSION_NUMBER < 0x10101000L
   if (s_tlsMinVersion >= 3)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot set minimal TLS version to 1.%d (not supported by server)", s_tlsMinVersion);
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   if (!SSL_CTX_set_min_proto_version(context, static_cast<int>(DecodeTLSVersion(s_tlsMinVersion))))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot set minimal TLS version to 1.%d", s_tlsMinVersion);
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }
#else
   SSL_CTX_set_options(context, SSL_CTX_get_options(context) | DecodeTLSVersion(s_tlsMinVersion));
#endif

   if (!SetupServerTlsContext(context))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot configure TLS context for syslog connection from %s", peerText);
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }

   SSL *ssl = SSL_new(context);
   if (ssl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot create TLS session object for syslog connection from %s", peerText);
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }

   SSL_set_accept_state(ssl);
   SSL_set_fd(ssl, (int)s);
   if (!DoTlsHandshake(ssl, s, peerText))
   {
      SSL_free(ssl);
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }

   if (s_requireClientCertificate)
   {
      // Chain validity of a presented certificate was already enforced against
      // the trust store during the handshake; only presence has to be checked
      X509 *cert = SSL_get_peer_certificate(ssl);
      if (cert == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"Dropping syslog TLS connection from %s: client certificate required but not provided", peerText);
         SSL_free(ssl);
         SSL_CTX_free(context);
         return CPR_COMPLETED;
      }
      X509_free(cert);
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"Accepted syslog TLS connection from %s", peerText);
   SyslogTlsSession *session = new SyslogTlsSession(s, context, ssl, peer);
   s_sessionLock.lock();
   s_sessions.add(session);
   s_sessionLock.unlock();
   session->start();
   return CPR_BACKGROUND;
}

/**
 * Listener thread
 */
static void SyslogTlsListenerThread()
{
   ThreadSetName("SyslogTlsLsnr");
   if (!IsServerCertificateLoaded())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Syslog TLS listener cannot be started because server certificate is not loaded");
      return;
   }

   uint16_t listenPort = static_cast<uint16_t>(ConfigReadULong(L"Syslog.TLS.ListenPort", 6514));
   s_tlsMinVersion = ConfigReadInt(L"Syslog.TLS.MinVersion", 2);
   s_requireClientCertificate = ConfigReadBoolean(L"Syslog.TLS.RequireClientCertificate", false);

   SyslogTlsListener listener(listenPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
      return;

   nxlog_debug_tag(DEBUG_TAG, 1, L"Syslog TLS listener started on port %u", listenPort);
   listener.mainLoop();
   listener.shutdown();
   nxlog_debug_tag(DEBUG_TAG, 1, L"Syslog TLS listener stopped");
}

/**
 * Listener thread handle
 */
static THREAD s_listenerThread = INVALID_THREAD_HANDLE;

/**
 * Start syslog over TLS listener
 */
void StartSyslogTlsListener()
{
   s_shutdown = false;
   s_listenerThread = ThreadCreateEx(SyslogTlsListenerThread);
}

/**
 * Stop syslog over TLS listener and all active sessions. Safe to call even
 * if the listener was never started.
 */
void StopSyslogTlsListener()
{
   s_shutdown = true;
   ThreadJoin(s_listenerThread);   // no new sessions can be registered past this point
   s_listenerThread = INVALID_THREAD_HANDLE;

   s_sessionLock.lock();
   for(int i = 0; i < s_sessions.size(); i++)
      s_sessions.get(i)->terminate();
   for(int i = 0; i < s_sessions.size(); i++)
   {
      SyslogTlsSession *session = s_sessions.get(i);
      session->join();
      delete session;
   }
   s_sessions.clear();
   s_sessionLock.unlock();
}
