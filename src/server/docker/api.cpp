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
   m_caCert = nullptr;
   m_clientCert = nullptr;
   m_clientKey = nullptr;
}

/**
 * DockerClient destructor
 */
DockerClient::~DockerClient()
{
   MemFree(m_caCert);
   MemFree(m_clientCert);
   MemFree(m_clientKey);
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
      const char *caCert = json_object_get_string_utf8(credentials, "tlsCaCert", nullptr);
      if (caCert != nullptr)
      {
         // TLS enabled
         snprintf(m_baseUrl, sizeof(m_baseUrl), "https://%s", hostPort);
         m_useTLS = true;
         m_caCert = MemCopyStringA(caCert);

         const char *clientCert = json_object_get_string_utf8(credentials, "tlsCert", nullptr);
         if (clientCert != nullptr)
            m_clientCert = MemCopyStringA(clientCert);

         const char *clientKey = json_object_get_string_utf8(credentials, "tlsKey", nullptr);
         if (clientKey != nullptr)
            m_clientKey = MemCopyStringA(clientKey);

         nxlog_debug_tag(DEBUG_TAG, 5, L"DockerClient: configured for TLS connection to \"%hs\"", hostPort);
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

#ifndef _WIN32

/**
 * TLS context data for SSL_CTX_FUNCTION callback
 */
struct TLSContextData
{
   const char *caCert;
   const char *clientCert;
   const char *clientKey;
};

/**
 * Curl SSL context callback for loading PEM certificates from memory
 */
static CURLcode SSLContextCallback(CURL *curl, void *sslctx, void *context)
{
   SSL_CTX *ctx = static_cast<SSL_CTX*>(sslctx);
   TLSContextData *data = static_cast<TLSContextData*>(context);

#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
   if (g_curlOpenSSL3Backend)
      SSL_CTX_set_options(ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);
#endif

   // Load CA certificate
   if (data->caCert != nullptr)
   {
      BIO *bio = BIO_new_mem_buf(data->caCert, -1);
      if (bio != nullptr)
      {
         X509 *cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
         if (cert != nullptr)
         {
            X509_STORE *store = SSL_CTX_get_cert_store(ctx);
            X509_STORE_add_cert(store, cert);
            X509_free(cert);
         }
         BIO_free(bio);
      }
   }

   // Load client certificate
   if (data->clientCert != nullptr)
   {
      BIO *bio = BIO_new_mem_buf(data->clientCert, -1);
      if (bio != nullptr)
      {
         X509 *cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
         if (cert != nullptr)
         {
            SSL_CTX_use_certificate(ctx, cert);
            X509_free(cert);
         }
         BIO_free(bio);
      }
   }

   // Load client private key
   if (data->clientKey != nullptr)
   {
      BIO *bio = BIO_new_mem_buf(data->clientKey, -1);
      if (bio != nullptr)
      {
         EVP_PKEY *key = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
         if (key != nullptr)
         {
            SSL_CTX_use_PrivateKey(ctx, key);
            EVP_PKEY_free(key);
         }
         BIO_free(bio);
      }
   }

   return CURLE_OK;
}

#endif /* _WIN32 */

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

#ifndef _WIN32
   // TLS with in-memory certificates
   TLSContextData tlsData;
   if (m_useTLS)
   {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)1);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)2);

      tlsData.caCert = m_caCert;
      tlsData.clientCert = m_clientCert;
      tlsData.clientKey = m_clientKey;
      curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, SSLContextCallback);
      curl_easy_setopt(curl, CURLOPT_SSL_CTX_DATA, &tlsData);
   }
#else
   if (m_useTLS)
   {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)0);
   }
#endif

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
