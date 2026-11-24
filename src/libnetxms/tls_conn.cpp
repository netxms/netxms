/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
** File: tls_conn.cpp
**
**/

#include "libnetxms.h"
#include <tls_conn.h>

/**
 * Opens a TLS connection. (First function to be used upon object creation.)
 * @param tls specifies if TLS connection should be established at once.
 * It is possible to connect TLS later by startTLS method.
 * @return TRUE if connection was successfull.
 */
bool TLSConnection::connect(const InetAddress& addr, uint16_t port, bool tls, const uint32_t timeout, const char *sniServerName)
{
   m_socket = ConnectToHost(addr, port, timeout);

   bool result = m_socket != INVALID_SOCKET;

   if (result && tls)
      result = startTLS(timeout, sniServerName);

   return result;
}

/**
 * Reads data from TLS connection.
 * @return number of bytes read on success, 0 if connection was closed, -1 on error, -2 on timeout
 */
ssize_t TLSConnection::tlsRecv(void* data, const size_t size, const uint32_t timeout)
{
   int64_t deadline = GetMonotonicClockTime() + timeout;
   bool canRetry;
   ssize_t bytes;
   do
   {
      canRetry = false;
      bytes = SSL_read(m_ssl, data, static_cast<int>(size));
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, static_cast<int>(bytes));
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            int64_t remaining = deadline - GetMonotonicClockTime();
            if (remaining <= 0)
            {
               bytes = -2;
               break;
            }
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(static_cast<uint32_t>(remaining)) > 0)
               canRetry = true;
            else
               bytes = -2;
         }
         else if (err == SSL_ERROR_ZERO_RETURN)
         {
            bytes = 0;
         }
         else
         {
            nxlog_debug_tag(m_debugTag, 7, _T("SSL_read error (bytes=%d ssl_err=%d socket_err=%d)"), bytes, err, WSAGetLastError());
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
            bytes = -1;
         }
      }
   }
   while (canRetry);
   return bytes;
}

/**
 * Writes data to TLS connection.
 */
ssize_t TLSConnection::tlsSend(const void* data, const size_t size, const uint32_t timeout)
{
   int64_t deadline = GetMonotonicClockTime() + timeout;
   bool canRetry;
   ssize_t bytes;
   do
   {
      canRetry = false;
      bytes = SSL_write(m_ssl, data, static_cast<int>(size));
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, static_cast<int>(bytes));
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            int64_t remaining = deadline - GetMonotonicClockTime();
            if (remaining <= 0)
               break;
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(static_cast<uint32_t>(remaining)) > 0)
               canRetry = true;
         }
         else
         {
            nxlog_debug_tag(m_debugTag, 7, _T("SSL_write error (bytes=%d ssl_err=%d socket_err=%d)"), bytes, err, WSAGetLastError());
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
         }
      }
   }
   while (canRetry);
   return bytes;
}

#define TLS_DEBUG_TAG _T("ssl")

/**
 * SSL message callback. Will be called each time as something happens if enabled.
 */
static void SSLInfoCallback(const SSL* ssl, int where, int ret)
{
   if (where & SSL_CB_ALERT)
   {
      nxlog_debug_tag(TLS_DEBUG_TAG, 4, _T("SSL %s alert: %hs (%hs)"), (where & SSL_CB_READ) ? _T("read") : _T("write"),
                      SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
   }
   else if (where & SSL_CB_HANDSHAKE_START)
   {
      nxlog_debug_tag(TLS_DEBUG_TAG, 6, _T("SSL handshake start (%hs)"), SSL_state_string_long(ssl));
   }
   else if (where & SSL_CB_HANDSHAKE_DONE)
   {
      nxlog_debug_tag(TLS_DEBUG_TAG, 6, _T("SSL handshake done (%hs)"), SSL_state_string_long(ssl));
   }
   else
   {
      int method = where & ~SSL_ST_MASK;
      const TCHAR* prefix;
      if (method & SSL_ST_CONNECT)
         prefix = _T("SSL_connect");
      else if (method & SSL_ST_ACCEPT)
         prefix = _T("SSL_accept");
      else
         prefix = _T("undefined");

      if (where & SSL_CB_LOOP)
      {
         nxlog_debug_tag(TLS_DEBUG_TAG, 6, _T("%s: %hs"), prefix, SSL_state_string_long(ssl));
      }
      else if (where & SSL_CB_EXIT)
      {
         if (ret == 0)
            nxlog_debug_tag(TLS_DEBUG_TAG, 3, _T("%s: failed in %hs"), prefix, SSL_state_string_long(ssl));
         else if (ret < 0)
            nxlog_debug_tag(TLS_DEBUG_TAG, 3, _T("%s: error in %hs"), prefix, SSL_state_string_long(ssl));
      }
   }
}

/**
 * Opens a TLS connection.
 * @return TRUE if connection was successfull.
 */
bool TLSConnection::startTLS(uint32_t timeout, const char *sniServerName)
{
   if (m_socket == INVALID_SOCKET)
   {
      nxlog_debug_tag(m_debugTag, 4, _T("Connection socket is invalid"));
      return false;
   }

   if (isTLS())
      return false;

   if (timeout == 0)
      timeout = m_defaultTimeout;

   int64_t deadline = GetMonotonicClockTime() + timeout;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD* method = TLS_method();
#else
   const SSL_METHOD* method = SSLv23_method();
#endif

   if (method == nullptr)
   {
      nxlog_debug_tag(m_debugTag, 4, _T("Cannot obtain TLS method"));
      goto failure;
   }

   m_context = SSL_CTX_new((SSL_METHOD*)method);
   if (m_context == nullptr)
   {
      nxlog_debug_tag(m_debugTag, 4, _T("Cannot create TLS context"));
      goto failure;
   }

   if (m_enableSSLTrace)
      SSL_CTX_set_info_callback(m_context, SSLInfoCallback);

#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif

   if (m_verifyPeer)
   {
      if (m_trustedCertificatesFile != nullptr)
      {
         if (SSL_CTX_load_verify_locations(m_context, m_trustedCertificatesFile, nullptr) != 1)
         {
            nxlog_debug_tag(m_debugTag, 4, _T("Cannot load trusted certificates from file \"%hs\""), m_trustedCertificatesFile);
            LogOpenSSLErrorStack(4);
            goto failure;
         }
      }
      else
      {
         SSL_CTX_set_default_verify_paths(m_context);
      }
      SSL_CTX_set_verify(m_context, SSL_VERIFY_PEER, nullptr);
   }

   m_ssl = SSL_new(m_context);
   if (m_ssl == nullptr)
   {
      nxlog_debug_tag(m_debugTag, 4, _T("Cannot create SSL object"));
      goto failure;
   }

   if (sniServerName != nullptr)
   {
      struct in_addr ipv4;
      struct in6_addr ipv6;
      bool isIP = (inet_pton(AF_INET, sniServerName, &ipv4) == 1) || (inet_pton(AF_INET6, sniServerName, &ipv6) == 1);

      if (!isIP)
      {
         nxlog_debug_tag(m_debugTag, 7, _T("Using SNI server name \"%hs\""), sniServerName);
         SSL_set_tlsext_host_name(m_ssl, sniServerName);
      }

      if (m_verifyPeer)
      {
         if (isIP)
         {
            X509_VERIFY_PARAM *param = SSL_get0_param(m_ssl);
            X509_VERIFY_PARAM_set1_ip_asc(param, sniServerName);
         }
         else
         {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
            SSL_set1_host(m_ssl, sniServerName);
#else
            X509_VERIFY_PARAM *param = SSL_get0_param(m_ssl);
            X509_VERIFY_PARAM_set1_host(param, sniServerName, 0);
#endif
         }
      }
   }

   SSL_set_connect_state(m_ssl);
   SSL_set_fd(m_ssl, static_cast<int>(m_socket));

   {
      int64_t remaining = deadline - GetMonotonicClockTime();
      if (PerformTLSHandshake(m_ssl, m_socket, (remaining > 0) ? static_cast<uint32_t>(remaining) : 0, m_debugTag, 4, _T("TLSConnection::startTLS")) != TLSHandshakeResult::SUCCESS)
         goto failure;
   }

   nxlog_debug_tag(m_debugTag, 7, _T("TLS handshake completed"));
   return true;

failure:
   stopTLS();
   return false;
}

/**
 * Perform TLS handshake on given socket. SSL object should be already bound to the socket and set to connect or accept state.
 * Socket is switched to non-blocking mode and left in that mode. Timeout is applied to the whole handshake. Failures are logged
 * with given debug tag and level (OpenSSL error details at the next level) with given context string as prefix.
 *
 * @param ssl SSL object
 * @param s socket
 * @param timeout handshake timeout in milliseconds
 * @param debugTag debug tag for log messages
 * @param debugLevel debug level for failure messages
 * @param context context string used as prefix for log messages
 * @param errorText optional buffer for error message
 * @param errorTextSize size of error message buffer in characters
 * @return handshake result
 */
TLSHandshakeResult LIBNETXMS_EXPORTABLE PerformTLSHandshake(SSL *ssl, SOCKET s, uint32_t timeout, const TCHAR *debugTag, int debugLevel,
         const TCHAR *context, TCHAR *errorText, size_t errorTextSize)
{
   SetSocketNonBlocking(s);

   int64_t deadline = GetMonotonicClockTime() + timeout;
   while(true)
   {
      int rc = SSL_do_handshake(ssl);
      if (rc == 1)
         return TLSHandshakeResult::SUCCESS;

      uint32_t socketError = WSAGetLastError();
      int sslErr = SSL_get_error(ssl, rc);
      TCHAR text[512];
      if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
      {
         int pollResult = 0;
         int64_t remaining = deadline - GetMonotonicClockTime();
         if (remaining > 0)
         {
            SocketPoller poller(sslErr == SSL_ERROR_WANT_WRITE);
            poller.add(s);
            pollResult = poller.poll(static_cast<uint32_t>(remaining));
            if (pollResult > 0)
               continue;
            if ((pollResult < 0) && (WSAGetLastError() == WSAEINTR))
               continue;   // Interrupted wait, deadline will be checked again on next iteration
         }
         if (pollResult == 0)
         {
            nxlog_debug_tag(debugTag, debugLevel, _T("%s: TLS handshake failed (timeout on %s)"), context, (sslErr == SSL_ERROR_WANT_READ) ? _T("read") : _T("write"));
            if (errorText != nullptr)
               _tcslcpy(errorText, _T("TLS handshake failed (timeout)"), errorTextSize);
            return TLSHandshakeResult::TIMEOUT;
         }

         TCHAR buffer[256];
         _sntprintf(text, 512, _T("TLS handshake failed (socket poll error: %s)"), GetLastSocketErrorText(buffer, 256));
      }
      else if (sslErr == SSL_ERROR_SYSCALL)
      {
         if (socketError != 0)
         {
            TCHAR buffer[256];
            _sntprintf(text, 512, _T("TLS handshake failed (system error: %s)"), GetSocketErrorText(socketError, buffer, 256));
         }
         else
         {
            _tcslcpy(text, _T("TLS handshake failed (connection closed by peer)"), 512);
         }
      }
      else
      {
         unsigned long error = ERR_get_error();
         if (error != 0)
         {
            char buffer[256];
            ERR_error_string_n(error, buffer, sizeof(buffer));
            _sntprintf(text, 512, _T("TLS handshake failed (ssl_err=%d: %hs)"), sslErr, buffer);
         }
         else
         {
            _sntprintf(text, 512, _T("TLS handshake failed (ssl_err=%d)"), sslErr);
         }
      }
      nxlog_debug_tag(debugTag, debugLevel, _T("%s: %s"), context, text);

      // Always drain OpenSSL error queue so that stale errors do not affect later TLS operations in this thread
      unsigned long error;
      while((error = ERR_get_error()) != 0)
      {
         char buffer[256];
         ERR_error_string_n(error, buffer, sizeof(buffer));
         nxlog_debug_tag(debugTag, debugLevel + 1, _T("%s: caused by: %hs"), context, buffer);
      }

      if (errorText != nullptr)
         _tcslcpy(errorText, text, errorTextSize);
      return TLSHandshakeResult::FAILURE;
   }
}
