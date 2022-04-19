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
** File: tls_conn.h
**
**/

#ifndef _tls_conn_h_
#define _tls_conn_h_

#include "nms_util.h"
#include <openssl/err.h>
#include <openssl/ssl.h>

// Default timeout for TLS handshake/send/recieve + SOCKET connect
#define TLS_CONN_DEFAULT_TIMEOUT 10000

/**
 * All things that are nessesary for TLS connection. Will automatically send/recieve data via TLS or without it.
 *
 * @note Destructor will close SSL connection and socket automatically.
 *
 * @note BY DEFAULT TIMEOUTS IN METHODS ARE NOT 0! They are equal to TLS_CONN_DEFAULT_TIMEOUT. This value can be changed by setDefaultTimeout() method.
 *
 * @note If you want to set timeout in some fuction to 0 you'll have to use setDefaultTimeout(0) first.
 */
class LIBNETXMS_EXPORTABLE TLSConnection
{
private:
   SOCKET m_socket;
   SSL* m_ssl;
   SSL_CTX* m_context;
   TCHAR m_debugTag[20];
   bool m_enableSSLTrace;
   uint32_t m_defaultTimeout;

   ssize_t tlsRecv(void* data, size_t size, uint32_t timeout);
   ssize_t tlsSend(const void* data, size_t size, uint32_t timeout);

public:
   /**
    * @param _debugTag will be copied.
    */
   TLSConnection(const TCHAR* debugTag, bool enableSSLTrace = false, uint32_t defaultTimeout = TLS_CONN_DEFAULT_TIMEOUT)
   {
      _tcslcpy(m_debugTag, CHECK_NULL_EX(debugTag), sizeof(m_debugTag) / sizeof(TCHAR));
      m_enableSSLTrace = enableSSLTrace;
      m_defaultTimeout = defaultTimeout;
      m_socket = INVALID_SOCKET;
      m_ssl = nullptr;
      m_context = nullptr;
   }

   /**
    * Destructor
    */
   ~TLSConnection()
   {
      stopTLS();
      shutdown(m_socket, SHUT_RDWR);
      closesocket(m_socket);
   }

   bool connect(const InetAddress& addr, uint16_t port, bool tls, uint32_t timeout = TLS_CONN_DEFAULT_TIMEOUT, const char *sniServerName = nullptr);
   bool startTLS(uint32_t timeout = 0, const char *sniServerName = nullptr);

   /**
    * Recieves data from socket considering if TLS is on or off.
    * @return number of bytes read on success, <= 0 on error.
    */
   ssize_t recv(void* data, size_t size, uint32_t timeout = 0)
   {
      if (timeout == 0)
         timeout = m_defaultTimeout;
      return isTLS() ? tlsRecv(data, size, timeout) : RecvEx(m_socket, data, size, 0, timeout);
   }

   /**
    * Sends data to socket considering is TLS is on or off.
    * @return number of bytes sent.
    */
   ssize_t send(const void* data, size_t size, uint32_t timeout = 0)
   {
      if (timeout == 0)
         timeout = m_defaultTimeout;
      return isTLS() ? tlsSend(data, size, timeout) : SendEx(m_socket, data, size, 0, nullptr);
   }

   /**
    * Poll socket for read rediness
    */
   bool poll(uint32_t timeout = 0)
   {
      SocketPoller sp;
      sp.add(m_socket);
      return sp.poll((timeout != 0) ? timeout : m_defaultTimeout) > 0;
   }

   /**
    * Closes a TLS connection.
    */
   void stopTLS()
   {
      SSL_free(m_ssl);
      SSL_CTX_free(m_context);
      m_ssl = nullptr;
      m_context = nullptr;
   }

   /**
    * Checks if TLS connection is established or not.
    * @return TRUE if TLS is connected.
    */
   bool isTLS()
   {
      return m_ssl != nullptr;
   }

   /**
    * Sets TLSConnection global timeout that will be used as default one. Overrides actual default values in methods' definitions.
    * @param timeout set to 0 to turn global timeout off.
    * @return TRUE if TLS is connected.
    */
   void setDefaultTimeout(uint32_t timeout)
   {
      m_defaultTimeout = timeout;
   }
};

#endif // _tls_conn_h_
