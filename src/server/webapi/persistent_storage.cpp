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
** File: persistent_storage.cpp
**
**/

#include "webapi.h"
#include <nxcore_ps.h>

/**
 * Maximum key and value lengths (match persistent_storage table column sizes)
 */
#define MAX_PSTORAGE_KEY_LEN     127
#define MAX_PSTORAGE_VALUE_LEN   2000

/**
 * Get persistent storage key from URL placeholder and validate it. On failure writes
 * an error response to the context and returns nullptr. Keys longer than column size
 * are rejected rather than truncated, so that they cannot silently address another entry.
 */
static const wchar_t *GetPersistentStorageKey(Context *context)
{
   const wchar_t *key = context->getPlaceholderValue(L"key");
   if ((key == nullptr) || (key[0] == 0))
   {
      context->setErrorResponse("Persistent storage key cannot be empty");
      return nullptr;
   }
   if (wcslen(key) > MAX_PSTORAGE_KEY_LEN)
   {
      context->setErrorResponse("Persistent storage key is too long");
      return nullptr;
   }
   for(const wchar_t *p = key; *p != 0; p++)
   {
      if (*p < 0x20)
      {
         context->setErrorResponse("Persistent storage key contains invalid characters");
         return nullptr;
      }
   }
   return key;
}

/**
 * Create JSON document for persistent storage entry
 */
static json_t *PersistentStorageEntryToJson(const wchar_t *key, const wchar_t *value)
{
   json_t *output = json_object();
   json_object_set_new(output, "key", json_string_w(key));
   json_object_set_new(output, "value", json_string_w(value));
   return output;
}

/**
 * Handler for GET /v1/persistent-storage
 */
int H_PersistentStorage(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_PERSISTENT_STORAGE))
      return 403;

   json_t *output = GetPersistentStorageListAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/persistent-storage/:key
 */
int H_PersistentStorageEntry(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_PERSISTENT_STORAGE))
      return 403;

   const wchar_t *key = GetPersistentStorageKey(context);
   if (key == nullptr)
      return 400;

   SharedString value = GetPersistentStorageValue(key);
   if (value.isNull())
      return 404;

   json_t *output = PersistentStorageEntryToJson(key, value);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/persistent-storage/:key
 * Creates or updates single entry. Body: { "value": "..." }.
 */
int H_PersistentStorageEntryUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_PERSISTENT_STORAGE))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on persistent storage update");
      return 403;
   }

   const wchar_t *key = GetPersistentStorageKey(context);
   if (key == nullptr)
      return 400;

   json_t *request = context->getRequestDocument();
   if (!json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *jsonValue = json_object_get(request, "value");
   if (!json_is_string(jsonValue))
   {
      context->setErrorResponse("Field \"value\" is missing or is not a string");
      return 400;
   }

   // utf8_wcharlen counts the terminating null
   if (utf8_wcharlen(json_string_value(jsonValue), -1) > MAX_PSTORAGE_VALUE_LEN + 1)
   {
      context->setErrorResponse("Persistent storage value is too long");
      return 400;
   }

   wchar_t *value = WideStringFromUTF8String(json_string_value(jsonValue));
   SharedString oldValue = GetPersistentStorageValue(key);
   SetPersistentStorageValue(key, value);
   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue.isNull() ? nullptr : oldValue.cstr(), value, 'T', L"Persistent storage entry \"%s\" set", key);

   json_t *output = PersistentStorageEntryToJson(key, value);
   MemFree(value);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/persistent-storage/:key
 */
int H_PersistentStorageEntryDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_PERSISTENT_STORAGE))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on persistent storage update");
      return 403;
   }

   const wchar_t *key = GetPersistentStorageKey(context);
   if (key == nullptr)
      return 400;

   SharedString oldValue = GetPersistentStorageValue(key);
   if (!DeletePersistentStorageValue(key))
      return 404;

   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue.cstr(), nullptr, 'T', L"Persistent storage entry \"%s\" deleted", key);
   return 204;
}
