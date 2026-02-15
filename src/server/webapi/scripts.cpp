/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: scripts.cpp
**
**/

#include "webapi.h"
#include <nms_script.h>

/**
 * Handler for GET /v1/script-library
 */
int H_ScriptLibrary(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SCRIPTS))
      return 403;

   json_t *output = json_array();

   NXSL_Library *library = GetServerScriptLibrary();
   library->lock();
   library->forEach([output](const NXSL_LibraryScript *script) {
      json_t *entry = json_object();
      json_object_set_new(entry, "id", json_integer(script->getId()));
      json_object_set_new(entry, "name", json_string_t(script->getName()));
      json_array_append_new(output, entry);
   });
   library->unlock();

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/script-library/:script-id
 */
int H_ScriptDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SCRIPTS))
      return 403;

   uint32_t scriptId = context->getPlaceholderValueAsUInt32(L"script-id");
   if (scriptId == 0)
      return 400;

   NXSL_Library *library = GetServerScriptLibrary();
   library->lock();
   NXSL_LibraryScript *script = library->findScript(scriptId);
   if (script == nullptr)
   {
      library->unlock();
      return 404;
   }

   json_t *output = json_object();
   json_object_set_new(output, "id", json_integer(script->getId()));
   json_object_set_new(output, "name", json_string_t(script->getName()));
   json_object_set_new(output, "code", json_string_t(script->getSourceCode()));
   library->unlock();

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/script-library
 */
int H_ScriptCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SCRIPTS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonName = json_object_get(request, "name");
   json_t *jsonCode = json_object_get(request, "code");
   if (!json_is_string(jsonName) || !json_is_string(jsonCode))
   {
      context->setErrorResponse("Missing or invalid name and/or code fields");
      return 400;
   }

   wchar_t name[MAX_DB_STRING];
   utf8_to_wchar(json_string_value(jsonName), -1, name, MAX_DB_STRING);
   const char *code = json_string_value(jsonCode);

   uint32_t scriptId = 0;
   uint32_t rcc = UpdateScript(name, code, &scriptId, context);

   int responseCode;
   if (rcc == RCC_SUCCESS)
   {
      json_t *output = json_object();
      json_object_set_new(output, "id", json_integer(scriptId));
      context->setResponseData(output);
      json_decref(output);
      responseCode = 201;
   }
   else if (rcc == RCC_INVALID_SCRIPT_NAME)
   {
      context->setErrorResponse("Invalid script name");
      responseCode = 400;
   }
   else
   {
      context->setErrorResponse("Database failure");
      responseCode = 500;
   }

   return responseCode;
}

/**
 * Handler for PUT /v1/script-library/:script-id
 */
int H_ScriptUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SCRIPTS))
      return 403;

   uint32_t scriptId = context->getPlaceholderValueAsUInt32(L"script-id");
   if (scriptId == 0)
      return 400;

   if (!IsValidScriptId(scriptId))
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonName = json_object_get(request, "name");
   json_t *jsonCode = json_object_get(request, "code");
   if (!json_is_string(jsonName) || !json_is_string(jsonCode))
   {
      context->setErrorResponse("Missing or invalid name and/or code fields");
      return 400;
   }

   wchar_t name[MAX_DB_STRING];
   utf8_to_wchar(json_string_value(jsonName), -1, name, MAX_DB_STRING);
   const char *code = json_string_value(jsonCode);

   // Rename if needed, then update code
   wchar_t oldName[MAX_DB_STRING];
   GetScriptName(scriptId, oldName, MAX_DB_STRING);

   int responseCode;
   if (wcscmp(oldName, name) != 0)
   {
      uint32_t rcc = RenameScript(scriptId, name);
      if (rcc == RCC_INVALID_SCRIPT_NAME)
      {
         context->setErrorResponse("Invalid script name");
         return 400;
      }
      else if (rcc != RCC_SUCCESS)
      {
         context->setErrorResponse("Database failure");
         return 500;
      }
   }

   uint32_t rcc = UpdateScript(name, code, &scriptId, context);
   if (rcc == RCC_SUCCESS)
   {
      json_t *output = json_object();
      json_object_set_new(output, "id", json_integer(scriptId));
      context->setResponseData(output);
      json_decref(output);
      responseCode = 200;
   }
   else
   {
      context->setErrorResponse("Database failure");
      responseCode = 500;
   }

   return responseCode;
}

/**
 * Handler for DELETE /v1/script-library/:script-id
 */
int H_ScriptDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SCRIPTS))
      return 403;

   uint32_t scriptId = context->getPlaceholderValueAsUInt32(L"script-id");
   if (scriptId == 0)
      return 400;

   wchar_t scriptName[MAX_DB_STRING];
   if (!GetScriptName(scriptId, scriptName, MAX_DB_STRING))
      return 404;

   uint32_t rcc = DeleteScript(scriptId);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Library script %s [%u] deleted", scriptName, scriptId);
      return 204;
   }
   if (rcc == RCC_INVALID_SCRIPT_ID)
   {
      return 404;
   }
   context->setErrorResponse("Database failure");
   return 500;
}
