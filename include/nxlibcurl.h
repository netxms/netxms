/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: nxlibcurl.h
**
**/

#ifndef _nxlibcurl_h_
#define _nxlibcurl_h_

#include <curl/curl.h>

#ifndef _WIN32
#include <openssl/ssl.h>
#endif

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

bool LIBNETXMS_EXPORTABLE InitializeLibCURL();
const char LIBNETXMS_EXPORTABLE *GetLibCURLVersion();

#if !defined(_WIN32) && defined(SSL_OP_IGNORE_UNEXPECTED_EOF)

extern LIBNETXMS_EXPORTABLE_VAR(bool g_curlOpenSSL3Backend);

static CURLcode __nx_SSL_CTX_setup_cb(CURL *curl, void *sslctx, void *context)
{
   SSL_CTX_set_options(static_cast<SSL_CTX*>(sslctx), SSL_OP_IGNORE_UNEXPECTED_EOF);
   return CURLE_OK;
}

/**
 * Enable workaround for "unexpected EOF" errors
 * See https://github.com/curl/curl/issues/9024 and SSL_OP_IGNORE_UNEXPECTED_EOF in https://www.openssl.org/docs/man3.0/man3/SSL_CTX_set_options.html
 */
#define EnableLibCURLUnexpectedEOFWorkaround(curl) \
do \
{ \
   if (g_curlOpenSSL3Backend) \
      curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, __nx_SSL_CTX_setup_cb); \
} while(0)

#else

#define EnableLibCURLUnexpectedEOFWorkaround(curl)

#endif

#endif   /* _nxlibcurl_h_ */
