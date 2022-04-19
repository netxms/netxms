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

#include <tls_conn.h>
#include "libnetxms.h"

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
      result = startTLS(0, sniServerName);

   return result;
}

/**
 * Reads data from TLS connection.
 */
ssize_t TLSConnection::tlsRecv(void* data, const size_t size, const uint32_t timeout)
{
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
            SocketPoller sp(err == SSL_ERROR_WANT_READ);
            sp.add(m_socket);
            if (sp.poll(timeout) > 0)
               canRetry = true;
         }
         else
         {
            nxlog_debug_tag(m_debugTag, 7, _T("SSL_read error (bytes=%d ssl_err=%d socket_err=%d)"), bytes, err, WSAGetLastError());
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
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
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(timeout) > 0)
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

      // Setup secure connection
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
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif

   m_ssl = SSL_new(m_context);
   if (m_ssl == nullptr)
   {
      nxlog_debug_tag(m_debugTag, 4, _T("Cannot create SSL object"));
      goto failure;
   }

   if (sniServerName != nullptr)
   {
      nxlog_debug_tag(m_debugTag, 7, _T("Using SNI server name \"%hs\""), sniServerName);
      SSL_set_tlsext_host_name(m_ssl, sniServerName);
   }

   SSL_set_connect_state(m_ssl);
   SSL_set_fd(m_ssl, static_cast<int>(m_socket));

   while (true)
   {
      int rc = SSL_do_handshake(m_ssl);
      if (rc != 1)
      {
         int sslErr = SSL_get_error(m_ssl, rc);
         if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
         {
            SocketPoller poller(sslErr == SSL_ERROR_WANT_WRITE);
            poller.add(m_socket);
            if (poller.poll(timeout) > 0)
            {
               nxlog_debug_tag(m_debugTag, 8, _T("TLS handshake: %s wait completed"), (sslErr == SSL_ERROR_WANT_READ) ? _T("read") : _T("write"));
               continue;
            }
            nxlog_debug_tag(m_debugTag, 4, _T("TLS handshake failed (timeout on %s)"), (sslErr == SSL_ERROR_WANT_READ) ? _T("read") : _T("write"));
            goto failure;
         }
         else
         {
            char buffer[128];
            nxlog_debug_tag(m_debugTag, 4, _T("TLS handshake failed (%hs)"), ERR_error_string(sslErr, buffer));

            unsigned long error;
            while ((error = ERR_get_error()) != 0)
            {
               ERR_error_string_n(error, buffer, sizeof(buffer));
               nxlog_debug_tag(m_debugTag, 5, _T("Caused by: %hs"), buffer);
            }
            goto failure;
         }
      }
      break;
   }

   nxlog_debug_tag(m_debugTag, 7, _T("TLS handshake completed"));
   return true;

failure:
   stopTLS();  // Will destroy SSL context and SSL connection
   return false;
}
