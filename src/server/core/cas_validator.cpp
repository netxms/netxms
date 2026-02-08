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

#define DEBUG_TAG _T("cas")

/**
 * Settings
 */
static char s_hostname[MAX_DNS_NAME] = "localhost";
static int s_port = 8443;
static char s_service[MAX_CONFIG_VALUE_LENGTH] = "";
static char s_trustedCA[MAX_PATH] = "";
static char s_validateURL[MAX_CONFIG_VALUE_LENGTH] = "/cas/serviceValidate";
static StringSet s_proxies;
static Mutex s_lock;

/**
 * Read CAS settings
 */
void CASReadSettings()
{
   s_lock.lock();

   ConfigReadStrA(L"CAS.Host", s_hostname, MAX_DNS_NAME, "localhost");
   s_port = ConfigReadInt(L"CAS.Port", 8443);
   ConfigReadStrA(L"CAS.Service", s_service, MAX_CONFIG_VALUE_LENGTH, "http://127.0.0.1:10080/nxmc");
   ConfigReadStrA(L"CAS.TrustedCACert", s_trustedCA, MAX_PATH, "");
   ConfigReadStrA(L"CAS.ValidateURL", s_validateURL, MAX_CONFIG_VALUE_LENGTH, "/cas/serviceValidate");

   wchar_t proxies[MAX_CONFIG_VALUE_LENGTH];
   ConfigReadStr(_T("CAS.AllowedProxies"), proxies, MAX_CONFIG_VALUE_LENGTH, L"");
   s_proxies.clear();
   TrimW(proxies);
   if (proxies[0] != 0)
   {
      s_proxies.splitAndAdd(proxies, L",");
   }

   s_lock.unlock();
   nxlog_debug_tag(DEBUG_TAG, 4, L"CAS configuration reloaded");
}

/**
 * Main code
 */

/**
 * Ticket identifiers to avoid needless validating passwords that
 * aren't tickets
 */
#define CAS_BEGIN_PT "PT-"
#define CAS_BEGIN_ST "ST-"


/* Error codes (decided upon by ESUP-Portail group) */
#define CAS_SUCCESS                 0
#define CAS_AUTHENTICATION_FAILURE -1
#define CAS_ERROR                  -2

#define CAS_SSL_ERROR_CTX          -10
#define CAS_SSL_ERROR_CONN         -11
#define CAS_SSL_ERROR_CERT         -12
#define CAS_SSL_ERROR_HTTPS        -13

#define CAS_ERROR_CONN             -20
#define CAS_PROTOCOL_FAILURE       -21
#define CAS_BAD_PROXY              -22


#define SET_RET_AND_GOTO_END(x) { ret = (x); goto end; }
#define FAIL SET_RET_AND_GOTO_END(CAS_ERROR)
#define SUCCEED SET_RET_AND_GOTO_END(CAS_SUCCESS)

/**
 * Read certificate from file
 */
static X509 *ReadCertificateFromFile(const char *filename)
{
   FILE *f = fopen(filename, "r");
   if (f == nullptr)
      return nullptr;

   X509 *c = PEM_read_X509(f, nullptr, nullptr, nullptr);
   fclose(f);
   return c;
}

/*
 * Fills buf (up to buflen characters) with all characters (including
 * those representing other elements) within the nth element in the
 * document with the name provided by tagname.
 */
static char *element_body(const char *doc, const char *tagname, int n, char *buf, int buflen)
{
   const char *body_end;
   char *ret = nullptr;

   buf[0] = 0;

   char start_tag_pattern[256];
   snprintf(start_tag_pattern, 256, "<%s>", tagname);

   char end_tag_pattern[256];
   snprintf(end_tag_pattern, 256, "</%s>", tagname);

   const char *body_start = doc;
   while (n-- > 0)
   {
      body_start = strstr(body_start, start_tag_pattern);
      if (!body_start)
      {
         SET_RET_AND_GOTO_END(nullptr);
      }
      body_start += strlen(start_tag_pattern);
   }
   body_end = strstr(body_start, end_tag_pattern);
   if (!body_end)
   {
      SET_RET_AND_GOTO_END(nullptr);
   }
   if (body_end - body_start < buflen - 1)
   {
      strncpy(buf, body_start, body_end - body_start);
      buf[body_end - body_start] = 0;
   }
   else
   {
      strlcpy(buf, body_start, buflen);
   }
   SET_RET_AND_GOTO_END(buf);

end:
   return ret;
}

/**
 * Returns status of certification:  0 for invalid, 1 for valid.
 */
static int IsValidCertificate(X509 *cert, const char *hostname)
{
   char buf[4096];
   X509_STORE *store = X509_STORE_new();
   X509_STORE_CTX *ctx = X509_STORE_CTX_new();
   X509 *cacert = ReadCertificateFromFile(s_trustedCA);
   if (cacert != NULL)
   {
      X509_STORE_add_cert(store, cacert);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CAS: cannot load CA certificate from file %hs"), s_trustedCA);
   }
   X509_STORE_CTX_init(ctx, store, cert, sk_X509_new_null());
   if (X509_verify_cert(ctx) == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CAS: X509_verify_cert() failed"));
      return 0;
   }
   X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, sizeof(buf) - 1);
   // anal-retentive:  make sure the hostname isn't as long as the
   // buffer, since we don't want to match only because of truncation
   if (strlen(hostname) >= sizeof(buf) - 1)
   {
      return 0;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("CAS: certificate CN=%hs, hostname=%hs"), buf, hostname);
   return (!strcmp(buf, hostname));
}

/** Returns status of ticket by filling 'loginName' with a NetID if the ticket
 *  is valid and not exceeds NetXMS user name length limitation and returning 0.
 *  If not, error code is returned.
 */
static int CASValidate(const char *ticket, char *loginName)
{
   InetAddress a;
   SockAddrBuffer sa;
   SOCKET s = INVALID_SOCKET;
   int b, ret;
   size_t total;
   SSL *ssl = nullptr;
   X509 *s_cert = nullptr;
   char buf[4096];
   char *full_request = nullptr, *str;
   char parsebuf[MAX_DNS_NAME];

   SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
   if (!ctx)
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CTX);
   }
   if ((s = CreateSocket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      SET_RET_AND_GOTO_END(CAS_ERROR_CONN);
   }

   a = InetAddress::resolveHostName(s_hostname);
   if (!a.isValidUnicast())
   {
      SET_RET_AND_GOTO_END(CAS_ERROR_CONN);
   }
   a.fillSockAddr(&sa, s_port);
   if (connect(s, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa)) == -1)
   {
      SET_RET_AND_GOTO_END(CAS_ERROR_CONN);
   }
   if (!(ssl = SSL_new(ctx)))
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CTX);
   }
   if (!SSL_set_fd(ssl, (int)s))
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CTX);
   }
   if (SSL_connect(ssl) <= 0)
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CONN);
   }
   if (!(s_cert = SSL_get_peer_certificate(ssl)))
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CERT);
   }
   if (!IsValidCertificate(s_cert, s_hostname))
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CERT);
   }

   X509_free(s_cert);

   full_request = MemAllocStringA(4096);
   if (snprintf(full_request, 4096, "GET %s?ticket=%s&service=%s HTTP/1.0\r\n\r\n", s_validateURL, ticket, s_service) >= 4096)
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_HTTPS);
   }
   if (!SSL_write(ssl, full_request, (int)strlen(full_request)))
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_HTTPS);
   }

   total = 0;
   do 
   {
      b = SSL_read(ssl, buf + total, static_cast<int>(sizeof(buf) - 1 - total));
      total += b;
   } while (b > 0);
   buf[total] = '\0';

   if ((b != 0) || (total >= sizeof(buf) - 1))
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_HTTPS);		// unexpected read error or response too large
   }

   str = (char *)strstr(buf, "\r\n\r\n");  // find the end of the header

   if (str == nullptr)
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_HTTPS);			  // no header
   }

   /*
    * 'str' now points to the beginning of the body, which should be an
    * XML document
    */

   // make sure that the authentication succeeded

   if (!element_body(str, "cas:authenticationSuccess", 1, parsebuf, sizeof(parsebuf)))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CAS: authentication failure"));
      SET_RET_AND_GOTO_END(CAS_AUTHENTICATION_FAILURE);
   }

   // retrieve the NetID
   if (!element_body(str, "cas:user", 1, loginName, MAX_USER_NAME))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CAS: unable to determine user name"));
      SET_RET_AND_GOTO_END(CAS_PROTOCOL_FAILURE);
   }

   // check the first proxy (if present)
   if (element_body(str, "cas:proxies", 1, parsebuf, sizeof(parsebuf)) != nullptr)
   {
      if (element_body(str, "cas:proxy", 1, parsebuf, sizeof(parsebuf)) != nullptr)
      {
         wchar_t wproxy[MAX_DNS_NAME];
         utf8_to_wchar(parsebuf, -1, wproxy, MAX_DNS_NAME);
         if (!s_proxies.contains(wproxy))
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("CAS: proxy %hs is not in allowed proxies list"), parsebuf);
            SET_RET_AND_GOTO_END(CAS_BAD_PROXY);
         }
      }
   }

   SUCCEED;

   /* cleanup and return */

end:
   MemFree(full_request);
   if (ssl != nullptr)
   {
      SSL_shutdown(ssl);
      SSL_free(ssl);
   }
   if (s != INVALID_SOCKET)
      closesocket(s);
   if (ctx != nullptr)
      SSL_CTX_free(ctx);
   return ret;
}

/**
 * Authenticate user via CAS
 */
bool CASAuthenticate(const char *ticket, wchar_t *loginName)
{
   bool success = false;
   s_lock.lock();
   char mbLogin[MAX_USER_NAME];
   int rc = CASValidate(ticket, mbLogin);
   if (rc == CAS_SUCCESS)
   {
      mb_to_wchar(mbLogin, -1, loginName, MAX_USER_NAME);
      success = true;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CAS: ticket %hs validation failed, error %d"), ticket, rc);
   }
   s_lock.unlock();
   return success;
}
