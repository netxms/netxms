/*
** NetXMS - Network Management System
** Copyright (C) 2023-2026 Raden Solutions
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
** File: webapi_context.cpp
**
**/

#include "nxcore.h"
#include <netxms-webapi.h>
#include <zlib.h>

/**
 * Get request JSON document.
 */
json_t *Context::getRequestDocument()
{
   if (!hasRequestData())
      return nullptr;
   if (m_requestDocument == nullptr)
   {
      json_error_t error;
      m_requestDocument = json_loads(getRequestData(), 0, &error);
      if (m_requestDocument == nullptr)
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Cannot parse request document (%hs)"), error.text);
   }
   return m_requestDocument;
}

/**
 * Get placeholder value as uint32. Allows decimal and hexadecimal forms.
 */
const uint32_t Context::getPlaceholderValueAsUInt32(const TCHAR *name, uint32_t defaultValue) const
{
   const TCHAR *v = m_placeholderValues.get(name);
   if (v == nullptr)
      return defaultValue;
   TCHAR *eptr;
   bool hex = (_tcsncmp(v, _T("0x"), 2) == 0);
   uint32_t n = _tcstoul(hex ? &v[2] : v, &eptr, hex ? 16 : 10);
   return (*eptr == 0) ? n : defaultValue;
}

/**
 * Get query parameter as int32. Allows decimal and hexadecimal forms.
 */
int32_t Context::getQueryParameterAsInt32(const char *name, int32_t defaultValue) const
{
   const char *v = getQueryParameter(name);
   if (v == nullptr)
      return defaultValue;
   char *eptr;
   bool hex = (strncmp(v, "0x", 2) == 0);
   int32_t n = strtol(hex ? &v[2] : v, &eptr, hex ? 16 : 10);
   return (*eptr == 0) ? n : defaultValue;
}

/**
 * Get query parameter as uint32. Allows decimal and hexadecimal forms.
 */
uint32_t Context::getQueryParameterAsUInt32(const char *name, uint32_t defaultValue) const
{
   const char *v = getQueryParameter(name);
   if (v == nullptr)
      return defaultValue;
   char *eptr;
   bool hex = (strncmp(v, "0x", 2) == 0);
   uint32_t n = strtoul(hex ? &v[2] : v, &eptr, hex ? 16 : 10);
   return (*eptr == 0) ? n : defaultValue;
}

/**
 * Get query parameter as time value
 */
time_t Context::getQueryParameterAsTime(const char *name, time_t defaultValue) const
{
   const char *v = getQueryParameter(name);
   if (v == nullptr)
      return defaultValue;

   char *eptr;
   int64_t n = strtoll(v, &eptr, 10);
   if (*eptr == 0)
      return static_cast<time_t>(n);   // Assume UNIX timestamp

   struct tm t;
   if (strptime(v, "%Y-%m-%dT%H:%M:%SZ", &t) == nullptr)
      return defaultValue;

   return timegm(&t);
}

/**
 * Set error response from agent error
 */
void Context::setAgentErrorResponse(uint32_t agentErrorCode)
{
   json_t *response = json_object();
   json_object_set_new(response, "reason", json_string("Agent error"));
   json_object_set_new(response, "agentErrorCode", json_integer(agentErrorCode));
   json_object_set_new(response, "agentErrorText", json_string_t(AgentErrorCodeToText(agentErrorCode)));
   setResponseData(response);
   json_decref(response);
}

/**
 * Write audit log
 */
void Context::writeAuditLog(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *format, ...) const
{
   va_list args;
   va_start(args, format);
   WriteAuditLog2(subsys, success, m_userId, m_workstation, 0, objectId, format, args);
   va_end(args);
}

/**
 * Write audit log with old and new values for changed entity
 */
void Context::writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...) const
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithValues2(subsys, success, m_userId, m_workstation, 0, objectId, oldValue, newValue, valueType, format, args);
   va_end(args);
}

/**
 * Write audit log with old and new values for changed entity
 */
void Context::writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...) const
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithJsonValues2(subsys, success, m_userId, m_workstation, 0, objectId, oldValue, newValue, format, args);
   va_end(args);
}

/**
 * Upload data block handler
 */
bool Context::onUploadData(const char *data, size_t size)
{
   if (m_requestData.size() + size > MAX_WEBAPI_REQUEST_SIZE)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("Request body exceeds maximum size limit (%u bytes)"), MAX_WEBAPI_REQUEST_SIZE);
      return false;
   }
   m_requestData.write(data, size);
   return true;
}

/**
 * Decompress request body (gzip or zlib/deflate) in place. Uses automatic header
 * detection so both "gzip" and "deflate" Content-Encoding values are handled.
 * Decompressed output is capped at MAX_WEBAPI_REQUEST_SIZE to guard against
 * decompression bombs. Returns true on success.
 */
static bool InflateRequestData(ByteStream& data)
{
   size_t inputSize;
   const BYTE *input = data.buffer(&inputSize);
   if (inputSize == 0)
      return true;

   z_stream stream;
   memset(&stream, 0, sizeof(stream));
   if (inflateInit2(&stream, 15 + 32) != Z_OK)   // 32 enables automatic gzip/zlib header detection
      return false;

   stream.next_in = const_cast<BYTE*>(input);
   stream.avail_in = static_cast<uInt>(inputSize);

   ByteStream output(inputSize * 4);
   output.setAllocationStep(32768);

   BYTE chunk[32768];
   int rc;
   do
   {
      stream.next_out = chunk;
      stream.avail_out = sizeof(chunk);
      rc = inflate(&stream, Z_NO_FLUSH);
      if ((rc != Z_OK) && (rc != Z_STREAM_END) && (rc != Z_BUF_ERROR))
      {
         inflateEnd(&stream);
         return false;
      }

      size_t produced = sizeof(chunk) - stream.avail_out;
      if (produced > 0)
      {
         if (output.size() + produced > MAX_WEBAPI_REQUEST_SIZE)
         {
            nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("Decompressed request body exceeds maximum size limit (%u bytes)"), MAX_WEBAPI_REQUEST_SIZE);
            inflateEnd(&stream);
            return false;
         }
         output.write(chunk, produced);
      }
      else if (rc == Z_BUF_ERROR)
      {
         break;   // no progress possible (truncated input) - stop and fail below
      }
   } while (rc != Z_STREAM_END);

   inflateEnd(&stream);

   if (rc != Z_STREAM_END)
      return false;   // incomplete or corrupt stream

   data.clear();
   size_t outputSize;
   const BYTE *outputBuffer = output.buffer(&outputSize);
   data.write(outputBuffer, outputSize);
   return true;
}

/**
 * Finalize request body once fully received: transparently decompress it if the
 * client used a supported Content-Encoding, then null-terminate for text/JSON consumers.
 */
void Context::onUploadComplete()
{
   if (m_requestData.size() == 0)
      return;

   const char *encoding = getRequestHeader(MHD_HTTP_HEADER_CONTENT_ENCODING);
   if ((encoding != nullptr) && (*encoding != 0) && stricmp(encoding, "identity"))
   {
      if (!stricmp(encoding, "gzip") || !stricmp(encoding, "x-gzip") || !stricmp(encoding, "deflate"))
      {
         size_t compressedSize = m_requestData.size();
         if (!InflateRequestData(m_requestData))
         {
            nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("Cannot decompress Web API request body (Content-Encoding: %hs)"), encoding);
            m_requestDecodingFailed = true;
            return;
         }
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Web API request body decompressed (%u -> %u bytes, Content-Encoding: %hs)"),
            static_cast<uint32_t>(compressedSize), static_cast<uint32_t>(m_requestData.size()), encoding);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("Unsupported Content-Encoding \"%hs\" in Web API request"), encoding);
         m_requestDecodingFailed = true;
         return;
      }
   }

   m_requestData.write('\0');
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Web API request data received (%u bytes)"),
      static_cast<uint32_t>(m_requestData.size() - 1));
}
