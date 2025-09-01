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
** File: info.cpp
**
**/

#include "webapi.h"
#include <netxms-version.h>

/**
 * Handler for /
 */
int H_Root(Context *context)
{
   json_t *response = json_object();
   json_object_set_new(response, "description", json_string("NetXMS web service API"));
   json_object_set_new(response, "version", json_string(NETXMS_VERSION_STRING_A));
   json_object_set_new(response, "build", json_string(NETXMS_BUILD_TAG_A));
   json_object_set_new(response, "apiVersion", json_integer(1));
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for /v1/server-info
 */
int H_ServerInfo(Context *context)
{
   json_t *response = json_object();
   json_object_set_new(response, "version", json_string(NETXMS_VERSION_STRING_A));
   json_object_set_new(response, "build", json_string(NETXMS_BUILD_TAG_A));
   json_object_set_new(response, "id", json_integer(g_serverId));

   wchar_t buffer[MAX_CONFIG_VALUE_LENGTH];
   ConfigReadStr(L"Server.Name", buffer, MAX_CONFIG_VALUE_LENGTH, L"");
   json_object_set_new(response, "name", json_string_w(buffer));

   ConfigReadStr(L"Server.Color", buffer, MAX_CONFIG_VALUE_LENGTH, L"");
   json_object_set_new(response, "color", json_string_w(buffer));

   ConfigReadStr(L"Server.MessageOfTheDay", buffer, MAX_CONFIG_VALUE_LENGTH, _T(""));
   json_object_set_new(response, "messageOfTheDay", json_string_w(buffer));

   json_object_set_new(response, "tz", json_string_w(GetSystemTimeZone(buffer, MAX_CONFIG_VALUE_LENGTH)));

   json_t *options = json_object();
   json_object_set_new(options, "zoningEnabled", json_boolean((g_flags & AF_ENABLE_ZONING) != 0));
   json_object_set_new(options, "strictAlarmStatusFlow", json_boolean(ConfigReadBoolean(_T("Alarms.StrictStatusFlow"), false)));
   json_object_set_new(options, "timedAlarmAckEnabled", json_boolean(ConfigReadBoolean(_T("Alarms.EnableTimedAck"), false)));
   json_object_set_new(options, "helpdeskLinkActive", json_boolean((g_flags & AF_HELPDESK_LINK_ACTIVE) != 0));

   ConfigReadStr(L"Client.TileServerURL", buffer, MAX_CONFIG_VALUE_LENGTH, L"http://tile.netxms.org/osm/");
   json_object_set_new(options, "tileServerURL", json_string_w(buffer));

   json_object_set_new(response, "options", options);

   json_t *dateTimeFormat = json_object();

   ConfigReadStr(L"Client.DefaultConsoleDateFormat", buffer, MAX_CONFIG_VALUE_LENGTH, L"dd.MM.yyyy");
   json_object_set_new(dateTimeFormat, "date", json_string_w(buffer));

   ConfigReadStr(L"Client.DefaultConsoleTimeFormat", buffer, MAX_CONFIG_VALUE_LENGTH, L"HH:mm:ss");
   json_object_set_new(dateTimeFormat, "timeLong", json_string_w(buffer));

   ConfigReadStr(L"Client.DefaultConsoleShortTimeFormat", buffer, MAX_CONFIG_VALUE_LENGTH, L"HH:mm");
   json_object_set_new(dateTimeFormat, "timeShort", json_string_w(buffer));

   json_object_set_new(response, "dateTimeFormat", dateTimeFormat);
   json_object_set_new(response, "components", ComponentsToJson());

   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for /v1/status
 */
int H_Status(Context *context)
{
   json_t *response = json_object();
   json_object_set_new(response, "userId", json_integer(context->getUserId()));
   json_object_set_new(response, "userName", json_string_t(context->getLoginName()));
   json_object_set_new(response, "systemAccessRights", json_integer(context->getSystemAccessRights()));
   context->setResponseData(response);
   json_decref(response);
   return 200;
}
