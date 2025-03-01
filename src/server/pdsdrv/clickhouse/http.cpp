/**
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for ClickHouse
 ** Copyright (C) 2024 Raden Solutions
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU Lesser General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
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
 ** File: http.cpp
 **/

#include "clickhouse.h"
#include <netxms-version.h>

#define MAX_URL_LENGTH  4096

#if HAVE_LIBCURL

/**
 * Constructor for HTTP sender
 */
HTTPSender::HTTPSender(const Config& config) : ClickHouseSender(config),
         m_database(config.getValue(_T("/ClickHouse/Database"), _T("netxms"))),
         m_table(config.getValue(_T("/ClickHouse/Table"), _T("metrics"))),
         m_user(config.getValue(_T("/ClickHouse/User"), _T(""))),
         m_password(config.getValue(_T("/ClickHouse/Password"), _T("")))
{
   if (m_port == 0)
      m_port = 8123;  // Default HTTP port for ClickHouse
   m_curl = nullptr;
   m_useHttps = config.getValueAsBoolean(_T("/ClickHouse/UseHTTPS"), false);
}

/**
 * Destructor for HTTP sender
 */
HTTPSender::~HTTPSender()
{
   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);
}

/**
 * Build ClickHouse HTTP URL
 */
void HTTPSender::buildURL(char *url)
{
   StringBuffer sb(m_useHttps ? _T("https://") : _T("http://"));
   sb.append(m_hostname);
   sb.append(_T(':'));
   sb.append(m_port);
   sb.append(_T("/"));
   sb.append(_T("?database="));
   sb.append(m_database);
   sb.append(_T("&query=INSERT%20INTO%20"));
   sb.append(m_table);
   sb.append(_T("%20FORMAT%20TSV"));
   
   tchar_to_utf8(sb, -1, url, MAX_URL_LENGTH);
}

/**
 * Add authentication headers if needed
 */
void HTTPSender::addHeaders(curl_slist **headers)
{
   if (!m_user.isEmpty() && !m_password.isEmpty())
   {
      TCHAR decryptedPassword[256];
      DecryptPassword(m_user, m_password, decryptedPassword, 256);
      
      char auth[1024];
      char *encodedAuth = static_cast<char*>(MemAlloc(1024));
      
      StringBuffer authString = m_user;
      authString.append(_T(':'));
      authString.append(decryptedPassword);
      
      size_t len = wchar_to_utf8(authString, -1, encodedAuth, 1024);
      base64_encode((BYTE *)encodedAuth, len, auth, 1024);
      MemFree(encodedAuth);
      
      char header[1100];
      snprintf(header, 1100, "Authorization: Basic %s", auth);
      *headers = curl_slist_append(*headers, header);
   }
}

/**
 * Send data block via HTTP
 */
bool HTTPSender::send(const char *data, size_t size)
{
   if (m_curl == nullptr)
   {
      time_t now = time(nullptr);
      if (m_lastConnect + 60 > now) // Attempt to re-create handle once per minute
         return false;
      m_curl = curl_easy_init();
      m_lastConnect = now;
      if (m_curl == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
         return false;
      }

      // Common handle setup
#if HAVE_DECL_CURLOPT_NOSIGNAL
      curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
#endif

      curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
      curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L); // do not include header in data
      curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 300L);
      curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
      curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "NetXMS ClickHouse Driver/" NETXMS_VERSION_STRING_A);

      char url[MAX_URL_LENGTH];
      buildURL(url);
      curl_easy_setopt(m_curl, CURLOPT_URL, url);

      nxlog_debug_tag(DEBUG_TAG, 4, _T("New cURL handle created for URL %hs"), url);
   }

   curl_slist *headers = nullptr;
   addHeaders(&headers);
   headers = curl_slist_append(headers, "Content-Type: text/plain; charset=utf-8");
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

   char errorText[CURL_ERROR_SIZE];
   curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, errorText);

   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, data);
   nxlog_debug_tag(DEBUG_TAG, 9, _T("Sending data: %hs"), data);

   bool success;
   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      long responseCode;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("HTTP response %03ld, %d bytes data"), responseCode, static_cast<int>(responseData.size()));
      if (responseData.size() > 0)
      {
         responseData.write('\0');
         nxlog_debug_tag(DEBUG_TAG, 7, _T("API response: %hs"), responseData.buffer());
      }
      // Consider only 2xx codes as success
      success = (responseCode >= 200) && (responseCode < 300);
      if (!success)
      {
         // Cleanup handle on error to force reconnect
         curl_easy_cleanup(m_curl);
         m_curl = nullptr;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed (%hs)"), errorText);
      curl_easy_cleanup(m_curl);
      m_curl = nullptr;
      success = false;
   }

   curl_slist_free_all(headers);
   return success;
}

#endif   /* HAVE_LIBCURL */