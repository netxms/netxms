/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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
**/

#include "netsvc.h"

/**
 * Connect to given host
 */
SOCKET NetConnectTCP(const char *hostname, const InetAddress& addr, uint16_t port, uint32_t timeout)
{
   char addrText[64];
   InetAddress hostAddr = (hostname != nullptr) ? InetAddress::resolveHostName(hostname) : addr;
   if (!hostAddr.isValidUnicast() && !hostAddr.isLoopback())
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NetConnectTCP(%hs:%d): invalid address"), (hostname != nullptr) ? hostname : addr.toStringA(addrText), port);
      return INVALID_SOCKET;
   }

   SOCKET s = ConnectToHost(hostAddr, port, (timeout != 0) ? timeout : g_netsvcTimeout);
   if (s == INVALID_SOCKET)
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NetConnectTCP(%hs:%d): connect failed (timeout %u ms)"), (hostname != nullptr) ? hostname : addr.toStringA(addrText), port, timeout);
   return s;
}

/**
 * Translate CURL error code into service check result code
 */
int CURLCodeToCheckResult(CURLcode cc)
{
   switch(cc)
   {
      case CURLE_OK:
         return PC_ERR_NONE;
      case CURLE_UNSUPPORTED_PROTOCOL:
      case CURLE_URL_MALFORMAT:
      case CURLE_LDAP_INVALID_URL:
#if HAVE_DECL_CURLE_NOT_BUILT_IN
      case CURLE_NOT_BUILT_IN:
#endif
#if HAVE_DECL_CURLE_UNKNOWN_OPTION
      case CURLE_UNKNOWN_OPTION:
#endif
         return PC_ERR_BAD_PARAMS;
      case CURLE_COULDNT_RESOLVE_PROXY:
      case CURLE_COULDNT_RESOLVE_HOST:
      case CURLE_COULDNT_CONNECT:
         return PC_ERR_CONNECT;
      default:
         return PC_ERR_HANDSHAKE;
   }
}

/**
 * Get timeout from metric arguments (used by legacy metrics)
 *
 * @return default timeout is returned if no timeout specified in args
 */
uint32_t GetTimeoutFromArgs(const TCHAR *metric, int argIndex)
{
   TCHAR timeoutText[64];
   if (!AgentGetParameterArg(metric, argIndex, timeoutText, 64))
      return g_netsvcTimeout;
   TCHAR* eptr;
   uint32_t timeout = _tcstol(timeoutText, &eptr, 0);
   return ((timeout != 0) && (*eptr == 0)) ? timeout : g_netsvcTimeout;
}

/**
 * Create URL parser
 */
URLParser::URLParser(const char *url)
{
#if HAVE_DECL_CURL_URL
   m_url = curl_url();
   m_valid = (curl_url_set(m_url, CURLUPART_URL, url, CURLU_NON_SUPPORT_SCHEME | CURLU_GUESS_SCHEME) == CURLUE_OK);
   m_scheme = nullptr;
#else
   m_url = MemCopyStringA(url);
   char *p = strchr(m_url, ':');
   if (p != nullptr)
   {
      m_scheme = m_url;
      *p = 0;
      p++;
      if ((*p == '/') && (*(p + 1) == '/'))
      {
         m_authority = p + 2;
         p = strchr(m_authority, '/');
         if (p != nullptr)
            *p = 0;
         strlwr(m_scheme);
         m_valid = true;
      }
      else
      {
         m_valid = false;
      }
   }
   else
   {
      m_valid = false;
   }
#endif
   m_host = nullptr;
   m_port = nullptr;
}

/**
 * URL parser destructor
 */
URLParser::~URLParser()
{
#if HAVE_DECL_CURL_URL
   curl_free(m_scheme);
   curl_free(m_host);
   curl_free(m_port);
   curl_url_cleanup(m_url);
#else
   MemFree(m_url);
#endif
}

/**
 * Extract schema
 */
const char *URLParser::scheme()
{
   if (!m_valid)
      return nullptr;

#if HAVE_DECL_CURL_URL
   if (m_scheme != nullptr)
      return m_scheme;

   if (curl_url_get(m_url, CURLUPART_SCHEME, &m_scheme, 0) != CURLUE_OK)
   {
      m_valid = false;
      return nullptr;
   }
#endif

   return m_scheme;
}

/**
 * Extract host part
 */
const char *URLParser::host()
{
   if (!m_valid)
      return nullptr;

   if (m_host != nullptr)
      return (m_host[0] == '[') ? &m_host[1] : m_host;

#if HAVE_DECL_CURL_URL
   if (curl_url_get(m_url, CURLUPART_HOST, &m_host, 0) != CURLUE_OK)
   {
      m_valid = false;
      return nullptr;
   }
   if (m_host[0] == '[')
   {
      // IPv6 address
      char *p = strchr(m_host, ']');
      if (p != nullptr)
         *p = 0;
   }
#else
   parseAuthority();
   if (!m_valid)
      return nullptr;
#endif

   return (m_host[0] == '[') ? &m_host[1] : m_host;
}

/**
 * Extract port part
 */
const char *URLParser::port()
{
   if (!m_valid)
      return nullptr;

   if (m_port != nullptr)
      return m_port;

#if HAVE_DECL_CURL_URL
   if (curl_url_get(m_url, CURLUPART_PORT, &m_port, CURLU_DEFAULT_PORT) != CURLUE_OK)
   {
      m_valid = false;
      return nullptr;
   }
#else
   parseAuthority();
   if (!m_valid)
      return nullptr;
#endif

   return m_port;
}

#if !HAVE_DECL_CURL_URL

/**
 * Parse authority part
 */
void URLParser::parseAuthority()
{
   char *p = strchr(m_authority, '@');
   if (p != nullptr)
      m_host = p + 1;
   else
      m_host = m_authority;

   if (m_host[0] == '[')
   {
      // IPv6 address
      m_host++;
      p = strchr(m_host, ']');
      if (p == nullptr)
      {
         m_valid = false;
         return;
      }
      *p = 0;
      p++;
      if (*p == ':')
         m_port = p + 1;
   }
   else
   {
      m_port = strchr(m_host, ':');
      if (m_port != nullptr)
      {
         *m_port = 0;
         m_port++;
      }
   }

   if (m_port == nullptr)
   {
      // Set default port
      m_port = m_defaultPort;
      if (!strcmp(m_scheme, "dict"))
         strcpy(m_port, "2628");
      else if (!strcmp(m_scheme, "ftp"))
         strcpy(m_port, "21");
      else if (!strcmp(m_scheme, "ftps"))
         strcpy(m_port, "990");
      else if (!strcmp(m_scheme, "gopher"))
         strcpy(m_port, "70");
      else if (!strcmp(m_scheme, "http"))
         strcpy(m_port, "80");
      else if (!strcmp(m_scheme, "https"))
         strcpy(m_port, "443");
      else if (!strcmp(m_scheme, "imap"))
         strcpy(m_port, "143");
      else if (!strcmp(m_scheme, "imaps"))
         strcpy(m_port, "993");
      else if (!strcmp(m_scheme, "ldap"))
         strcpy(m_port, "389");
      else if (!strcmp(m_scheme, "ldaps"))
         strcpy(m_port, "636");
      else if (!strcmp(m_scheme, "mqtt"))
         strcpy(m_port, "1883");
      else if (!strcmp(m_scheme, "pop3"))
         strcpy(m_port, "110");
      else if (!strcmp(m_scheme, "pop3s"))
         strcpy(m_port, "995");
      else if (!strcmp(m_scheme, "rtmp"))
         strcpy(m_port, "1935");
      else if (!strcmp(m_scheme, "rtsp"))
         strcpy(m_port, "554");
      else if (!strcmp(m_scheme, "smtp"))
         strcpy(m_port, "25");
      else if (!strcmp(m_scheme, "smtps"))
         strcpy(m_port, "465");
      else if (!strcmp(m_scheme, "ssh"))
         strcpy(m_port, "22");
      else if (!strcmp(m_scheme, "telnet"))
         strcpy(m_port, "23");
      else if (!strcmp(m_scheme, "tftp"))
         strcpy(m_port, "69");
      else
         m_valid = false;
   }
}

#endif
