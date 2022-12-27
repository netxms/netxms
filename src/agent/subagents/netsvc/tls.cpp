/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
** File: tls.cpp
**
**/

#include "netsvc.h"
#include <openssl/ssl.h>

/**
 * Setup TLS session and execute optional callback
 */
static bool SetupTLSSession(SOCKET hSocket, uint32_t timeout, const char *host, int port, std::function<bool(SSL_CTX*, SSL*)> callback)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   if (method == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("SetupTLSSession(%hs, %d): Cannot obtain TLS method"), host, port);
      return false;
   }

   SSL_CTX *context = SSL_CTX_new((SSL_METHOD *)method);
   if (context == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("SetupTLSSession(%hs, %d): Cannot create TLS context"), host, port);
      return false;
   }

   SSL *ssl = SSL_new(context);
   if (ssl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("SetupTLSSession(%hs, %d): Cannot create session object"), host, port);
      SSL_CTX_free(context);
      return false;
   }

   SSL_set_tlsext_host_name(ssl, host);
   SSL_set_connect_state(ssl);
   SSL_set_fd(ssl, (int)hSocket);

   bool success = true;
   while (true)
   {
      int rc = SSL_do_handshake(ssl);
      if (rc != 1)
      {
         int sslErr = SSL_get_error(ssl, rc);
         if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
         {
            SocketPoller poller(sslErr == SSL_ERROR_WANT_WRITE);
            poller.add(hSocket);
            if (poller.poll(timeout) > 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 7, _T("SetupTLSSession(%hs, %d): TLS handshake: %s wait completed"),
                               host, port, (sslErr == SSL_ERROR_WANT_READ) ? _T("read") : _T("write"));
               continue;
            }
            nxlog_debug_tag(DEBUG_TAG, 7, _T("SetupTLSSession(%hs, %d): TLS handshake failed (timeout on %s)"),
                            host, port, (sslErr == SSL_ERROR_WANT_READ) ? _T("read") : _T("write"));
            success = false;
            break;
         }
         else
         {
            char buffer[128];
            nxlog_debug_tag(DEBUG_TAG, 7, _T("SetupTLSSession(%hs, %d): TLS handshake failed (%hs)"), host, port, ERR_error_string(sslErr, buffer));

            unsigned long error;
            while ((error = ERR_get_error()) != 0)
            {
               ERR_error_string_n(error, buffer, sizeof(buffer));
               nxlog_debug_tag(DEBUG_TAG, 7, _T("SetupTLSSession(%hs, %d): caused by: %hs"), host, port, buffer);
            }
            success = false;
            break;
         }
      }
      break;
   }

   if (success && (callback != nullptr))
      success = callback(context, ssl);

   SSL_free(ssl);
   SSL_CTX_free(context);

   return success;
}

/**
 * Check TLS service
 */
int CheckTLS(const char *hostname, const InetAddress &addr, uint16_t port, uint32_t timeout)
{
   int status;
   char buffer[64];
   SOCKET hSocket = NetConnectTCP(hostname, addr, port, timeout);
   if (hSocket != INVALID_SOCKET)
   {
      status = SetupTLSSession(hSocket, timeout, (hostname != nullptr) ? hostname : addr.toStringA(buffer), port, nullptr) ? PC_ERR_NONE : PC_ERR_HANDSHAKE;
      NetClose(hSocket);
   }
   else
   {
      status = PC_ERR_CONNECT;
   }
   nxlog_debug_tag(DEBUG_TAG, 7, _T("CheckTLS(%hs, %d): result=%d"), (hostname != nullptr) ? hostname : addr.toStringA(buffer), (int)port, status);
   return status;
}

/**
 * Check TLS service - parameter handler
 */
LONG H_CheckTLS(const TCHAR *parameters, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char host[1024];
   TCHAR portText[32];
   AgentGetParameterArgA(parameters, 1, host, sizeof(host));
   AgentGetParameterArg(parameters, 2, portText, sizeof(portText) / sizeof(TCHAR));

   if (host[0] == 0 || portText[0] == 0)
      return SYSINFO_RC_ERROR;

   uint16_t port = static_cast<uint16_t>(_tcstol(portText, nullptr, 10));
   if (port == 0)
      return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_SUCCESS;

   const OptionList options(parameters, 3);
   uint32_t timeout = options.getAsUInt32(_T("timeout"), g_netsvcTimeout);

   int64_t start = GetCurrentTimeMs();
   int result = CheckTLS(host, InetAddress::INVALID, port, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_netsvcFlags & NETSVC_AF_NEGATIVE_TIME_ON_ERROR)
         ret_int64(value, -(GetCurrentTimeMs() - start));
      else
         rc = SYSINFO_RC_ERROR;
   }
   else
   {
      ret_int(value, result);
   }
   return rc;
}

/**
 * Get expiration date for certificate
 */
static inline String GetCertificateExpirationDate(X509 *cert)
{
   time_t e = GetCertificateExpirationTime(cert);
   TCHAR buffer[64];
   _tcsftime(buffer, 64, _T("%Y-%m-%d"), localtime(&e));
   return String(buffer);
}

/**
 * Get number of days until certificate expiration
 */
static inline int GetCertificateDaysUntilExpiration(X509 *cert)
{
   time_t e = GetCertificateExpirationTime(cert);
   time_t now = time(nullptr);
   return static_cast<int>((e - now) / 86400);
}

/**
 * TLS certificate information
 */
LONG H_TLSCertificateInfo(const TCHAR *parameters, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char host[1024], sniServerName[1024];
   TCHAR portText[32];
   AgentGetParameterArgA(parameters, 1, host, sizeof(host));
   AgentGetParameterArg(parameters, 2, portText, sizeof(portText) / sizeof(TCHAR));
   AgentGetParameterArgA(parameters, 3, sniServerName, sizeof(sniServerName));

   if (host[0] == 0 || portText[0] == 0)
      return SYSINFO_RC_ERROR;

   uint16_t port = static_cast<uint16_t>(_tcstol(portText, nullptr, 10));
   if (port == 0)
      return SYSINFO_RC_ERROR;

   const OptionList options(parameters, 4);
   uint32_t timeout = options.getAsUInt32(_T("timeout"), g_netsvcTimeout);

   SOCKET hSocket = NetConnectTCP(host, InetAddress::INVALID, port, timeout);
   if (hSocket == INVALID_SOCKET)
      return SYSINFO_RC_ERROR;

   bool success = SetupTLSSession(hSocket, timeout, (sniServerName[0] != 0) ? sniServerName : host, port,
       [host, port, arg, value](SSL_CTX *context, SSL *ssl) -> bool
       {
          X509 *cert = SSL_get_peer_certificate(ssl);
          if (cert == nullptr)
          {
             nxlog_debug_tag(DEBUG_TAG, 7, _T("H_TLSCertificateInfo(%hs, %d): server certificate not provided"), host, port);
             return false;
          }

          bool success = true;
          switch (*arg)
          {
          case 'D': // Expiration date
             ret_string(value, GetCertificateExpirationDate(cert));
             break;
          case 'E': // Expiration time
             ret_uint64(value, GetCertificateExpirationTime(cert));
             break;
          case 'I': // Issuer
             ret_string(value, GetCertificateIssuerString(cert));
             break;
          case 'S': // Subject
             ret_string(value, GetCertificateSubjectString(cert));
             break;
          case 'T': // Template ID
             ret_string(value, GetCertificateTemplateId(cert));
             break;
          case 'U': // Days until expiration
             ret_int(value, GetCertificateDaysUntilExpiration(cert));
             break;
          }
          X509_free(cert);
          return success;
       });
   NetClose(hSocket);

   return success ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}
