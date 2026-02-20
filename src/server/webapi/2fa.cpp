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
** File: 2fa.cpp
**
**/

#include "webapi.h"
#include <nxcore_2fa.h>
#include <nms_users.h>

/**
 * Handler for GET /v1/2fa/drivers
 */
int H_2FADrivers(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_2FA_METHODS))
      return 403;

   json_t *output = Get2FADriversAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/2fa/methods
 */
int H_2FAMethods(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_2FA_METHODS))
      return 403;

   json_t *output = Get2FAMethodsAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/2fa/methods/:method-name
 */
int H_2FAMethodDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_2FA_METHODS))
      return 403;

   const TCHAR *methodName = context->getPlaceholderValue(L"method-name");
   if (methodName == nullptr)
      return 400;

   json_t *output = Get2FAMethodAsJson(methodName);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/2fa/methods
 */
int H_2FAMethodCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_2FA_METHODS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonName = json_object_get(request, "name");
   json_t *jsonDriver = json_object_get(request, "driver");
   if (!json_is_string(jsonName) || !json_is_string(jsonDriver))
   {
      context->setErrorResponse("Missing or invalid required fields (name, driver)");
      return 400;
   }

   wchar_t name[MAX_OBJECT_NAME], driver[MAX_OBJECT_NAME], description[MAX_2FA_DESCRIPTION];
   utf8_to_wchar(json_string_value(jsonName), -1, name, MAX_OBJECT_NAME);
   utf8_to_wchar(json_string_value(jsonDriver), -1, driver, MAX_OBJECT_NAME);

   json_t *jsonDescription = json_object_get(request, "description");
   if (json_is_string(jsonDescription))
      utf8_to_wchar(json_string_value(jsonDescription), -1, description, MAX_2FA_DESCRIPTION);
   else
      description[0] = 0;

   if (Is2FAMethodExists(name))
   {
      context->setErrorResponse("Two-factor authentication method with this name already exists");
      return 409;
   }

   char *configuration = nullptr;
   json_t *jsonConfig = json_object_get(request, "configuration");
   if (json_is_object(jsonConfig))
      configuration = json_dumps(jsonConfig, 0);
   else
      configuration = MemCopyStringA("");

   uint32_t rcc = Modify2FAMethod(name, driver, description, configuration);
   if (rcc != RCC_SUCCESS)
   {
      if (rcc == RCC_NO_SUCH_2FA_DRIVER)
      {
         context->setErrorResponse("Unknown two-factor authentication driver");
         return 400;
      }
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Two-factor authentication method \"%s\" created", name);

   json_t *output = Get2FAMethodAsJson(name);
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for PUT /v1/2fa/methods/:method-name
 */
int H_2FAMethodUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_2FA_METHODS))
      return 403;

   const TCHAR *methodName = context->getPlaceholderValue(L"method-name");
   if (methodName == nullptr)
      return 400;

   json_t *current = Get2FAMethodAsJson(methodName);
   if (current == nullptr)
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      json_decref(current);
      return 400;
   }

   // Use current values as defaults, override with request values
   json_t *jsonDriver = json_object_get(request, "driver");
   const char *driverUtf8 = json_is_string(jsonDriver) ? json_string_value(jsonDriver) : json_string_value(json_object_get(current, "driver"));

   json_t *jsonDescription = json_object_get(request, "description");
   const char *descriptionUtf8 = json_is_string(jsonDescription) ? json_string_value(jsonDescription) : json_string_value(json_object_get(current, "description"));

   wchar_t driver[MAX_OBJECT_NAME], description[MAX_2FA_DESCRIPTION];
   utf8_to_wchar(driverUtf8, -1, driver, MAX_OBJECT_NAME);
   utf8_to_wchar(descriptionUtf8, -1, description, MAX_2FA_DESCRIPTION);

   char *configuration = nullptr;
   json_t *jsonConfig = json_object_get(request, "configuration");
   if (json_is_object(jsonConfig))
      configuration = json_dumps(jsonConfig, 0);
   else
      configuration = json_dumps(json_object_get(current, "configuration"), 0);

   json_decref(current);

   uint32_t rcc = Modify2FAMethod(methodName, driver, description, configuration);
   if (rcc != RCC_SUCCESS)
   {
      if (rcc == RCC_NO_SUCH_2FA_DRIVER)
      {
         context->setErrorResponse("Unknown two-factor authentication driver");
         return 400;
      }
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Two-factor authentication method \"%s\" updated", methodName);

   json_t *output = Get2FAMethodAsJson(methodName);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/2fa/methods/:method-name
 */
int H_2FAMethodDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_2FA_METHODS))
      return 403;

   const TCHAR *methodName = context->getPlaceholderValue(L"method-name");
   if (methodName == nullptr)
      return 400;

   uint32_t rcc = Delete2FAMethod(methodName);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Two-factor authentication method \"%s\" deleted", methodName);
      return 204;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Check access for user 2FA binding operations
 */
static bool CheckUser2FABindingAccess(Context *context, uint32_t userId)
{
   return (userId == context->getUserId()) || context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS);
}

/**
 * Handler for GET /v1/users/:user-id/2fa-bindings
 */
int H_User2FABindings(Context *context)
{
   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");
   if (userId == 0)
      return 400;

   if (!CheckUser2FABindingAccess(context, userId))
      return 403;

   json_t *output = GetUser2FABindingsAsJson(userId);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/users/:user-id/2fa-bindings/:method-name
 */
int H_User2FABindingUpdate(Context *context)
{
   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");
   if (userId == 0)
      return 400;

   if (!CheckUser2FABindingAccess(context, userId))
      return 403;

   const TCHAR *methodName = context->getPlaceholderValue(L"method-name");
   if (methodName == nullptr)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   StringMap configuration;
   const char *key;
   json_t *value;
   json_object_foreach(request, key, value)
   {
      if (json_is_string(value))
      {
         wchar_t wKey[256], wValue[1024];
         utf8_to_wchar(key, -1, wKey, 256);
         utf8_to_wchar(json_string_value(value), -1, wValue, 1024);
         configuration.set(wKey, wValue);
      }
   }

   uint32_t rcc = ModifyUser2FAMethodBinding(userId, methodName, configuration);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Two-factor authentication binding \"%s\" updated for user %u", methodName, userId);
      return 200;
   }

   if (rcc == RCC_INVALID_USER_ID)
      return 404;
   if (rcc == RCC_NO_SUCH_2FA_METHOD)
   {
      context->setErrorResponse("Unknown two-factor authentication method");
      return 404;
   }

   context->setErrorResponse("Internal error");
   return 500;
}

/**
 * Handler for DELETE /v1/users/:user-id/2fa-bindings/:method-name
 */
int H_User2FABindingDelete(Context *context)
{
   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");
   if (userId == 0)
      return 400;

   if (!CheckUser2FABindingAccess(context, userId))
      return 403;

   const TCHAR *methodName = context->getPlaceholderValue(L"method-name");
   if (methodName == nullptr)
      return 400;

   uint32_t rcc = DeleteUser2FAMethodBinding(userId, methodName);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Two-factor authentication binding \"%s\" deleted for user %u", methodName, userId);
      return 204;
   }

   if (rcc == RCC_INVALID_USER_ID)
      return 404;
   if (rcc == RCC_NO_SUCH_2FA_BINDING)
      return 404;

   context->setErrorResponse("Internal error");
   return 500;
}
