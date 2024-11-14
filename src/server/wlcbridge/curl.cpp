/*
** NetXMS - Network Management System
** Copyright (C) 2023-2024 Raden Solutions
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
** File: ruckus.cpp
**
**/

#include "wlcbridge.h"
#include <netxms-version.h>

#define DEBUG_TAG WLCBRIDGE_DEBUG_TAG _T(".curl")

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *context)
{
   size_t bytes = size * nmemb;
   static_cast<ByteStream*>(context)->write(ptr, bytes);
   return bytes;
}

/**
 * Create CURL handle with common setup
 */
CURL *CreateCurlHandle(ByteStream *responseData, char *errorBuffer)
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return nullptr;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)0);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS WLC Bridge/" NETXMS_VERSION_STRING_A);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseData);
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   return curl;
}
