/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
 * Service check sub-handler - other protocols
 */
LONG NetworkServiceStatus_Other(CURL *curl, const OptionList& options, const char *url, int *result)
{
    // Receiving buffer
    ByteStream data(32768);
    data.setAllocationStep(32768);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);

    char errorText[CURL_ERROR_SIZE] = "";
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorText);

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK)
    {
       nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkServiceStatus_Other(%hs): call to curl_easy_perform failed (%d: %hs)"), url, rc, errorText);
    }

    *result = CURLCodeToCheckResult(rc);
   return SYSINFO_RC_SUCCESS;
}
