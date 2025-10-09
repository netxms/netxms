/**
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for InfluxDB
 ** Copyright (C) 2019 Sebastian YEPES FERNANDEZ & Julien DERIVIERE
 ** Copyright (C) 2021-2025 Raden Solutions
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
 **              ,.-----__
 **          ,:::://///,:::-.
 **         /:''/////// ``:::`;/|/
 **        /'   ||||||     :://'`\
 **      .' ,   ||||||     `/(  e \
 **-===~__-'\__X_`````\_____/~`-._ `.
 **            ~~        ~~       `~-'
 ** Armadillo, The Metric Eater - https://www.nationalgeographic.com/animals/mammals/group/armadillos/
 **
 ** File: api.cpp
 **/

#include "influxdb.h"
#include <netxms-version.h>

#define MAX_URL_LENGTH  4096

/**
 * Constructor for Generic API sender
 */
APISender::APISender(const Config& config) : InfluxDBSender(config)
{
   if (m_port == 0)
      m_port = 8086;
   m_curl = nullptr;
}

/**
 * Destructor for generic API sender
 */
APISender::~APISender()
{
   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);
}

/**
 * Send data block via API
 */
bool APISender::send(const char *data, size_t size)
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
      curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "NetXMS InfluxDB Driver/" NETXMS_VERSION_STRING_A);

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
      // Interpret partial writes and unrecoverable errors as success because retry won't help anyway
      // Retry only on 429 (Token is temporarily over quota) and 503 (Serer is temporarily unavailable to accept writes)
      success = (responseCode != 429) && (responseCode != 503);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to curl_easy_perform() failed (%hs)"), errorText);
      success = false;
   }

   curl_slist_free_all(headers);
   return success;
}

/**
 * Default addHeaders implementation - do nothing
 */
void APISender::addHeaders(curl_slist **headers)
{
}

/**
 * Constructor for API v1 sender
 */
APIv1Sender::APIv1Sender(const Config& config) : APISender(config), m_db(config.getValue(_T("/InfluxDB/Database"), _T("netxms"))),
         m_user(config.getValue(_T("/InfluxDB/User"), _T(""))), m_password(config.getValue(_T("/InfluxDB/Password"), _T("")))
{
}

/**
 * Destructor for API v1 sender
 */
APIv1Sender::~APIv1Sender()
{
}

/**
 * Build API v1 URL
 */
void APIv1Sender::buildURL(char *url)
{
   StringBuffer sb(_T("http://"));
   sb.append(m_hostname);
   sb.append(_T(':'));
   sb.append(m_port);
   sb.append(_T("/write?precision=ns&db="));
   sb.append(m_db);
   if (!m_user.isEmpty())
   {
      sb.append(_T("&user="));
      sb.append(m_user);
   }
   if (!m_password.isEmpty())
   {
      sb.append(_T("&password="));
      TCHAR decryptedPassword[256];
      DecryptPassword(m_user, m_password, decryptedPassword, 256);
      sb.append(decryptedPassword);
   }
   tchar_to_utf8(sb, -1, url, MAX_URL_LENGTH);
}

/**
 * Constructor for API v2 sender
 */
APIv2Sender::APIv2Sender(const Config& config) : APISender(config),
         m_organization(config.getValue(_T("/InfluxDB/Organization"), _T(""))), m_bucket(config.getValue(_T("/InfluxDB/Bucket"), _T("")))
{
   m_token = UTF8StringFromTString(config.getValue(_T("/InfluxDB/Token"), _T("")));
}

/**
 * Destructor for API v2 sender
 */
APIv2Sender::~APIv2Sender()
{
   MemFree(m_token);
}

/**
 * Build API v2 URL
 */
void APIv2Sender::buildURL(char *url)
{
   StringBuffer sb(_T("http://"));
   sb.append(m_hostname);
   sb.append(_T(':'));
   sb.append(m_port);
   sb.append(_T("/api/v2/write?precision=ns"));
   if (!m_organization.isEmpty())
   {
      sb.append(_T("&org="));
      sb.append(m_organization);
   }
   if (!m_bucket.isEmpty())
   {
      sb.append(_T("&bucket="));
      sb.append(m_bucket);
   }
   tchar_to_utf8(sb, -1, url, MAX_URL_LENGTH);
}

/**
 * API v2 headers
 */
void APIv2Sender::addHeaders(curl_slist **headers)
{
   if (*m_token != 0)
   {
      char auth[1024];
      snprintf(auth, 1024, "Authorization: Token %s", m_token);
      *headers = curl_slist_append(*headers, auth);
   }
}
