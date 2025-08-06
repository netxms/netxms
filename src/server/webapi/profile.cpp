/*
** NetXMS - Network Management System
** Copyright (C) 2025 Raden Solutions
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
** File: profile.cpp
**
**/

#include "webapi.h"

/**
 * Handler for POST /v1/profile/push-token
 * Registers or updates a device token for push notifications
 */
int H_RegisterPushDeviceToken(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Empty device token registration request"));
      return 400;
   }

   const char *deviceToken = json_object_get_string_utf8(request, "deviceToken", nullptr);
   const char *platform = json_object_get_string_utf8(request, "platform", nullptr);

   if ((deviceToken == nullptr) || (*deviceToken == 0))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Device token registration request missing deviceToken"));
      context->setErrorResponse("Missing deviceToken");
      return 400;
   }

   if ((platform == nullptr) || (*platform == 0))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Device token registration request missing platform"));
      context->setErrorResponse("Missing platform");
      return 400;
   }

   if (strcmp(platform, "ios") != 0)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Device token registration request has unsupported platform: %hs"), platform);
      context->setErrorResponse("Unsupported platform");
      return 400;
   }

   size_t tokenLen = strlen(deviceToken);
   if (tokenLen != 64)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Device token registration request has invalid token format (length=%d, expected=64)"), (int)tokenLen);
      context->setErrorResponse("Invalid device token format");
      return 400;
   }

   for (size_t i = 0; i < tokenLen; i++)
   {
      if (!isxdigit(deviceToken[i]))
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Device token registration request has invalid token format (non-hex character at position %d)"), (int)i);
         context->setErrorResponse("Invalid device token format");
         return 400;
      }
   }

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 3, _T("Device token registration request: userId=%u, deviceToken=%hs, platform=%hs"),
      context->getUserId(), deviceToken, platform);

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI, _T("Registered push notification token for user %s (userId=%u): token=%hs, platform=%hs"),
      context->getLoginName(), context->getUserId(), deviceToken, platform);

   json_t *response = json_object();
   json_object_set_new(response, "success", json_true());
   context->setResponseData(response);
   json_decref(response);

   return 200;
}

/**
 * Handler for DELETE /v1/profile/push-token/:device-token
 * Removes a device token when user logs out or disables notifications
 */
int H_RemovePushDeviceToken(Context *context)
{
   const TCHAR *deviceTokenParam = context->getPlaceholderValue(_T("device-token"));
   if ((deviceTokenParam == nullptr) || (*deviceTokenParam == 0))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Device token removal request missing device token"));
      context->setErrorResponse("Missing device token");
      return 400;
   }

   size_t tokenLen = _tcslen(deviceTokenParam);
   if (tokenLen != 64)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Device token removal request has invalid token format (length=%d, expected=64)"), (int)tokenLen);
      context->setErrorResponse("Invalid device token format");
      return 400;
   }

   for (size_t i = 0; i < tokenLen; i++)
   {
      if (!_istxdigit(deviceTokenParam[i]))
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Device token removal request has invalid token format (non-hex character at position %d)"), (int)i);
         context->setErrorResponse("Invalid device token format");
         return 400;
      }
   }

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 3, _T("Device token removal request: userId=%u, deviceToken=%s"),
      context->getUserId(), deviceTokenParam);

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WEBAPI, _T("Removed push notification token for user %s (userId=%u): token=%s"),
      context->getLoginName(), context->getUserId(), deviceTokenParam);

   json_t *response = json_object();
   json_object_set_new(response, "success", json_true());
   context->setResponseData(response);
   json_decref(response);

   return 200;
}
