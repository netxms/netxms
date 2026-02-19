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
** File: users.cpp
**
**/

#include "webapi.h"
#include <nms_users.h>

/**
 * Handler for GET /v1/users
 */
int H_Users(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   json_t *output = json_array();
   Iterator<UserDatabaseObject> it = OpenUserDatabase();
   while(it.hasNext())
   {
      UserDatabaseObject *object = it.next();
      if (!object->isGroup() && !object->isDeleted())
         json_array_append_new(output, object->toJson());
   }
   CloseUserDatabase();

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/users/:user-id
 */
int H_UserDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");

   IntegerArray<uint32_t> ids(1);
   ids.add(userId);
   unique_ptr<ObjectArray<UserDatabaseObject>> result = FindUserDBObjects(ids);
   if (result == nullptr || result->isEmpty())
      return 404;

   UserDatabaseObject *object = result->get(0);
   if (object->isGroup() || object->isDeleted())
      return 404;

   json_t *output = object->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/users
 */
int H_UserCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *nameField = json_object_get(request, "name");
   if (!json_is_string(nameField))
   {
      context->setErrorResponse("Missing or invalid \"name\" field");
      return 400;
   }

   TCHAR *name = json_object_get_string_t(request, "name", nullptr);
   if (!IsValidObjectName(name))
   {
      MemFree(name);
      context->setErrorResponse("Invalid user name");
      return 400;
   }

   uint32_t userId;
   uint32_t rcc = CreateNewUser(name, false, &userId);
   MemFree(name);

   if (rcc == RCC_OBJECT_ALREADY_EXISTS)
   {
      context->setErrorResponse("User with this name already exists");
      return 409;
   }
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SECURITY, true, 0, L"User [%u] created", userId);

   IntegerArray<uint32_t> ids(1);
   ids.add(userId);
   unique_ptr<ObjectArray<UserDatabaseObject>> result = FindUserDBObjects(ids);
   if (result != nullptr && !result->isEmpty())
   {
      json_t *output = result->get(0)->toJson();
      context->setResponseData(output);
      json_decref(output);
   }
   return 201;
}

/**
 * Handler for PUT /v1/users/:user-id
 */
int H_UserUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");
   if (userId & GROUP_FLAG)
   {
      context->setErrorResponse("Invalid user ID");
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *oldValue = nullptr, *newValue = nullptr;
   uint32_t rcc = ModifyUserDatabaseObjectFromJson(userId, request, &oldValue, &newValue);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLogWithValues(AUDIT_SECURITY, true, 0, oldValue, newValue, L"User [%u] updated", userId);
      json_decref(oldValue);

      context->setResponseData(newValue);
      json_decref(newValue);
      return 200;
   }

   if (oldValue != nullptr)
      json_decref(oldValue);
   if (newValue != nullptr)
      json_decref(newValue);

   if (rcc == RCC_INVALID_USER_ID)
      return 404;
   if (rcc == RCC_INVALID_OBJECT_NAME)
   {
      context->setErrorResponse("Invalid user name");
      return 400;
   }
   if (rcc == RCC_OBJECT_ALREADY_EXISTS)
   {
      context->setErrorResponse("User with this name already exists");
      return 409;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Handler for DELETE /v1/users/:user-id
 */
int H_UserDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");
   if (userId & GROUP_FLAG)
   {
      context->setErrorResponse("Invalid user ID");
      return 400;
   }

   if (userId == 0)
   {
      context->setErrorResponse("Cannot delete system user");
      return 400;
   }

   if (IsLoggedIn(userId))
   {
      context->setErrorResponse("User is currently logged in");
      return 409;
   }

   TCHAR userName[MAX_USER_NAME];
   ResolveUserId(userId, userName, true);

   uint32_t rcc = DeleteUserDatabaseObject(userId);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SECURITY, true, 0, L"User %s [%u] deleted", userName, userId);
      return 204;
   }

   if (rcc == RCC_INVALID_USER_ID)
      return 404;

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Handler for POST /v1/users/:user-id/password
 */
int H_UserSetPassword(Context *context)
{
   uint32_t userId = context->getPlaceholderValueAsUInt32(L"user-id");
   if (userId & GROUP_FLAG)
   {
      context->setErrorResponse("Invalid user ID");
      return 400;
   }

   bool isSelf = (context->getUserId() == userId);
   if (!isSelf && !context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *newPassField = json_object_get(request, "newPassword");
   if (!json_is_string(newPassField))
   {
      context->setErrorResponse("Missing or invalid \"newPassword\" field");
      return 400;
   }

#ifdef UNICODE
   WCHAR *newPassword = WideStringFromUTF8String(json_string_value(newPassField));
#else
   char *newPassword = MBStringFromUTF8String(json_string_value(newPassField));
#endif

   TCHAR *oldPassword = nullptr;
   json_t *oldPassField = json_object_get(request, "oldPassword");
   if (oldPassField != nullptr && json_is_string(oldPassField))
   {
#ifdef UNICODE
      oldPassword = WideStringFromUTF8String(json_string_value(oldPassField));
#else
      oldPassword = MBStringFromUTF8String(json_string_value(oldPassField));
#endif
   }

   uint32_t rcc = SetUserPassword(userId, newPassword, oldPassword, isSelf);
   MemFree(newPassword);
   MemFree(oldPassword);

   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SECURITY, true, 0, L"Password changed for user [%u]", userId);
      return 200;
   }

   if (rcc == RCC_INVALID_USER_ID)
      return 404;
   if (rcc == RCC_ACCESS_DENIED)
      return 403;
   if (rcc == RCC_WEAK_PASSWORD)
   {
      context->setErrorResponse("Password does not meet complexity requirements");
      return 400;
   }
   if (rcc == RCC_REUSED_PASSWORD)
   {
      context->setErrorResponse("Password was recently used");
      return 400;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Handler for GET /v1/user-groups
 */
int H_UserGroups(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   json_t *output = json_array();
   Iterator<UserDatabaseObject> it = OpenUserDatabase();
   while(it.hasNext())
   {
      UserDatabaseObject *object = it.next();
      if (object->isGroup() && !object->isDeleted())
         json_array_append_new(output, object->toJson());
   }
   CloseUserDatabase();

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/user-groups/:group-id
 */
int H_UserGroupDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   uint32_t groupId = context->getPlaceholderValueAsUInt32(L"group-id");
   if (!(groupId & GROUP_FLAG))
   {
      context->setErrorResponse("Invalid group ID");
      return 400;
   }

   IntegerArray<uint32_t> ids(1);
   ids.add(groupId);
   unique_ptr<ObjectArray<UserDatabaseObject>> result = FindUserDBObjects(ids);
   if (result == nullptr || result->isEmpty())
      return 404;

   UserDatabaseObject *object = result->get(0);
   if (!object->isGroup() || object->isDeleted())
      return 404;

   json_t *output = object->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/user-groups
 */
int H_UserGroupCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *nameField = json_object_get(request, "name");
   if (!json_is_string(nameField))
   {
      context->setErrorResponse("Missing or invalid \"name\" field");
      return 400;
   }

   TCHAR *name = json_object_get_string_t(request, "name", nullptr);
   if (!IsValidObjectName(name))
   {
      MemFree(name);
      context->setErrorResponse("Invalid group name");
      return 400;
   }

   uint32_t groupId;
   uint32_t rcc = CreateNewUser(name, true, &groupId);
   MemFree(name);

   if (rcc == RCC_OBJECT_ALREADY_EXISTS)
   {
      context->setErrorResponse("Group with this name already exists");
      return 409;
   }
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SECURITY, true, 0, L"User group [%u] created", groupId);

   IntegerArray<uint32_t> ids(1);
   ids.add(groupId);
   unique_ptr<ObjectArray<UserDatabaseObject>> result = FindUserDBObjects(ids);
   if (result != nullptr && !result->isEmpty())
   {
      json_t *output = result->get(0)->toJson();
      context->setResponseData(output);
      json_decref(output);
   }
   return 201;
}

/**
 * Handler for PUT /v1/user-groups/:group-id
 */
int H_UserGroupUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   uint32_t groupId = context->getPlaceholderValueAsUInt32(L"group-id");
   if (!(groupId & GROUP_FLAG))
   {
      context->setErrorResponse("Invalid group ID");
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *oldValue = nullptr, *newValue = nullptr;
   uint32_t rcc = ModifyUserDatabaseObjectFromJson(groupId, request, &oldValue, &newValue);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLogWithValues(AUDIT_SECURITY, true, 0, oldValue, newValue, L"User group [%u] updated", groupId);
      json_decref(oldValue);

      context->setResponseData(newValue);
      json_decref(newValue);
      return 200;
   }

   if (oldValue != nullptr)
      json_decref(oldValue);
   if (newValue != nullptr)
      json_decref(newValue);

   if (rcc == RCC_INVALID_USER_ID)
      return 404;
   if (rcc == RCC_INVALID_OBJECT_NAME)
   {
      context->setErrorResponse("Invalid group name");
      return 400;
   }
   if (rcc == RCC_OBJECT_ALREADY_EXISTS)
   {
      context->setErrorResponse("Group with this name already exists");
      return 409;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Handler for DELETE /v1/user-groups/:group-id
 */
int H_UserGroupDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_USERS))
      return 403;

   uint32_t groupId = context->getPlaceholderValueAsUInt32(L"group-id");
   if (!(groupId & GROUP_FLAG))
   {
      context->setErrorResponse("Invalid group ID");
      return 400;
   }

   if (groupId == GROUP_EVERYONE)
   {
      context->setErrorResponse("Cannot delete \"Everyone\" group");
      return 400;
   }

   TCHAR groupName[MAX_USER_NAME];
   ResolveUserId(groupId, groupName, true);

   uint32_t rcc = DeleteUserDatabaseObject(groupId);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SECURITY, true, 0, L"User group %s [%u] deleted", groupName, groupId);
      return 204;
   }

   if (rcc == RCC_INVALID_USER_ID)
      return 404;

   context->setErrorResponse("Database failure");
   return 500;
}
