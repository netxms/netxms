/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    ByteStream *data = (ByteStream*) userdata;
    size_t bytes = size * nmemb;
    data->write(ptr, bytes);
    return bytes;
}

/**
 * Service check sub-handler - other protocols
 */
LONG NetworkServiceStatus_Other(CURL *curl, const OptionList& options, int *result)
{
    // Receiving buffer
    ByteStream data(32768);
    data.setAllocationStep(32768);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnCurlDataReceived);

    *result = CURLCodeToCheckResult(curl_easy_perform(curl));
   return SYSINFO_RC_SUCCESS;
}
