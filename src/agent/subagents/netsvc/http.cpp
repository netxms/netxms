/*
 ** NetXMS Network Service check subagent
 ** Copyright (C) 2013-2022 Alex Kirhenshtein
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
#include <netxms-version.h>
#include <stdlib.h>

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

/**
 * Handler for Service.Status(url, pattern)
 */
LONG H_CheckServiceHTTP(CURL *curl,
        TCHAR *value,
        const OptionList& options,
        char *url,
        PCRE *compiledPattern)
{
    int retCode = PC_ERR_BAD_PARAMS;

    curl_easy_setopt(curl, CURLOPT_HEADER, static_cast<long>(1)); // include header in data
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
            "Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36");

    // Receiving buffer
    ByteStream data(32768);
    data.setAllocationStep(32768);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnCurlDataReceived);

    char *requestURL = url;

    retry: if (curl_easy_setopt(curl, CURLOPT_URL, requestURL) == CURLE_OK)
    {
        if (curl_easy_perform(curl) == 0)
        {
            long responseCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
            nxlog_debug_tag(DEBUG_TAG, 6,
                    _T(
                            "H_CheckService(%hs): got reply (%u bytes, response code %03ld)"),
                    url, static_cast<uint32_t>(data.size()), responseCode);

            if ((responseCode >= 300) && (responseCode <= 399)
                    && options.exists(_T("follow-location")))
            {
                char *redirectURL = nullptr;
                curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL,
                        &redirectURL);
                if (redirectURL != nullptr)
                {
                    requestURL = redirectURL;
                    nxlog_debug_tag(DEBUG_TAG, 6,
                            _T(
                                    "H_CheckService(%hs): follow redirect to %hs"),
                            url, redirectURL);
                    data.clear();
                    goto retry;
                }
            }

            if (data.size() > 0)
            {
                data.write('\0');
                size_t size;
                int pmatch[30];
#ifdef UNICODE
           WCHAR *wtext = WideStringFromUTF8String((char *)data.buffer(&size));
           if (_pcre_exec_t(compiledPattern, NULL, reinterpret_cast<const PCRE_TCHAR*>(wtext), static_cast<int>(wcslen(wtext)), 0, 0, pmatch, 30) >= 0)
#else
                char *text = (char*) data.buffer(&size);
                if (pcre_exec(compiledPattern, NULL, text,
                        static_cast<int>(size), 0, 0, pmatch, 30) >= 0)
#endif
                {
                    nxlog_debug_tag(DEBUG_TAG, 5,
                            _T("H_CheckService(%hs): matched"), url);
                    retCode = PC_ERR_NONE;
                }
                else
                {
                    nxlog_debug_tag(DEBUG_TAG, 5,
                            _T("H_CheckService(%hs): not matched"), url);
                    retCode = PC_ERR_NOMATCH;
                }
#ifdef UNICODE
           MemFree(wtext);
#endif
            }
            else
            {
                // zero size reply
                retCode = PC_ERR_NOMATCH;
            }
        }
        else
        {
            nxlog_debug_tag(DEBUG_TAG, 6,
                    _T(
                            "H_CheckService(%hs): call to curl_easy_perform failed"),
                    url);
            retCode = PC_ERR_CONNECT;
        }
    }

    _pcre_free_t(compiledPattern);

    ret_int(value, retCode);
    return SYSINFO_RC_SUCCESS;
}
