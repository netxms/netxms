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

#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Settings
 */
static char m_host[128] = "localhost";
static int m_port = 8443;
static char m_trustedCA[1024] = "/Users/alk/Development/cas/pam_cas/ca.pem";
//static char m_validateURL[1024] = "/cas/proxyValidate";
static char m_validateURL[1024] = "/cas/serviceValidate";
static char *m_proxies[] = { NULL };


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

#define DEBUG
//#undef DEBUG

#ifdef DEBUG
# define LOG(X) printf("%s", (X))
#else
# define LOG(X) 
#endif

int cas_validate(char *ticket, char *service, char *outbuf, int outbuflen, char *proxies[]);
static X509 *get_cert_from_file(char *filename);
static int valid_cert(X509 *cert, char *hostname);
static int arrayContains(char *array[], char *element);
char *element_body(char *doc, char *tagname, int n, char *buf, int buflen);

/** Returns status of certification:  0 for invalid, 1 for valid. */
static int valid_cert(X509 *cert, char *hostname) {
   int i;
   char buf[4096];
   X509_STORE *store = X509_STORE_new();
   X509_STORE_CTX *ctx = X509_STORE_CTX_new();
   X509 *cacert = get_cert_from_file(m_trustedCA);
   if (cacert != NULL) {
      X509_STORE_add_cert(store, cacert);
   }
   X509_STORE_CTX_init(ctx, store, cert, sk_X509_new_null());
   if (X509_verify_cert(ctx) == 0) {
      return 0;
   }
   X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, sizeof(buf) - 1);
   // anal-retentive:  make sure the hostname isn't as long as the
   // buffer, since we don't want to match only because of truncation
   if (strlen(hostname) >= sizeof(buf) - 1) {
      return 0;
   }
   return (!strcmp(buf, hostname));
}

/** Returns status of ticket by filling 'buf' with a NetID if the ticket
 *  is valid and buf is large enough and returning 1.  If not, 0 is
 *  returned.
 */
int cas_validate(char *ticket, char *service, char *outbuf, int outbuflen, char *proxies[]) {
   int s = 0, err, b, ret, total;
   struct sockaddr_in sa;
   struct hostent h, *hp2;
   SSL_CTX *ctx = NULL;
   SSL *ssl = NULL;
   X509 *s_cert = NULL;
   char buf[4096];
   SSL_METHOD *method = NULL;
   char *full_request, *str, *tmp;
   char netid[14];
   char parsebuf[128];
   int i;

   SSLeay_add_ssl_algorithms();
   method = SSLv23_client_method();
   SSL_load_error_strings();
   ctx = SSL_CTX_new(method);
   if (!ctx) {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CTX);
   }
   if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      SET_RET_AND_GOTO_END(CAS_ERROR_CONN);
   }

   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   hp2 = gethostbyname(m_host);
   memcpy(&h, hp2, sizeof(h));
   //  gethostbyname_r(m_host, &h, buf, sizeof(buf), &hp2, &b);

   memcpy(&(sa.sin_addr.s_addr), h.h_addr_list[0], sizeof(long));
   sa.sin_port = htons(m_port);
   if (connect(s, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
      SET_RET_AND_GOTO_END(CAS_ERROR_CONN);
   }
   if (!(ssl = SSL_new(ctx))) {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CTX);
   }
   if (!SSL_set_fd(ssl, s)) {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_CTX);
   }
   if (! (err = SSL_connect(ssl))) {
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
   if (!SSL_write(ssl, full_request, strlen(full_request))) {
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

   if (!str) {
      SET_RET_AND_GOTO_END(CAS_SSL_ERROR_HTTPS);			  // no header
   }

   /*
    * 'str' now points to the beginning of the body, which should be an
    * XML document
    */

   // make sure that the authentication succeeded

   if (!element_body(str, "cas:authenticationSuccess", 1, parsebuf, sizeof(parsebuf))) {
      LOG("authentication failure\n");
      LOG(str);
      LOG("\n");
      SET_RET_AND_GOTO_END(CAS_AUTHENTICATION_FAILURE);
   }

   // retrieve the NetID
   if (!element_body(str, "cas:user", 1, netid, sizeof(netid))) {
      LOG("unable to determine username\n");
      SET_RET_AND_GOTO_END(CAS_PROTOCOL_FAILURE);
   }

   // check the first proxy (if present)
   if (element_body(str, "cas:proxies", 1, parsebuf, sizeof(parsebuf))) {
      if (element_body(str, "cas:proxy", 1, parsebuf, sizeof(parsebuf))) {
         if (!arrayContains(proxies, parsebuf)) {
            LOG("bad proxy: ");
            LOG(parsebuf);
            LOG("\n");
            SET_RET_AND_GOTO_END(CAS_BAD_PROXY);
         }
      }
   }

   /*
    * without enough space, fail entirely, since a partial NetID could
    * be dangerous
    */
   if (outbuflen < strlen(netid) + 1) {
      LOG("output buffer too short\n");
      SET_RET_AND_GOTO_END(CAS_PROTOCOL_FAILURE);
   }

   strcpy(outbuf, netid);
   SUCCEED;

   /* cleanup and return */

end:
   if (ssl) SSL_shutdown(ssl);
   if (s > 0) close(s);
   if (ssl) SSL_free(ssl);
   if (ctx) SSL_CTX_free(ctx);
   return ret;
}

static X509 *get_cert_from_file(char *filename) {
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
static int arrayContains(char *array[], char *element) {
   char *p;
   int i = 0;

   for (p = array[0]; p; p = array[++i]) {
      LOG("  checking element ");
      LOG(p);
      LOG("\n");
      if (!strcmp(p, element)) {
         return 1;
      }
   }
   return 0;
}

/*
 * Fills buf (up to buflen characters) with all characters (including
 * those representing other elements) within the nth element in the
 * document with the name provided by tagname.
 */
char *element_body(char *doc, char *tagname, int n, char *buf, int buflen) {
   char *start_tag_pattern = (char *)malloc(strlen(tagname) + strlen("<") + strlen(">") + 1);
   char *end_tag_pattern = (char *)malloc(strlen(tagname) + strlen("<") + strlen("/") + strlen(">") + 1);
   char *body_start, *body_end;
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
