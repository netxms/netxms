/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: notifications.cpp
**
**/

#include "webapi.h"
#include <nms_actions.h>

/**
 * Handler for GET /v1/notification-channels
 */
int H_NotificationChannels(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *output = GetNotificationChannels(false);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/notification-channels/:channel-name
 */
int H_NotificationChannelDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *channelName = context->getPlaceholderValue(L"channel-name");
   if (channelName == nullptr)
      return 400;

   json_t *output = GetNotificationChannelByName(channelName);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/notification-channels
 */
int H_NotificationChannelCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonName = json_object_get(request, "name");
   json_t *jsonDriverName = json_object_get(request, "driverName");
   if (!json_is_string(jsonName) || !json_is_string(jsonDriverName))
   {
      context->setErrorResponse("Missing or invalid required fields (name, driverName)");
      return 400;
   }

   wchar_t name[MAX_OBJECT_NAME], driverName[MAX_OBJECT_NAME], description[MAX_NC_DESCRIPTION];
   utf8_to_wchar(json_string_value(jsonName), -1, name, MAX_OBJECT_NAME);
   utf8_to_wchar(json_string_value(jsonDriverName), -1, driverName, MAX_OBJECT_NAME);

   if (name[0] == 0)
   {
      context->setErrorResponse("Channel name cannot be empty");
      return 400;
   }

   if (driverName[0] == 0)
   {
      context->setErrorResponse("Driver name cannot be empty");
      return 400;
   }

   if (IsNotificationChannelExists(name))
   {
      context->setErrorResponse("Notification channel with this name already exists");
      return 409;
   }

   json_t *jsonDescription = json_object_get(request, "description");
   if (json_is_string(jsonDescription))
      utf8_to_wchar(json_string_value(jsonDescription), -1, description, MAX_NC_DESCRIPTION);
   else
      description[0] = 0;

   json_t *jsonConfig = json_object_get(request, "configuration");
   char *configuration = json_is_string(jsonConfig) ? MemCopyStringA(json_string_value(jsonConfig)) : MemCopyStringA("");

   CreateNotificationChannelAndSave(name, description, driverName, configuration);
   NotifyClientSessions(NX_NOTIFY_NC_CHANNEL_CHANGED, 0);
   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Notification channel \"%s\" created via REST API", name);

   json_t *output = GetNotificationChannelByName(name);
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for PUT /v1/notification-channels/:channel-name
 */
int H_NotificationChannelUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *channelName = context->getPlaceholderValue(L"channel-name");
   if (channelName == nullptr)
      return 400;

   if (!IsNotificationChannelExists(channelName))
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   // Get current channel data to use as defaults for unspecified fields
   json_t *current = GetNotificationChannelByName(channelName);
   if (current == nullptr)
      return 404;

   json_t *jsonDriverName = json_object_get(request, "driverName");
   const char *driverNameUtf8 = json_is_string(jsonDriverName) ? json_string_value(jsonDriverName) : json_string_value(json_object_get(current, "driverName"));

   json_t *jsonDescription = json_object_get(request, "description");
   const char *descriptionUtf8 = json_is_string(jsonDescription) ? json_string_value(jsonDescription) : json_string_value(json_object_get(current, "description"));

   wchar_t driverName[MAX_OBJECT_NAME], description[MAX_NC_DESCRIPTION];
   utf8_to_wchar(driverNameUtf8, -1, driverName, MAX_OBJECT_NAME);
   utf8_to_wchar(descriptionUtf8, -1, description, MAX_NC_DESCRIPTION);

   json_t *jsonConfig = json_object_get(request, "configuration");
   char *configuration;
   if (json_is_string(jsonConfig))
      configuration = MemCopyStringA(json_string_value(jsonConfig));
   else
      configuration = MemCopyStringA(json_string_value(json_object_get(current, "configuration")));

   json_decref(current);

   UpdateNotificationChannel(channelName, description, driverName, configuration);
   MemFree(configuration);
   NotifyClientSessions(NX_NOTIFY_NC_CHANNEL_CHANGED, 0);
   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Notification channel \"%s\" updated via REST API", channelName);

   json_t *output = GetNotificationChannelByName(channelName);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/notification-channels/:channel-name
 */
int H_NotificationChannelDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *channelName = context->getPlaceholderValue(L"channel-name");
   if (channelName == nullptr)
      return 400;

   wchar_t name[MAX_OBJECT_NAME];
   wcslcpy(name, channelName, MAX_OBJECT_NAME);

   if (CheckChannelIsUsedInAction(name))
   {
      context->setErrorResponse("Notification channel is used in server actions");
      return 409;
   }

   if (!DeleteNotificationChannel(name))
      return 404;

   NotifyClientSessions(NX_NOTIFY_NC_CHANNEL_CHANGED, 0);
   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Notification channel \"%s\" deleted via REST API", name);
   return 204;
}

/**
 * Handler for POST /v1/notification-channels/:channel-name/rename
 */
int H_NotificationChannelRename(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *channelName = context->getPlaceholderValue(L"channel-name");
   if (channelName == nullptr)
      return 400;

   if (!IsNotificationChannelExists(channelName))
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonNewName = json_object_get(request, "newName");
   if (!json_is_string(jsonNewName))
   {
      context->setErrorResponse("Missing or invalid required field (newName)");
      return 400;
   }

   wchar_t newName[MAX_OBJECT_NAME];
   utf8_to_wchar(json_string_value(jsonNewName), -1, newName, MAX_OBJECT_NAME);
   if (newName[0] == 0)
   {
      context->setErrorResponse("New channel name cannot be empty");
      return 400;
   }

   if (IsNotificationChannelExists(newName))
   {
      context->setErrorResponse("Notification channel with this name already exists");
      return 409;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Notification channel \"%s\" renamed to \"%s\" via REST API", channelName, newName);
   RenameNotificationChannel(MemCopyString(channelName), MemCopyString(newName));
   NotifyClientSessions(NX_NOTIFY_NC_CHANNEL_CHANGED, 0);
   return 200;
}

/**
 * Handler for POST /v1/notification-channels/:channel-name/clear-queue
 */
int H_NotificationChannelClearQueue(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *channelName = context->getPlaceholderValue(L"channel-name");
   if (channelName == nullptr)
      return 400;

   if (!ClearNotificationChannelQueue(channelName))
      return 404;

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Notification channel \"%s\" queue cleared via REST API", channelName);
   return 204;
}

/**
 * Handler for POST /v1/notification-channels/:channel-name/send
 */
int H_NotificationChannelSend(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *channelName = context->getPlaceholderValue(L"channel-name");
   if (channelName == nullptr)
      return 400;

   if (!IsNotificationChannelExists(channelName))
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonRecipient = json_object_get(request, "recipient");
   json_t *jsonSubject = json_object_get(request, "subject");
   json_t *jsonBody = json_object_get(request, "body");

   if (!json_is_string(jsonRecipient) || !json_is_string(jsonBody))
   {
      context->setErrorResponse("Missing or invalid required fields (recipient, body)");
      return 400;
   }

   wchar_t *recipient = WideStringFromUTF8String(json_string_value(jsonRecipient));
   wchar_t subject[256];
   if (json_is_string(jsonSubject))
      utf8_to_wchar(json_string_value(jsonSubject), -1, subject, 256);
   else
      subject[0] = 0;

   wchar_t *body = WideStringFromUTF8String(json_string_value(jsonBody));

   SendNotification(channelName, recipient, subject, body, 0, 0, uuid::NULL_UUID);

   MemFree(recipient);
   MemFree(body);
   return 204;
}

/**
 * Handler for GET /v1/notification-drivers
 */
int H_NotificationDrivers(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *output = GetNotificationDriversAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
