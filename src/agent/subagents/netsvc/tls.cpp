/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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

static const TCHAR *startTlsTag = _T("netsvc.starttls");

/**
 * Send data once, await response within timeout and checks for "completion" with a supplied regex
 *
 * @param hSocket TCP socket connected to the surveyed application server
 * @param timeout Timeout in milliseconds
 * @param ourLine Data we send to initiate StartTLS
 * @param expectRegex Pattern which a complete response must match
 * @param descrTag description of the connection for log messages
 * @return True if successful, false otherwise
 */
static bool GenericStartTls(SOCKET hSocket, uint32_t timeout, const char *ourLine, const char *expectRegex, const TCHAR *descrTag)
{
   if (!NetWrite(hSocket, ourLine, strlen(ourLine)))
   {
      nxlog_debug_tag(startTlsTag, 9, _T("%s: Failed to send our line"), descrTag);
      return false;
   }

   int64_t startTime = GetCurrentTimeMs();
   int64_t deadlineTime = startTime + timeout;
   char buf[1024] = {0};
   char *ptr = buf;
   size_t bufAvail = sizeof(buf);

   while (true)
   {
      if (SocketCanRead(hSocket, timeout))
      {
         ssize_t bytesRead = NetRead(hSocket, ptr, bufAvail - 1);
         if (bytesRead <= 0)
         {
            nxlog_debug_tag(startTlsTag, 9, _T("%s: NetRead returned non-positive value %zd"), descrTag, bytesRead);
            return false;
         }
         assert(bytesRead > 0 && bytesRead <= bufAvail - 1);
         ptr += bytesRead;
         bufAvail -= bytesRead;
      }

      // check if the response is complete;
      if (RegexpMatchA(buf, expectRegex, false))
      {
         nxlog_debug_tag(startTlsTag, 7, _T("%s: Response complete, ready to start TLS"), descrTag);
         return true;
      }

      int64_t now = GetCurrentTimeMs();
      if (now >= deadlineTime)
      {
         nxlog_debug_tag(startTlsTag, 7, _T("%s: Timeout"), descrTag);
         return false;
      }
      timeout = static_cast<uint32_t>(deadlineTime - now);
   }
   return false;
}

/**
 * Perform protocol-specific STARTTLS sequence on a connected socket
 *
 * @param hSocket TCP socket connected to the surveyed application server
 * @param timeout Timeout in milliseconds
 * @param host    Surveyed application server hostname
 * @param proto   Application protocol string. Supported values: "smtp"
 * @param descrTag description of the connection for log messages
 * @return True if successful, false otherwise
 */
static bool SetupStartTLSSession(SOCKET hSocket, uint32_t timeout, const char *host, const char *proto, const TCHAR *descrTag)
{
// Newline regex: \r (which might be omitted), then \n
#define NL_RE "\\r?\\n"
   if (!strcmp(proto, "smtp"))
   {
      const char ourLine[] = "EHLO mail.example.com.\r\nSTARTTLS\r\n";
      const char expectRegex[] = NL_RE "220 .*" NL_RE;
      return GenericStartTls(hSocket, timeout, ourLine, expectRegex, descrTag);
   }
   else if (!strcmp(proto, "imap"))
   {
      const char ourLine[] = "cmd1 CAPABILITY\r\ncmd2 STARTTLS\r\n";
      const char expectRegex[] = NL_RE "cmd2 .*" NL_RE;
      return GenericStartTls(hSocket, timeout, ourLine, expectRegex, descrTag);
   }
   else if (!strcmp(proto, "pop3"))
   {
      const char ourLine[] = "STLS\r\n";
      const char expectRegex[] = "\\+OK .*" NL_RE "\\+OK .*" NL_RE;
      return GenericStartTls(hSocket, timeout, ourLine, expectRegex, descrTag);
   }
   nxlog_write_tag(NXLOG_ERROR, startTlsTag, _T("%s: unknown protocol %hs"), descrTag, proto);
   return false;
#undef NL_RE
}

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

   if (!AgentGetParameterArgA(parameters, 1, host, sizeof(host)) ||
       !AgentGetParameterArg(parameters, 2, portText, sizeof(portText) / sizeof(TCHAR)))
      return SYSINFO_RC_UNSUPPORTED;

   if (host[0] == 0 || portText[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   uint16_t port = static_cast<uint16_t>(_tcstol(portText, nullptr, 10));
   if (port == 0)
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;

   const OptionList options(parameters, 3);
   if (!options.isValid())
      return SYSINFO_RC_UNSUPPORTED;

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
   char startTlsInProto[5] = "";

   if (!AgentGetParameterArgA(parameters, 1, host, sizeof(host)) ||
       !AgentGetParameterArg(parameters, 2, portText, sizeof(portText) / sizeof(TCHAR)) ||
       !AgentGetParameterArgA(parameters, 3, sniServerName, sizeof(sniServerName)) ||
       !AgentGetParameterArgA(parameters, 4, startTlsInProto, sizeof(startTlsInProto)))
      return SYSINFO_RC_UNSUPPORTED;

   if (host[0] == 0 || portText[0] == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("H_TLSCertificateInfo: invalid host/port combination \"%hs\"/\"%s\""), host, portText);
      return SYSINFO_RC_UNSUPPORTED;
   }

   TCHAR *eptr;
   uint16_t port = static_cast<uint16_t>(_tcstol(portText, &eptr, 10));
   if ((port == 0) || (*eptr != 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("H_TLSCertificateInfo(%hs): invalid port number \"%s\""), host, portText);
      return SYSINFO_RC_UNSUPPORTED;
   }

   const OptionList options(parameters, 4);
   if (!options.isValid())
      return SYSINFO_RC_UNSUPPORTED;

   uint32_t timeout = options.getAsUInt32(_T("timeout"), g_netsvcTimeout);

   SOCKET hSocket = NetConnectTCP(host, InetAddress::INVALID, port, timeout);
   if (hSocket == INVALID_SOCKET)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("H_TLSCertificateInfo(%hs, %d): connection error"), host, port);
      return SYSINFO_RC_ERROR;
   }

   if (startTlsInProto[0] != '\0') {
      const char *serverName = (sniServerName[0] != 0) ? sniServerName : host;
      TCHAR descrTag[256];
      _sntprintf(descrTag, sizeof(descrTag), _T("%hs StartTLS %hs:%u"), startTlsInProto, serverName, port);

      bool startTlsSuccess = SetupStartTLSSession(hSocket, timeout, serverName, startTlsInProto, descrTag);
      if (!startTlsSuccess) {
         nxlog_debug_tag(startTlsTag, 7, _T("%s: StartTLS error"), descrTag);
         return SYSINFO_RC_ERROR;
      }
   }
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
