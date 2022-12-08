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

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <netxms-regex.h>
#include <netxms-version.h>
#include <stdlib.h>

#include <curl/curl.h>
#include "netsvc.h"

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb,
        void *userdata)
{
    ByteStream *data = (ByteStream*) userdata;
    size_t bytes = size * nmemb;
    data->write(ptr, bytes);
    return bytes;
}

LONG H_CheckServiceDefault(CURL *curl,
        TCHAR *value)
{
    int retCode = PC_ERR_BAD_PARAMS;

    // Receiving buffer
    ByteStream data(32768);
    data.setAllocationStep(32768);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnCurlDataReceived);

    CURLcode result = curl_easy_perform(curl);
    if (result == 0)
    {
        retCode = PC_ERR_NONE;
    }
    else
    {
        nxlog_debug_tag(DEBUG_TAG, 6, _T("call to curl_easy_perform failed with result %hs"), result);
    }

   ret_int(value, retCode);

   return SYSINFO_RC_SUCCESS;
}
