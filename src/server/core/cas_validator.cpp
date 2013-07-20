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

/**
 * Settings
 */
static char m_host[128] = "localhost";
static int m_port = 8443;
static char m_service[MAX_PATH] = "";
static char m_trustedCA[MAX_PATH] = "";
static char m_validateURL[MAX_PATH] = "/cas/serviceValidate";
static char *m_proxies[] = { NULL };
static MUTEX m_lock = MutexCreate();

/**
 * Read CAS settings
 */
void CASReadSettings()
{
   MutexLock(m_lock);
   ConfigReadStrA(_T("CASHost"), m_host, 128, "localhost");
   m_port = ConfigReadInt(_T("CASPort"), 8443);
   ConfigReadStrA(_T("CASService"), m_service, MAX_PATH, "http://127.0.0.1:10080/nxmc");
   ConfigReadStrA(_T("CASTrustedCACert"), m_trustedCA, MAX_PATH, "");
   ConfigReadStrA(_T("CASValidateURL"), m_validateURL, MAX_PATH, "/cas/serviceValidate");
   MutexUnlock(m_lock);
   DbgPrintf(4, _T("CAS config reloaded"));
}

/**
 * Main code
 */

#ifdef _WITH_ENCRYPTION

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

static int cas_validate(const char *ticket, const char *service, char *outbuf, int outbuflen, char *proxies[]);
static X509 *get_cert_from_file(const char *filename);
static int valid_cert(X509 *cert, const char *hostname);
static int arrayContains(char *array[], char *element);
static char *element_body(const char *doc, const char *tagname, int n, char *buf, int buflen);

/** Returns status of certification:  0 for invalid, 1 for valid. */
static int valid_cert(X509 *cert, const char *hostname) 
{
   char buf[4096];
   X509_STORE *store = X509_STORE_new();
   X509_STORE_CTX *ctx = X509_STORE_CTX_new();
   X509 *cacert = get_cert_from_file(m_trustedCA);
   if (cacert != NULL) 
   {
      X509_STORE_add_cert(store, cacert);
   }
   else
   {
      DbgPrintf(4, _T("CAS: cannot load CA certificate from file %hs"), m_trustedCA);
   }
   X509_STORE_CTX_init(ctx, store, cert, sk_X509_new_null());
   if (X509_verify_cert(ctx) == 0) 
   {
      DbgPrintf(4, _T("CAS: X509_verify_cert() failed"));
      return 0;
   }
   X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, sizeof(buf) - 1);
   // anal-retentive:  make sure the hostname isn't as long as the
   // buffer, since we don't want to match only because of truncation
   if (strlen(hostname) >= sizeof(buf) - 1) 
   {
      return 0;
   }

   DbgPrintf(6, _T("CAS: certificate CN=%hs, hostname=%hs"), buf, hostname);
   return (!strcmp(buf, hostname));
}

/** Returns status of ticket by filling 'buf' with a NetID if the ticket
 *  is valid and buf is large enough and returning 0.  If not, error code is
 *  returned.
 */
static int cas_validate(const char *ticket, const char *service, char *outbuf, int outbuflen, char *proxies[]) 
{
   SOCKET s = INVALID_SOCKET;
   int err, b, ret, total;
   struct sockaddr_in sa;
   struct hostent h, *hp2;
   SSL *ssl = NULL;
   X509 *s_cert = NULL;
   char buf[4096];
   char *full_request, *str;
   char netid[14];
   char parsebuf[128];

   SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
   if (!ctx) 
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CTX);
   }
   if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
   {
      SET_RET_AND_GOTO_END(CAS_ERROR_CONN);
   }

   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   hp2 = gethostbyname(m_host);
   memcpy(&h, hp2, sizeof(h));
   //  gethostbyname_r(m_host, &h, buf, sizeof(buf), &hp2, &b);

   memcpy(&(sa.sin_addr.s_addr), h.h_addr_list[0], sizeof(long));
   sa.sin_port = htons(m_port);
   if (connect(s, (struct sockaddr*) &sa, sizeof(sa)) == -1) 
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
   if (! (err = SSL_connect(ssl))) 
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CONN);
   }
   if (!(s_cert = SSL_get_peer_certificate(ssl))) {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CERT);
   }
   if (!valid_cert(s_cert, m_host)) {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CERT);
   }

   X509_free(s_cert);

   full_request = (char *)malloc(4096);
   if (snprintf(full_request, 4096, "GET %s?ticket=%s&service=%s HTTP/1.0\n\n", m_validateURL, ticket, service) >= 4096) {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_HTTPS);
   }
   if (!SSL_write(ssl, full_request, (int)strlen(full_request))) 
   {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_HTTPS);
   }

   total = 0;
   do {
      b = SSL_read(ssl, buf + total, (sizeof(buf) - 1) - total);
      total += b;
   } while (b > 0);
   buf[total] = '\0';

   if (b != 0 || total >= sizeof(buf) - 1) {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_HTTPS);		// unexpected read error or response too large
   }

   str = (char *)strstr(buf, "\r\n\r\n");  // find the end of the header

   if (!str) 
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
      DbgPrintf(4, _T("CAS: authentication failure"));
      SET_RET_AND_GOTO_END(CAS_AUTHENTICATION_FAILURE);
   }

   // retrieve the NetID
   if (!element_body(str, "cas:user", 1, netid, sizeof(netid))) 
   {
      DbgPrintf(4, _T("CAS: unable to determine username"));
      SET_RET_AND_GOTO_END(CAS_PROTOCOL_FAILURE);
   }

   // check the first proxy (if present)
   if (element_body(str, "cas:proxies", 1, parsebuf, sizeof(parsebuf))) 
   {
      if (element_body(str, "cas:proxy", 1, parsebuf, sizeof(parsebuf))) 
      {
         if (!arrayContains(proxies, parsebuf)) 
         {
            DbgPrintf(4, _T("CAS: bad proxy (%hs)"), parsebuf);
            SET_RET_AND_GOTO_END(CAS_BAD_PROXY);
         }
      }
   }

   /*
    * without enough space, fail entirely, since a partial NetID could
    * be dangerous
    */
   if (outbuflen < strlen(netid) + 1) 
   {
      DbgPrintf(4, _T("CAS: output buffer too short"));
      SET_RET_AND_GOTO_END(CAS_PROTOCOL_FAILURE);
   }

   strcpy(outbuf, netid);
   SUCCEED;

   /* cleanup and return */

end:
   if (ssl) SSL_shutdown(ssl);
   if (s != INVALID_SOCKET) closesocket(s);
   if (ssl) SSL_free(ssl);
   if (ctx) SSL_CTX_free(ctx);
   return ret;
}

static X509 *get_cert_from_file(const char *filename) 
{
   X509 *c;
   FILE *f = fopen(filename, "r");
   if (!f) {
      return NULL;
   }
   c = PEM_read_X509(f, NULL, NULL, NULL);
   fclose(f);
   return c;
}

// returns 1 if a char* array contains the given element, 0 otherwise
static int arrayContains(char *array[], char *element) 
{
   char *p;
   int i = 0;

   for (p = array[0]; p; p = array[++i]) 
   {
      if (!strcmp(p, element))
         return 1;
   }
   return 0;
}

/*
 * Fills buf (up to buflen characters) with all characters (including
 * those representing other elements) within the nth element in the
 * document with the name provided by tagname.
 */
static char *element_body(const char *doc, const char *tagname, int n, char *buf, int buflen) 
{
   char *start_tag_pattern = (char *)malloc(strlen(tagname) + strlen("<") + strlen(">") + 1);
   char *end_tag_pattern = (char *)malloc(strlen(tagname) + strlen("<") + strlen("/") + strlen(">") + 1);
   const char *body_start, *body_end;
   char *ret = NULL;

   buf[0] = 0;

   if (start_tag_pattern != NULL && end_tag_pattern != NULL)
   {
      sprintf(start_tag_pattern, "<%s>", tagname);
      sprintf(end_tag_pattern, "</%s>", tagname);
      body_start = doc;
      while (n-- > 0)
      {
         body_start = strstr(body_start, start_tag_pattern);
         if (!body_start)
         {
            SET_RET_AND_GOTO_END(NULL);
         }
         body_start += strlen(start_tag_pattern);
      }
      body_end = strstr(body_start, end_tag_pattern);
      if (!body_end)
      {
         SET_RET_AND_GOTO_END(NULL);
      }
      if (body_end - body_start < buflen - 1)
      {
         strncpy(buf, body_start, body_end - body_start);
         buf[body_end - body_start] = 0;
      }
      else
      {
         strncpy(buf, body_start, buflen - 1);
         buf[buflen - 1] = 0;
      }
      SET_RET_AND_GOTO_END(buf);
   }

end:
   if (start_tag_pattern) free(start_tag_pattern);
   if (end_tag_pattern) free(end_tag_pattern);
   return ret;
}

/**
 * Authenticate user via CAS
 */
bool CASAuthenticate(const char *ticket, TCHAR *loginName)
{
   bool success = false;
   MutexLock(m_lock);
#ifdef UNICODE
   char mbLogin[MAX_USER_NAME];
   int rc = cas_validate(ticket, m_service, mbLogin, MAX_USER_NAME, m_proxies);
   if (rc == CAS_SUCCESS)
   {
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbLogin, -1, loginName, MAX_USER_NAME);
      success = true;
   }
#else
   int rc = cas_validate(ticket, m_service, loginName, MAX_USER_NAME, m_proxies);
   if (rc == CAS_SUCCESS)
   {
      success = true;
   }
#endif
   else
   {
      DbgPrintf(4, _T("CAS: ticket %hs validation failed, error %d"), ticket, rc);
   }
   MutexUnlock(m_lock);
   return success;
}

#else	/* _WITH_ENCRYPTION */

bool CASAuthenticate(const char *ticket, TCHAR *loginName)
{
	DbgPrintf(4, _T("CAS ticket cannot be validated - server built without encryption support"));
	return false;
}

#endif
