/**
 * This code is heavily based on "pam_cas" module from "cas-client-2.0.11" package.
 *
 * -- Alex Kirhenshtein <alk@netxms.org>
 */

/*
 *  Copyright (c) 2000-2003 Yale University. All rights reserved.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS," AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE EXPRESSLY
 *  DISCLAIMED. IN NO EVENT SHALL YALE UNIVERSITY OR ITS EMPLOYEES BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED, THE COSTS OF
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED IN ADVANCE OF THE POSSIBILITY OF SUCH
 *  DAMAGE.
 *
 *  Redistribution and use of this software in source or binary forms,
 *  with or without modification, are permitted, provided that the
 *  following conditions are met:
 *
 *  1. Any redistribution must include the above copyright notice and
 *  disclaimer and this list of conditions in any related documentation
 *  and, if feasible, in the redistributed software.
 *
 *  2. Any redistribution must include the acknowledgment, "This product
 *  includes software developed by Yale University," in any related
 *  documentation and, if feasible, in the redistributed software.
 *
 *  3. The names "Yale" and "Yale University" must not be used to endorse
 *  or promote products derived from this software.
 */

/*
 * CAS 2.0 service- and proxy-ticket validator in C, using OpenSSL.
 *
 * Originally by Shawn Bayern, Yale ITS Technology and Planning.
 * Patches submitted by Vincent Mathieu, University of Nancy, France.
 */

#include "nxcore.h"
#include <tls_conn.h>

#define DEBUG_TAG L"cas"

/**
 * Timeout for connection to CAS server, TLS handshake, and complete validation response (milliseconds)
 */
#define CAS_REQUEST_TIMEOUT   10000

/**
 * Characters that can appear in URL query parameter value without encoding (RFC 3986 unreserved set)
 */
#define URL_SAFE_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._~-"

/**
 * Settings
 */
static char s_hostname[MAX_DNS_NAME] = "localhost";
static uint16_t s_port = 8443;
static char s_service[MAX_CONFIG_VALUE_LENGTH * 3] = "";  // URL-encoded
static char s_trustedCA[MAX_PATH] = "";
static char s_validateURL[MAX_CONFIG_VALUE_LENGTH] = "/cas/serviceValidate";
static StringSet s_proxies;
static Mutex s_lock;

/**
 * Check if string can be inserted into HTTP request line or header as is (contains no whitespace or control characters)
 */
static bool IsValidRequestElement(const char *s)
{
   for(const char *p = s; *p != 0; p++)
   {
      if ((static_cast<unsigned char>(*p) <= ' ') || (*p == 0x7F))
         return false;
   }
   return true;
}

/**
 * Read CAS settings
 */
void CASReadSettings()
{
   s_lock.lock();

   ConfigReadStrA(L"CAS.Host", s_hostname, MAX_DNS_NAME, "localhost");
   if (!IsValidRequestElement(s_hostname))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Value of configuration parameter CAS.Host contains whitespace or control characters, CAS authentication disabled");
      s_hostname[0] = 0;
   }

   int port = ConfigReadInt(L"CAS.Port", 8443);
   if ((port < 1) || (port > 65535))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Invalid value %d of configuration parameter CAS.Port, using default port 8443", port);
      port = 8443;
   }
   s_port = static_cast<uint16_t>(port);

   // CAS protocol requires service to be URL-encoded. Value that consists only of unreserved characters
   // and percent signs is either already encoded or does not need encoding, and is used as is.
   char service[MAX_CONFIG_VALUE_LENGTH];
   ConfigReadStrA(L"CAS.Service", service, MAX_CONFIG_VALUE_LENGTH, "http://127.0.0.1:10080/nxmc");
   if (strspn(service, URL_SAFE_CHARS "%") == strlen(service))
      strlcpy(s_service, service, sizeof(s_service));
   else
      URLEncode(service, s_service, sizeof(s_service));

   ConfigReadStrA(L"CAS.TrustedCACert", s_trustedCA, MAX_PATH, "");

   ConfigReadStrA(L"CAS.ValidateURL", s_validateURL, MAX_CONFIG_VALUE_LENGTH, "/cas/serviceValidate");
   if (!IsValidRequestElement(s_validateURL))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Value of configuration parameter CAS.ValidateURL contains whitespace or control characters, CAS authentication disabled");
      s_validateURL[0] = 0;
   }

   wchar_t proxies[MAX_CONFIG_VALUE_LENGTH];
   ConfigReadStr(L"CAS.AllowedProxies", proxies, MAX_CONFIG_VALUE_LENGTH, L"");
   s_proxies.clear();
   String::split(proxies, L",", true,
      [] (const String& proxy) -> void
      {
         if (!proxy.isEmpty())
            s_proxies.add(proxy);
      });

   s_lock.unlock();
   nxlog_debug_tag(DEBUG_TAG, 4, L"CAS configuration reloaded");
}

/* Error codes (decided upon by ESUP-Portail group) */
#define CAS_SUCCESS                 0
#define CAS_AUTHENTICATION_FAILURE -1

#define CAS_SSL_ERROR_CONN         -11
#define CAS_SSL_ERROR_HTTPS        -13

#define CAS_ERROR_CONN             -20
#define CAS_PROTOCOL_FAILURE       -21
#define CAS_BAD_PROXY              -22
#define CAS_ERROR_TIMEOUT          -23

/**
 * Find body of first element with given name in XML document. Returns pointer to the beginning of element body
 * (not null-terminated) and sets length to body length, or returns nullptr if element not found.
 */
static const char *FindElementBody(const char *doc, const char *tagName, size_t *length)
{
   char startTag[64], endTag[64];
   snprintf(startTag, sizeof(startTag), "<%s>", tagName);
   snprintf(endTag, sizeof(endTag), "</%s>", tagName);

   const char *start = strstr(doc, startTag);
   if (start == nullptr)
      return nullptr;
   start += strlen(startTag);

   const char *end = strstr(start, endTag);
   if (end == nullptr)
      return nullptr;

   *length = end - start;
   return start;
}

/**
 * Send validation request to CAS server and parse response. On success fills loginName with user name
 * from CAS response and returns CAS_SUCCESS. Otherwise returns error code.
 */
static int ValidateTicket(const char *hostname, uint16_t port, const char *trustedCA, const char *request, wchar_t *loginName)
{
   InetAddress addr = InetAddress::resolveHostName(hostname);
   if (!addr.isValid())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: cannot resolve host name %hs", hostname);
      return CAS_ERROR_CONN;
   }

   TLSConnection conn(DEBUG_TAG, false, CAS_REQUEST_TIMEOUT);
   if (trustedCA[0] != 0)
      conn.enablePeerVerification(trustedCA);
   else
      conn.enablePeerVerification();
   if (!conn.connect(addr, port, false, CAS_REQUEST_TIMEOUT))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: cannot connect to %hs:%u", hostname, static_cast<unsigned int>(port));
      return CAS_ERROR_CONN;
   }
   if (!conn.startTLS(CAS_REQUEST_TIMEOUT, hostname))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: TLS handshake with %hs:%u failed (TLS error or server certificate verification failure)", hostname, static_cast<unsigned int>(port));
      return CAS_SSL_ERROR_CONN;
   }

   size_t requestLength = strlen(request);
   if (conn.send(request, requestLength) != static_cast<ssize_t>(requestLength))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: cannot send validation request");
      return CAS_SSL_ERROR_HTTPS;
   }

   // Server closes connection after sending response (HTTP/1.0)
   char response[4096];
   size_t total = 0;
   int64_t deadline = GetMonotonicClockTime() + CAS_REQUEST_TIMEOUT;
   while(true)
   {
      if (total == sizeof(response) - 1)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: validation response is too large");
         return CAS_SSL_ERROR_HTTPS;
      }

      int64_t remaining = deadline - GetMonotonicClockTime();
      ssize_t bytes = (remaining > 0) ? conn.recv(response + total, sizeof(response) - 1 - total, static_cast<uint32_t>(remaining)) : -2;
      if (bytes == -2)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: timeout waiting for validation response");
         return CAS_ERROR_TIMEOUT;
      }
      if (bytes <= 0)
      {
         // 0 means TLS close notification, -1 is either read error or connection closed without close notification.
         // Response is checked for completeness below.
         if (bytes < 0)
            nxlog_debug_tag(DEBUG_TAG, 6, L"CAS: connection closed without TLS close notification or read error");
         break;
      }
      total += static_cast<size_t>(bytes);
   }
   response[total] = 0;

   const char *body = strstr(response, "\r\n\r\n");  // find the end of the header
   if (body == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: invalid HTTP response from server");
      return CAS_SSL_ERROR_HTTPS;
   }
   body += 4;

   // Response truncated by connection reset or read error would not have closing tag
   if (strstr(body, "</cas:serviceResponse>") == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: validation response is incomplete or invalid");
      return CAS_PROTOCOL_FAILURE;
   }

   size_t length;
   if (FindElementBody(body, "cas:authenticationSuccess", &length) == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: authentication failure");
      return CAS_AUTHENTICATION_FAILURE;
   }

   // retrieve the NetID
   const char *user = FindElementBody(body, "cas:user", &length);
   if (user == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: unable to determine user name");
      return CAS_PROTOCOL_FAILURE;
   }
   if ((length == 0) || (utf8_wcharlen(user, length) >= MAX_USER_NAME))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: user name in validation response is empty or too long");
      return CAS_PROTOCOL_FAILURE;
   }
   size_t chars = utf8_to_wchar(user, length, loginName, MAX_USER_NAME - 1);
   loginName[chars] = 0;

   // check the first proxy (if present)
   const char *proxies = FindElementBody(body, "cas:proxies", &length);
   if (proxies != nullptr)
   {
      const char *proxy = FindElementBody(proxies, "cas:proxy", &length);
      if (proxy != nullptr)
      {
         if (length >= MAX_DNS_NAME)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: proxy name in validation response is too long");
            return CAS_BAD_PROXY;
         }

         wchar_t wproxy[MAX_DNS_NAME];
         chars = utf8_to_wchar(proxy, length, wproxy, MAX_DNS_NAME - 1);
         wproxy[chars] = 0;

         s_lock.lock();
         bool allowed = s_proxies.contains(wproxy);
         s_lock.unlock();
         if (!allowed)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: proxy %s is not in allowed proxies list", wproxy);
            return CAS_BAD_PROXY;
         }
      }
   }

   return CAS_SUCCESS;
}

/**
 * Authenticate user via CAS
 */
bool CASAuthenticate(const char *ticket, wchar_t *loginName)
{
   // Ticket is inserted into validation request URL as is, so only URL-safe characters are accepted
   if ((*ticket == 0) || (strspn(ticket, URL_SAFE_CHARS) != strlen(ticket)))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: ticket is empty or contains invalid characters");
      return false;
   }

   // Take snapshot of settings and build request under lock, and do network I/O without holding it
   s_lock.lock();
   if ((s_hostname[0] == 0) || (s_validateURL[0] == 0))
   {
      s_lock.unlock();
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: server host name or validation URL is not configured or invalid");
      return false;
   }
   char hostname[MAX_DNS_NAME];
   strcpy(hostname, s_hostname);
   uint16_t port = s_port;
   char trustedCA[MAX_PATH];
   strcpy(trustedCA, s_trustedCA);
   bool ipv6Literal = (strchr(s_hostname, ':') != nullptr);
   size_t requestSize = strlen(s_validateURL) + strlen(ticket) + strlen(s_service) + strlen(s_hostname) + 64;
   char *request = MemAllocStringA(requestSize);
   snprintf(request, requestSize, "GET %s?ticket=%s&service=%s HTTP/1.0\r\nHost: %s%s%s:%u\r\n\r\n",
         s_validateURL, ticket, s_service, ipv6Literal ? "[" : "", s_hostname, ipv6Literal ? "]" : "", static_cast<unsigned int>(s_port));
   s_lock.unlock();

   wchar_t casLoginName[MAX_USER_NAME];
   int rc = ValidateTicket(hostname, port, trustedCA, request, casLoginName);

   SecureZeroMemory(request, requestSize);  // contains ticket
   MemFree(request);

   if (rc != CAS_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CAS: ticket validation failed, error %d", rc);
      return false;
   }
   wcscpy(loginName, casLoginName);
   return true;
}
