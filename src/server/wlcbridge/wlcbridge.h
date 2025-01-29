/*
** NetXMS - Network Management System
** Copyright (C) 2023-2025 Raden Solutions
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
** File: wlcbridge.h
**
**/

#ifndef _wlcbridge_h_
#define _wlcbridge_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <nxmodule.h>
#include <nxlibcurl.h>

#define WLCBRIDGE_DEBUG_TAG   _T("wlcbridge")

/**
 * Cache entry for access point data
 */
struct AccessPointCacheEntry
{
   json_t *data;
   time_t timestamp;
   volatile bool processing;

   AccessPointCacheEntry()
   {
      data = nullptr;
      timestamp = 0;
      processing = false;
   }

   ~AccessPointCacheEntry()
   {
      json_decref(data);
   }
};

/**
 * Helper function to extract DCI value from JSON document
 */
static inline DataCollectionError GetValueFromJson(json_t *document, const TCHAR *path, TCHAR *value, size_t size)
{
   json_t *data = json_object_get_by_path(document, path);
   if (data == nullptr)
      return DCE_NOT_SUPPORTED;

   TCHAR buffer[32];
   switch(json_typeof(data))
   {
      case JSON_STRING:
         utf8_to_tchar(json_string_value(data), -1, value, size);
         value[size - 1] = 0;
         break;
      case JSON_INTEGER:
         IntegerToString(static_cast<int64_t>(json_integer_value(data)), buffer);
         _tcslcpy(value, buffer, size);
         break;
      case JSON_REAL:
         _sntprintf(value, size, _T("%f"), json_real_value(data));
         break;
      case JSON_TRUE:
         _tcslcpy(value, _T("true"), size);
         break;
      case JSON_FALSE:
         _tcslcpy(value, _T("false"), size);
         break;
      default:
         *value = 0;
         break;
   }
   return DCE_SUCCESS;
}

/**
 * Create CURL handle with default configuration
 */
CURL *CreateCurlHandle(ByteStream *responseData, char *errorBuffer);

#endif   /* _wlcbridge_h_ */
