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

static char payload_text[2048] = "";

struct upload_status
{
    size_t bytes_read;
};

static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
    struct upload_status *upload_ctx = (struct upload_status*) userp;
    const char *data;
    size_t room = size * nmemb;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1))
    {
        return 0;
    }

    data = &payload_text[upload_ctx->bytes_read];

    if (data)
    {
        size_t len = strlen(data);
        if (room < len)
            len = room;
        memcpy(ptr, data, len);
        upload_ctx->bytes_read += len;

        return len;
    }

    return 0;
}

LONG H_CheckServiceSMTP(CURL *curl,
        TCHAR *value,
        const OptionList& options)
{
    int retCode = PC_ERR_BAD_PARAMS;

    if(options.exists(_T("from")) &&
       options.exists(_T("to")))
    {
        const int optionMaxSize = 64;
        char from[optionMaxSize] = "";
        wcstombs(from,options.get(_T("from")), optionMaxSize);
        char to[optionMaxSize] = "";
        wcstombs(to,options.get(_T("to")), optionMaxSize);

        time_t currentTime;
        struct tm* pCurrentTM;
        char timeAsChar[64];

        time(&currentTime);
#ifdef HAVE_LOCALTIME_R
        struct tm currentTM;
        localtime_r(&currentTime, &currentTM);
        pCurrentTM = &currentTM;
#else
        pCurrentTM = localtime(&currentTime);
#endif
        strftime(timeAsChar, sizeof(timeAsChar), "%a, %d %b %Y %H:%M:%S %z\r\n", pCurrentTM);

        snprintf(payload_text, sizeof(payload_text), "From: <%s>\r\nTo: <%s>\r\nSubject: NetXMS test mail\r\nDate: %s\r\n\r\nNetXMS test mail\r\n.\r\n",
                 from, to, timeAsChar);

        struct curl_slist *recipients = NULL;
        struct upload_status upload_ctx = { 0 };

        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);
        recipients = curl_slist_append(recipients, to);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        if (curl_easy_perform(curl) == 0)
        {
            retCode = PC_ERR_NONE;
        }
    }

   ret_int(value, retCode);

   return SYSINFO_RC_SUCCESS;
}
