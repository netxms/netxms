/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: nc.cpp
**
**/

#include "aitools.h"
#include <nms_users.h>

/**
 * Send notification
 */
std::string F_SendNotification(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_SEND_NOTIFICATION) == 0)
      return std::string("User does not have rights to send notifications");

   const char *channel = json_object_get_string_utf8(arguments, "channel", nullptr);
   if ((channel == nullptr) || (channel[0] == 0))
      return std::string("Channel must be provided");

   wchar_t *recipient = json_object_get_string_w(arguments, "recipient", nullptr);
   if ((recipient == nullptr) || (recipient[0] == 0))
   {
      MemFree(recipient);
      return std::string("Recipient must be provided");
   }

   const char *subject = json_object_get_string_utf8(arguments, "subject", nullptr);

   const char *message = json_object_get_string_utf8(arguments, "message", nullptr);
   if ((message == nullptr) || (message[0] == 0))
      return std::string("Message must be provided");

   SendNotification(String(channel, "utf-8"), recipient, String(subject, "utf-8"), String(message, "utf-8"), 0, 0, uuid::NULL_UUID);
   MemFree(recipient);

   return std::string("Notification sent");
}

/**
 * Get list of configured notification channels
 */
std::string F_GetNotificationChannels(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   return JsonToString(GetNotificationChannels((systemAccess & SYSTEM_ACCESS_SERVER_CONFIG) == 0));
}
