/*
** NetXMS Docker cloud connector module
** Copyright (C) 2026 Raden Solutions
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
** File: api.cpp
**/

#include "docker.h"
#include <netxms-version.h>

/**
 * Docker Engine API version used in URL paths
 */
#define DOCKER_API_VERSION "/v1.47"

/**
 * DockerClient constructor
 */
DockerClient::DockerClient()
{
   m_baseUrl[0] = 0;
   m_socketPath[0] = 0;
   m_useTLS = false;
   m_caCertPEM = nullptr;
   m_caCerts = nullptr;
   m_clientCert = nullptr;
   m_clientKey = nullptr;
   m_pkcs12 = nullptr;
   m_pkcs12Size = 0;
   m_pkcs12Password[0] = 0;
}

/**
 * DockerClient destructor
 */
DockerClient::~DockerClient()
{
   MemFree(m_caCertPEM);
   if (m_caCerts != nullptr)
      sk_X509_pop_free(m_caCerts, X509_free);
   if (m_clientCert != nullptr)
      X509_free(m_clientCert);
   if (m_clientKey != nullptr)
      EVP_PKEY_free(m_clientKey);
   MemFree(m_pkcs12);
}

/**
 * Passphrase callback for PEM readers. Encrypted private keys are not supported, and without this
 * callback OpenSSL would prompt for passphrase on the controlling terminal.
 */
static int NoPassphraseCallback(char *buffer, int size, int rwflag, void *userdata)
{
   return 0;
}

/**
 * Get text of last OpenSSL error and clear error queue
 */
static const char *GetLastOpenSSLError(char *buffer, size_t size)
{
   unsigned long error = ERR_get_error();
   if (error != 0)
      ERR_error_string_n(error, buffer, size);
   else
      strlcpy(buffer, "unknown error", size);
   ERR_clear_error();
   return buffer;
}

/**
 * Load TLS credentials (PEM encoded CA certificates, client certificate and client private key)
 */
bool DockerClient::loadTLSCredentials(json_t *credentials)
{
   char errorText[256];

   const char *caCert = json_object_get_string_utf8(credentials, "tlsCaCert", nullptr);
   const char *clientCert = json_object_get_string_utf8(credentials, "tlsCert", nullptr);
   const char *clientKey = json_object_get_string_utf8(credentials, "tlsKey", nullptr);

   if ((clientCert == nullptr) != (clientKey == nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: both \"tlsCert\" and \"tlsKey\" should be provided for client authentication");
      return false;
   }

   m_caCertPEM = MemCopyStringA(caCert);
   m_caCerts = sk_X509_new_null();
   BIO *bio = BIO_new_mem_buf(caCert, -1);
   if (bio != nullptr)
   {
      X509 *cert;
      while ((cert = PEM_read_bio_X509(bio, nullptr, NoPassphraseCallback, nullptr)) != nullptr)
         sk_X509_push(m_caCerts, cert);
      BIO_free(bio);
   }
   ERR_clear_error();   // reading stops with "no start line" error at the end of input
   if (sk_X509_num(m_caCerts) == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot parse CA certificate from \"tlsCaCert\"");
      return false;
   }

   if (clientCert == nullptr)
      return true;

   bio = BIO_new_mem_buf(clientCert, -1);
   if (bio != nullptr)
   {
      m_clientCert = PEM_read_bio_X509(bio, nullptr, NoPassphraseCallback, nullptr);
      BIO_free(bio);
   }
   if (m_clientCert == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot parse client certificate from \"tlsCert\" (%hs)", GetLastOpenSSLError(errorText, sizeof(errorText)));
      return false;
   }

   bio = BIO_new_mem_buf(clientKey, -1);
   if (bio != nullptr)
   {
      m_clientKey = PEM_read_bio_PrivateKey(bio, nullptr, NoPassphraseCallback, nullptr);
      BIO_free(bio);
   }
   if (m_clientKey == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot parse client private key from \"tlsKey\" (%hs)", GetLastOpenSSLError(errorText, sizeof(errorText)));
      return false;
   }

   if (!X509_check_private_key(m_clientCert, m_clientKey))
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: client private key does not match client certificate (%hs)", GetLastOpenSSLError(errorText, sizeof(errorText)));
      return false;
   }

   return true;
}

/**
 * Configure client from cloud domain credentials.
 * Credential JSON format:
 * {
 *   "endpoint": "unix:///var/run/docker.sock" or "tcp://host:port",
 *   "tlsCaCert": "-----BEGIN CERTIFICATE-----\n...",
 *   "tlsCert": "-----BEGIN CERTIFICATE-----\n...",
 *   "tlsKey": "-----BEGIN RSA PRIVATE KEY-----\n..."
 * }
 * TLS is used when "tlsCaCert" is present; "tlsCert" and "tlsKey" are optional, but must be given together.
 */
bool DockerClient::configure(json_t *credentials)
{
   if (credentials == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DockerClient::configure: no credentials provided");
      return false;
   }

   const char *endpoint = json_object_get_string_utf8(credentials, "endpoint", "");

   if (!strncmp(endpoint, "unix://", 7))
   {
      // Unix socket connection
      strlcpy(m_socketPath, endpoint + 7, sizeof(m_socketPath));
      strlcpy(m_baseUrl, "http://localhost", sizeof(m_baseUrl));
      nxlog_debug_tag(DEBUG_TAG, 5, L"DockerClient: configured for Unix socket \"%hs\"", m_socketPath);
   }
   else if (!strncmp(endpoint, "tcp://", 6))
   {
      // TCP connection
      const char *hostPort = endpoint + 6;
      if (json_object_get_string_utf8(credentials, "tlsCaCert", nullptr) != nullptr)
      {
         // TLS enabled
         if (!loadTLSCredentials(credentials))
            return false;
         snprintf(m_baseUrl, sizeof(m_baseUrl), "https://%s", hostPort);
         m_useTLS = true;
         nxlog_debug_tag(DEBUG_TAG, 5, L"DockerClient: configured for TLS connection to \"%hs\"%s", hostPort, (m_clientCert != nullptr) ? L" with client certificate" : L"");
      }
      else if ((json_object_get_string_utf8(credentials, "tlsCert", nullptr) != nullptr) || (json_object_get_string_utf8(credentials, "tlsKey", nullptr) != nullptr))
      {
         nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: client certificate provided for \"%hs\" without CA certificate (\"tlsCaCert\")", hostPort);
         return false;
      }
      else
      {
         // No TLS
         snprintf(m_baseUrl, sizeof(m_baseUrl), "http://%s", hostPort);
         nxlog_debug_tag(DEBUG_TAG, 5, L"DockerClient: configured for TCP connection to \"%hs\"", hostPort);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DockerClient::configure: unsupported endpoint scheme \"%hs\"", endpoint);
      return false;
   }

   return true;
}

/**
 * Curl SSL context callback (OpenSSL backend) for installing CA certificates and client credentials
 */
CURLcode DockerClient::sslContextCallback(CURL *curl, void *sslctx, void *context)
{
   SSL_CTX *ctx = static_cast<SSL_CTX*>(sslctx);
   DockerClient *client = static_cast<DockerClient*>(context);

#if !defined(_WIN32) && defined(SSL_OP_IGNORE_UNEXPECTED_EOF)
   if (g_curlOpenSSL3Backend)
      SSL_CTX_set_options(ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);
#endif

   X509_STORE *store = SSL_CTX_get_cert_store(ctx);
   for(int i = 0; i < sk_X509_num(client->m_caCerts); i++)
      X509_STORE_add_cert(store, sk_X509_value(client->m_caCerts, i));
   ERR_clear_error();   // older OpenSSL versions report already present certificate as an error

   if (client->m_clientCert != nullptr)
   {
      if (!SSL_CTX_use_certificate(ctx, client->m_clientCert) || !SSL_CTX_use_PrivateKey(ctx, client->m_clientKey))
      {
         char errorText[256];
         nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot install client certificate (%hs)", GetLastOpenSSLError(errorText, sizeof(errorText)));
         return CURLE_SSL_CERTPROBLEM;
      }
   }

   return CURLE_OK;
}

#if HAVE_DECL_CURLOPT_CAINFO_BLOB

/**
 * Build PKCS#12 container with client certificate and key for TLS backends that cannot use PEM key
 * (Schannel). Legacy 3DES/SHA1 protection is used for compatibility with Windows PFX import.
 */
bool DockerClient::buildPKCS12()
{
   if (m_pkcs12 != nullptr)
      return true;

   // Schannel cannot acquire credentials for EC client key imported from PKCS#12 (fails with SEC_E_NO_CREDENTIALS)
   if ((EVP_PKEY_base_id(m_clientKey) == EVP_PKEY_EC) && !strncmp(GetLibCURLSSLBackend(), "Schannel", 8))
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: Schannel TLS backend does not support EC client keys, TLS handshake will likely fail (use RSA client key or set CurlSSLBackend to OpenSSL)");

   BYTE random[16];
   GenerateRandomBytes(random, sizeof(random));
   BinToStrA(random, sizeof(random), m_pkcs12Password);

   char errorText[256];
   PKCS12 *p12 = PKCS12_create(m_pkcs12Password, nullptr, m_clientKey, m_clientCert, nullptr,
      NID_pbe_WithSHA1And3_Key_TripleDES_CBC, NID_pbe_WithSHA1And3_Key_TripleDES_CBC, PKCS12_DEFAULT_ITER, -1, 0);
   if (p12 == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot create PKCS#12 container for client certificate (%hs)", GetLastOpenSSLError(errorText, sizeof(errorText)));
      return false;
   }
   if (!PKCS12_set_mac(p12, m_pkcs12Password, -1, nullptr, 0, PKCS12_DEFAULT_ITER, EVP_sha1()))
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot set PKCS#12 container MAC (%hs)", GetLastOpenSSLError(errorText, sizeof(errorText)));
      PKCS12_free(p12);
      return false;
   }

   int size = i2d_PKCS12(p12, nullptr);
   if (size > 0)
   {
      m_pkcs12 = MemAllocArrayNoInit<BYTE>(size);
      BYTE *p = m_pkcs12;
      i2d_PKCS12(p12, &p);
      m_pkcs12Size = size;
   }
   PKCS12_free(p12);
   if (m_pkcs12 == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot encode PKCS#12 container (%hs)", GetLastOpenSSLError(errorText, sizeof(errorText)));
      return false;
   }
   return true;
}

#endif /* HAVE_DECL_CURLOPT_CAINFO_BLOB */

/**
 * Setup TLS verification and client authentication on curl handle. OpenSSL backend gets CA
 * certificates and client credentials through SSL context callback (works with any libcurl version).
 * Other backends (Schannel, GnuTLS, etc.) reject the callback, and in-memory certificate blobs are
 * used instead. Returns false if TLS cannot be set up, so the request is never sent unverified.
 */
bool DockerClient::setupTLS(CURL *curl)
{
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)1);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)2);

   CURLcode rc = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, sslContextCallback);
   if (rc == CURLE_OK)
   {
      curl_easy_setopt(curl, CURLOPT_SSL_CTX_DATA, this);
      return true;
   }

#if HAVE_DECL_CURLOPT_CAINFO_BLOB
   nxlog_debug_tag(DEBUG_TAG, 7, L"DockerClient: SSL context callback is not supported by TLS backend (%d: %hs), using certificate blobs", rc, curl_easy_strerror(rc));

   struct curl_blob blob;
   blob.data = m_caCertPEM;
   blob.len = strlen(m_caCertPEM);
   blob.flags = CURL_BLOB_COPY;
   rc = curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);
   if (rc != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot set CA certificate (%d: %hs)", rc, curl_easy_strerror(rc));
      return false;
   }

   if (m_clientCert != nullptr)
   {
      if (!buildPKCS12())
         return false;

      blob.data = m_pkcs12;
      blob.len = m_pkcs12Size;
      blob.flags = CURL_BLOB_COPY;
      rc = curl_easy_setopt(curl, CURLOPT_SSLCERT_BLOB, &blob);
      if (rc == CURLE_OK)
         rc = curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "P12");
      if (rc == CURLE_OK)
         rc = curl_easy_setopt(curl, CURLOPT_KEYPASSWD, m_pkcs12Password);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: cannot set client certificate (%d: %hs)", rc, curl_easy_strerror(rc));
         return false;
      }
   }

#ifdef CURLSSLOPT_REVOKE_BEST_EFFORT
   // Private Docker CA usually has no CRL distribution points; Schannel would fail revocation check otherwise
   curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_REVOKE_BEST_EFFORT);
#endif
   return true;
#else
   nxlog_debug_tag(DEBUG_TAG, 1, L"DockerClient: TLS backend does not support SSL context callback (%d: %hs) and libcurl is too old for in-memory certificates", rc, curl_easy_strerror(rc));
   return false;
#endif
}

/**
 * Execute HTTP GET request against Docker Engine API.
 * Returns parsed JSON response (caller owns) or nullptr on error.
 */
json_t *DockerClient::httpGet(const char *path)
{
   if (IsShutdownInProgress())
      return nullptr;

   char url[1024];
   snprintf(url, sizeof(url), "%s" DOCKER_API_VERSION "%s", m_baseUrl, path);

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DockerClient::httpGet: curl_easy_init() failed");
      return nullptr;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Docker Connector/" NETXMS_VERSION_STRING_A);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   // Unix socket support
#if HAVE_DECL_CURLOPT_UNIX_SOCKET_PATH
   if (m_socketPath[0] != 0)
   {
      curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, m_socketPath);
   }
#endif

   if (m_useTLS && !setupTLS(curl))
   {
      curl_easy_cleanup(curl);
      return nullptr;
   }

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
   curl_easy_setopt(curl, CURLOPT_URL, url);

   CURLcode rc = curl_easy_perform(curl);
   if (rc != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"DockerClient::httpGet(%hs) failed (%d: %hs)", url, rc, errorBuffer);
      curl_easy_cleanup(curl);
      return nullptr;
   }

   long httpCode = 0;
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
   nxlog_debug_tag(DEBUG_TAG, 7, L"DockerClient::httpGet(%hs): %d bytes, HTTP %d",
      url, static_cast<int>(responseData.size()), static_cast<int>(httpCode));
   curl_easy_cleanup(curl);

   if ((httpCode < 200) || (httpCode > 299))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"DockerClient::httpGet(%hs): HTTP %d", path, static_cast<int>(httpCode));
      return nullptr;
   }

   if (responseData.size() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"DockerClient::httpGet(%hs): empty response", path);
      return nullptr;
   }

   responseData.write('\0');
   const char *data = reinterpret_cast<const char*>(responseData.buffer());
   json_error_t error;
   json_t *json = json_loads(data, 0, &error);
   if (json == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DockerClient::httpGet(%hs): failed to parse JSON on line %d: %hs", path, error.line, error.text);
      return nullptr;
   }

   return json;
}
